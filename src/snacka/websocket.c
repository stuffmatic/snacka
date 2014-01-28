/*
 * Copyright (c) 2013, Per Gantelius
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the copyright holders.
 */

#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include <uriparser/Uri.h>

#include "backends/bsdsocket/iocallbacks_socket.h"
#include "websocket.h"
#include "openinghandshakeparser.h"
#include "frameparser.h"
#include "utf8.h"
#include "logging.h"

#include "frame.h"
#include <stdarg.h>

#define SN_DEFAULT_MAX_FRAME_SIZE 1 << 16

#define SN_DEFAULT_WRITE_CHUNK_SIZE 1 << 16

#define SN_CLOSING_HANDSHAKE_TIMEOUT 2.0 //in seconds

/** */
struct snWebsocket
{
    /** Handles parsing of the websocket opening handshake response. */
    snOpeningHandshakeParser openingHandshakeParser;
    /** Extracts websocket frames from incoming bytes. */
    snFrameParser frameParser;
    /** A set of callbacks for I/O operation */
    snIOCallbacks ioCallbacks;
    /** The object to pass to the I/O callbacks, e.g a socket. */
    void* ioObject;
    /** The URI scheme. */
    snMutableString uriScheme;
    /** The host. */
    snMutableString host;
    /** The port. */
    int port;
    /** The http request path, used in the websocket opening handshake request. */
    snMutableString pathTail;
    /** */
    snMutableString query;
    /** The maximum size of a frame, i.e header + payload. */
    int maxFrameSize;
    /** Buffer used for storing frames. */
    char* readBuffer;
    /** */
    int writeChunkSize;
    /** */
    char* writeChunkBuffer;
    /** */
    int hasCompletedOpeningHandshake;
    /** */
    int hasSentCloseFrame;
    /** Timer used to force disconnect if the closing handshake is too slow. */
    float closingHandshakeTimer;
    /** */
    snReadyState websocketState;
    /** */
    snIOCancelCallback cancelCallback;
    /** */
    snOpenCallback openCallback;
    /** */
    snFrameCallback frameCallback;
    /** */
    snCloseCallback closeCallback;
    /** */
    snErrorCallback errorCallback;
    /** */
    void* callbackData;
    /** */
    snLogCallback logCallback;
    /** */
    double prevPollTime;
    
};

static void log(snWebsocket* sn, const char* message, ...)
{
    if (sn->logCallback)
    {
        va_list args;
        va_start(args, message);
        vprintf(message, args); //TODO: actually use log callback
        va_end(args);

    }
}

static void disconnectWithStatus(snWebsocket* ws, snStatusCode status, snError error);

static int generateMaskingKey()
{
    return rand();
}


snError snWebsocket_sendFrame(snWebsocket* ws, snOpcode opcode, int numPayloadBytes, const char* payload)
{
    if (ws->hasSentCloseFrame)
    {
        return SN_NO_ERROR;
    }
    
    if (ws->websocketState != SN_STATE_OPEN)
    {
        return SN_WEBSOCKET_CONNECTION_IS_NOT_OPEN;
    }
    
    snFrame f;
    f.header.opcode = opcode;
    f.header.isMasked = 1;
    f.header.maskingKey = generateMaskingKey();
    f.header.isFinal = 1;
    f.header.payloadSize = numPayloadBytes;
    
    snError validationResult = snFrameHeader_validate(&f.header);
    
    if (validationResult != SN_NO_ERROR)
    {
        return validationResult;
    }
    
    f.payload = payload;
    
    unsigned long long payloadSize = f.header.payloadSize;
    char headerBytes[SN_MAX_HEADER_SIZE];
    int headerSize = 0;
    snFrameHeader_toBytes(&f.header, headerBytes, &headerSize);
    
    assert(payloadSize + headerSize <= ws->maxFrameSize);
    
    //send header
    int nBytesWritten = 0;
    snError sendResult = ws->ioCallbacks.writeCallback(ws->ioObject,
                                                       headerBytes,
                                                       headerSize,
                                                       &nBytesWritten,
                                                       ws->cancelCallback);
    if (sendResult != SN_NO_ERROR)
    {
        disconnectWithStatus(ws, SN_STATUS_UNEXPECTED_ERROR, sendResult);
        return sendResult;
    }
    
    //send masked payload in chunks
    int numBytesSent = 0;
    while (numBytesSent < payloadSize)
    {
        const int numBytesLeft = payloadSize - numBytesSent;
        const int chunkSize = numBytesLeft < ws->writeChunkSize ? numBytesLeft : ws->writeChunkSize;
        //copy the current chunk into the write buffer
        memcpy(ws->writeChunkBuffer, &f.payload[numBytesSent], chunkSize);
        //apply mask in place
        snFrameHeader_applyMask(&f.header, ws->writeChunkBuffer, chunkSize, numBytesSent);
        //send masked bytes
        snError sendResult = ws->ioCallbacks.writeCallback(ws->ioObject,
                                                           ws->writeChunkBuffer,
                                                           chunkSize,
                                                           &nBytesWritten,
                                                           ws->cancelCallback);
        if (sendResult != SN_NO_ERROR)
        {
            disconnectWithStatus(ws, SN_STATUS_UNEXPECTED_ERROR, sendResult);
            return sendResult;
        }
        //move to the next chunk
        numBytesSent += chunkSize;
    }
    
    return SN_NO_ERROR;
}

static void sendCloseFrame(snWebsocket* ws, snStatusCode code)
{
    if (ws->hasSentCloseFrame)
    {
        return;
    }
    
    ws->closingHandshakeTimer = 0.0f;
    
    char payload[2] = { (code >> 8) , (code >> 0) };
    
    snWebsocket_sendFrame(ws, SN_OPCODE_CONNECTION_CLOSE, 2, payload);
    
    ws->hasSentCloseFrame = 1;
}

/**
 * Intercepts state changes before passing them on to the user defined callback.
 */
void transitionToStateAndInvokeStateCallback(snWebsocket* ws, snReadyState state)
{
    const int oldState = ws->websocketState;
    ws->websocketState = state;
    
    if (state == SN_STATE_OPEN && ws->openCallback)
    {
        ws->openCallback(ws->callbackData);
    }
    else if (state == SN_STATE_CLOSED && oldState == SN_STATE_CONNECTING && ws->closeCallback)
    {
        //an error occurred before completing the opening handshake
        ws->closeCallback(ws->callbackData, SN_STATUS_UNEXPECTED_ERROR);
    }
    else if (state == SN_STATE_CLOSED && oldState == SN_STATE_OPEN && ws->closeCallback)
    {
        ws->closeCallback(ws->callbackData, SN_STATUS_ENDPOINT_GOING_AWAY);
    }
}

/**
 *
 */
static void disconnectWithStatus(snWebsocket* ws, snStatusCode status, snError error)
{
    if (error == SN_NO_ERROR)
    {
        sendCloseFrame(ws, status);
    }
    
    ws->ioCallbacks.disconnectCallback(ws->ioObject);
    
    if (ws->closeCallback)
    {
        ws->closeCallback(ws->callbackData, status);
    }
    
    transitionToStateAndInvokeStateCallback(ws, SN_STATE_CLOSED);
    
    if (error != SN_NO_ERROR && ws->errorCallback)
    {
        ws->errorCallback(ws->callbackData, error);
    }
}

static int isValidCloseCode(int code)
{
    switch (code)
    {
        case SN_STATUS_NORMAL_CLOSURE:
        case SN_STATUS_ENDPOINT_GOING_AWAY:
        case SN_STATUS_PROTOCOL_ERROR:
        case SN_STATUS_INVALID_DATA:
        case SN_STATUS_INCONSISTENT_DATA:
        case SN_STATUS_POLICY_VIOLATION:
        case SN_STATUS_MESSAGE_TOO_BIG:
        case SN_STATUS_MISSING_EXTENSION:
        case SN_STATUS_UNEXPECTED_ERROR:
        {
            return 1;
        }
        default:
            break;
    }
    
    //https://tools.ietf.org/html/rfc6455#section-7.4.2
    
    if (code >= 3000 && code <= 4999)
    {
        return 1;
    }
    
    return 0;
}

/**
 * Intercepts parsed frames before passing them on to the user defined callback.
 */
void invokeFrameCallback(void* data, const snFrame* frame)
{
    snWebsocket* ws = (snWebsocket*)data;
    
    snError headerValidationResult = snFrameHeader_validate(&frame->header);
    if (headerValidationResult != SN_NO_ERROR)
    {
        disconnectWithStatus(ws, SN_STATUS_PROTOCOL_ERROR, headerValidationResult);
        return;
    }
        
    if (ws->frameCallback)
    {
        ws->frameCallback(ws->callbackData, frame);
    }
    
    if (frame->header.opcode == SN_OPCODE_CONNECTION_CLOSE)
    {
        
        int closeCode = SN_STATUS_NORMAL_CLOSURE;
        if (frame->header.payloadSize == 1)
        {
            closeCode = SN_STATUS_PROTOCOL_ERROR;
        }
        else if (frame->header.payloadSize >= 2)
        {
            closeCode = (((unsigned char*)frame->payload)[0] << 8) |
                        (((unsigned char*)frame->payload)[1] << 0);
            if (!isValidCloseCode(closeCode))
            {
                closeCode = SN_STATUS_PROTOCOL_ERROR;
            }
            
            const int messageSize = frame->header.payloadSize - 2;
            char temp[256];
            memcpy(temp, &frame->payload[2], messageSize);
            temp[messageSize] = '\0';
            if (!snUTF8ValidateString(temp))
            {
                closeCode = SN_STATUS_INCONSISTENT_DATA;
            }
        }
        
        sendCloseFrame(ws, closeCode);
        disconnectWithStatus(ws, closeCode, SN_NO_ERROR);
    }
    else if (frame->header.opcode == SN_OPCODE_PING)
    {
        snWebsocket_sendFrame(ws, SN_OPCODE_PONG, frame->header.payloadSize, frame->payload);
    }
}

static void setDefaultIOCallbacks(snIOCallbacks* ioc)
{
    ioc->connectCallback = snSocketConnectCallback;
    ioc->deinitCallback = snSocketDeinitCallback;
    ioc->disconnectCallback = snSocketDisconnectCallback;
    ioc->initCallback = snSocketInitCallback;
    ioc->readCallback = snSocketReadCallback;
    ioc->writeCallback = snSocketWriteCallback;
}

snWebsocket* snWebsocket_create(snOpenCallback openCallback,
                                snMessageCallback messageCallback,
                                snCloseCallback closeCallback,
                                snErrorCallback errorCallback,
                                void* callbackData)
{
    snIOCallbacks ioc;
    memset(&ioc, 0, sizeof(snIOCallbacks));
    
    setDefaultIOCallbacks(&ioc);
    
    snWebsocketSettings s;
    s.frameCallback = NULL;
    s.ioCallbacks = &ioc;
    s.logCallback = NULL;
    s.maxFrameSize = 0;
    
    return snWebsocket_createWithSettings(openCallback,
                                          messageCallback,
                                          closeCallback,
                                          errorCallback,
                                          callbackData,
                                          &s);
}

snWebsocket* snWebsocket_createWithSettings(snOpenCallback openCallback,
                                            snMessageCallback messageCallback,
                                            snCloseCallback closeCallback,
                                            snErrorCallback errorCallback,
                                            void* callbackData,
                                            snWebsocketSettings* settings)
{
    snIOCallbacks ioCallbacksFallback;
    setDefaultIOCallbacks(&ioCallbacksFallback);
    snIOCallbacks* ioCallbacks = settings->ioCallbacks == NULL ? &ioCallbacksFallback : settings->ioCallbacks;
    
    snWebsocket* ws = (snWebsocket*)malloc(sizeof(snWebsocket));
    memset(ws, 0, sizeof(snWebsocket));
    memcpy(&ws->ioCallbacks, ioCallbacks, sizeof(snIOCallbacks));
    ws->ioCallbacks.initCallback(&ws->ioObject);
    
    ws->callbackData = callbackData;
    ws->openCallback = openCallback;
    ws->closeCallback = closeCallback;
    ws->errorCallback = errorCallback;
    
    ws->maxFrameSize = SN_DEFAULT_MAX_FRAME_SIZE;
    
    ws->websocketState = SN_STATE_CLOSED;
    
    ws->logCallback = snSilentLogCallback;
    
    if (settings)
    {
        if (settings->maxFrameSize != 0)
        {
            ws->maxFrameSize = settings->maxFrameSize;
        }
                
        if (settings->logCallback)
        {
            ws->logCallback = settings->logCallback;
        }
        
        if (settings->frameCallback)
        {
            ws->frameCallback = settings->frameCallback;
        }
        
        if (settings->cancelCallback)
        {
            ws->cancelCallback = settings->cancelCallback;
        }
    }
    
    if (ws->readBuffer == 0)
    {
        ws->readBuffer = malloc(ws->maxFrameSize);
        memset(ws->readBuffer, 0, ws->maxFrameSize);
    }
    
    ws->writeChunkSize = SN_DEFAULT_WRITE_CHUNK_SIZE;
    ws->writeChunkBuffer = malloc(ws->writeChunkSize);
        
    snFrameParser_init(&ws->frameParser,
                       invokeFrameCallback,
                       ws,
                       messageCallback,
                       callbackData,
                       ws->readBuffer,
                       ws->maxFrameSize);
    
    return ws;
}


void snWebsocket_delete(snWebsocket* ws)
{
    if (ws->ioObject)
    {
        ws->ioCallbacks.deinitCallback(ws->ioObject);
    }
    
    snFrameParser_deinit(&ws->frameParser);
    snOpeningHandshakeParser_deinit(&ws->openingHandshakeParser);
    
    snMutableString_deinit(&ws->uriScheme);
    snMutableString_deinit(&ws->host);
    snMutableString_deinit(&ws->pathTail);
    snMutableString_deinit(&ws->query);
    
    free(ws->readBuffer);
    
    free(ws->writeChunkBuffer);

    free(ws);
}

static int getPort(UriUriA* uri)
{
    char portTemp[256];
    const long portLength = uri->portText.afterLast - uri->portText.first;
    if (portLength == 0)
    {
        return -1;
    }
    
    memcpy(portTemp, uri->portText.first, portLength);
    portTemp[portLength] = '\0';
    int port = (int)strtol(portTemp, NULL, 10);
    
    return port;
}

static void sendOpeningHandshake(snWebsocket* ws)
{
    snMutableString req;
    snMutableString_init(&req);
    
    snOpeningHandshakeParser_createOpeningHandshakeRequest(&ws->openingHandshakeParser,
                                                           snMutableString_getString(&ws->host),
                                                           ws->port,
                                                           snMutableString_getString(&ws->pathTail),
                                                           snMutableString_getString(&ws->query),
                                                           &req);
    
    const char* reqStr = snMutableString_getString(&req);
    
    int numBytesWritten = 0;
    ws->ioCallbacks.writeCallback(ws->ioObject,
                                  reqStr,
                                  (int)strlen(reqStr),
                                  &numBytesWritten,
                                  ws->cancelCallback);
    
    snMutableString_deinit(&req);
}

snError snWebsocket_connect(snWebsocket* ws, const char* url)
{
    snMutableString_deinit(&ws->uriScheme);
    snMutableString_deinit(&ws->host);
    snMutableString_deinit(&ws->pathTail);
    snMutableString_deinit(&ws->query);
    
    snFrameParser_reset(&ws->frameParser);
    snOpeningHandshakeParser_init(&ws->openingHandshakeParser);
    
    transitionToStateAndInvokeStateCallback(ws, SN_STATE_CONNECTING);
    
    //parse the url
    UriParserStateA state;
    UriUriA uri;
  
    state.uri = &uri;
    if (uriParseUriA(&state, url) != URI_SUCCESS)
    {
        //parsing failed
        uriFreeUriMembersA(&uri);
        return SN_INVALID_URL;
    }
    else
    {
        //parsing succeeded
        
        ws->port = getPort(&uri);
        
        const long hostLength = uri.hostText.afterLast - uri.hostText.first;
        snMutableString_appendBytes(&ws->host, uri.hostText.first, hostLength);
        
        const long schemeLength = uri.scheme.afterLast - uri.scheme.first;
        snMutableString_appendBytes(&ws->uriScheme, uri.scheme.first, schemeLength);
        
        long tailLength = 0;
        if (uri.pathTail)
        {            
            tailLength = uri.pathTail->text.afterLast - uri.pathTail->text.first;
            snMutableString_appendBytes(&ws->pathTail, uri.pathTail->text.first, tailLength);
        }
        
        const long queryLength = uri.query.afterLast - uri.query.first;
        snMutableString_appendBytes(&ws->query, uri.query.first, queryLength);
        
        if (ws->port < 0)
        {
            //no port given in the url. use default port 80
            ws->port = 80;
        }
        
        //printf("scheme '%s', host '%s', port '%d'\n", ws->uriScheme, ws->host, ws->port);
        
        uriFreeUriMembersA(&uri);
    }
    
    snError e = ws->ioCallbacks.connectCallback(ws->ioObject,
                                                snMutableString_getString(&ws->host),
                                                ws->port,
                                                ws->cancelCallback);
    
    if (e != SN_NO_ERROR)
    {
        transitionToStateAndInvokeStateCallback(ws, SN_STATE_CLOSED);
        return e;
    }
    
    ws->hasCompletedOpeningHandshake = 0;
    ws->hasSentCloseFrame = 0;
    
    sendOpeningHandshake(ws);
    
    return SN_NO_ERROR;
}

void snWebsocket_disconnect(snWebsocket* ws, int disconnectImmediately)
{
    if (disconnectImmediately)
    {
        disconnectWithStatus(ws, SN_STATUS_ENDPOINT_GOING_AWAY, SN_NO_ERROR);
        transitionToStateAndInvokeStateCallback(ws, SN_STATE_CLOSED);
    }
    else
    {
        sendCloseFrame(ws, SN_STATUS_NORMAL_CLOSURE);
        transitionToStateAndInvokeStateCallback(ws, SN_STATE_CLOSING);
    }
}

snReadyState snWebsocket_getState(snWebsocket* ws)
{
    return ws->websocketState;
}

snError snWebsocket_sendPing(snWebsocket* ws, int payloadSize, const char* payload)
{
    return snWebsocket_sendFrame(ws, SN_OPCODE_PING, payloadSize, payload);
}

snError snWebsocket_sendTextData(snWebsocket* ws, const char* payload)
{
    return snWebsocket_sendFrame(ws, SN_OPCODE_TEXT, strlen(payload), payload);
}

snError snWebsocket_sendBinaryData(snWebsocket* ws, int payloadSize, const char* payload)
{
    return snWebsocket_sendFrame(ws, SN_OPCODE_BINARY, payloadSize, payload);
}

static void handlePaserResult(snWebsocket* ws, snError error)
{
    if (error == SN_NO_ERROR)
    {
        return;
    }
    
    //printf("handle parser result: error %d\n", error);
    
    snStatusCode status = SN_STATUS_PROTOCOL_ERROR;
    
    if (error == SN_INVALID_UTF8)
    {
        status = SN_STATUS_INCONSISTENT_DATA;
    }
    
    /*if (ws->errorCallback)
    {
        ws->errorCallback(ws->callbackData, error);
    }*/
    
    //invokes error callback if an error occurred.
    disconnectWithStatus(ws, status, error);
}

void snWebsocket_poll(snWebsocket* ws)
{
    if (ws->websocketState == SN_STATE_CLOSED)
    {
        return;
    }
    
    //update timer
    {
        int firstPoll = ws->prevPollTime == 0.0f;
        
        struct timeval time;
        gettimeofday(&time, NULL);
        
        const double newPollTime = time.tv_sec + time.tv_usec / 1000000.0;
        const double dt = newPollTime - ws->prevPollTime;
        
        if (!firstPoll)
        {
            if (ws->hasSentCloseFrame)
            {
                ws->closingHandshakeTimer += dt;
                if (ws->closingHandshakeTimer >= SN_CLOSING_HANDSHAKE_TIMEOUT)
                {
                    disconnectWithStatus(ws, SN_STATUS_ENDPOINT_GOING_AWAY, SN_NO_ERROR);
                }
            }
        }
        
        ws->prevPollTime = newPollTime;
    }
    
    snFrameParser* fp = &ws->frameParser;
    const int numBytesLeftInBuffer = fp->maxFrameSize - fp->bufferPosition;
    int numBytesRead = 0;
    char readBytes[1024];
    snError e = ws->ioCallbacks.readCallback(ws->ioObject,
                                             readBytes,
                                             1024,
                                             &numBytesRead);
    
    if (e != SN_NO_ERROR)
    {
        disconnectWithStatus(ws, SN_STATUS_UNEXPECTED_ERROR, e);
        return;
    }
    
    if (numBytesRead == 0)
    {
        return;
    }
    
    if (0)
    {
        log(ws, "bytes from socket:\n");
        log(ws, "-----------------------\n");
        for (int i = 0; i < numBytesRead; i++)
        {
            log(ws, "%c", readBytes[i]);
        }
        
        log(ws, "\n-----------------------\n");
    }

    int readOffset = 0;
    
    if (ws->hasCompletedOpeningHandshake == 0)
    {
        int done = 0;
        //printf("hasCompletedOpeningHandshake %d\n", ws->hasCompletedOpeningHandshake);
        snError result = snOpeningHandshakeParser_processBytes(&ws->openingHandshakeParser,
                                                               readBytes,
                                                               numBytesRead,
                                                               &readOffset,
                                                               &done);
        
        ws->hasCompletedOpeningHandshake = done;
        //printf("first character after header %c. after header '%s'\n", readBytes[readOffset], &readBytes[readOffset]);
        
        if (result != SN_NO_ERROR)
        {
            snOpeningHandshakeParser_deinit(&ws->openingHandshakeParser);
            handlePaserResult(ws, result);
            return;
        }
        
        if (ws->hasCompletedOpeningHandshake)
        {
            transitionToStateAndInvokeStateCallback(ws, SN_STATE_OPEN);
            snOpeningHandshakeParser_deinit(&ws->openingHandshakeParser);
        }
        
    }
    
    assert(numBytesRead >= readOffset);
    
    if (ws->hasCompletedOpeningHandshake && readOffset < numBytesRead)
    {
        snError result = snFrameParser_processBytes(&ws->frameParser,
                                                    &readBytes[readOffset],
                                                    numBytesRead - readOffset);
        handlePaserResult(ws, result);
    }
}

