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
 * of the authors and should not be interpreted asrepresenting official policies,
 * either expressed or implied, of the copyright holders.
 */

#include <assert.h>
#include <string.h>

#include <uriparser/Uri.h>

#include "backends/bsdsocket/iocallbacks_socket.h"
#include "websocket.h"
#include "frameparser.h"
#include "utf8.h"
#include "logging.h"

#include "frame.h"
#include <stdarg.h>

#define SN_DEFAULT_MAX_FRAME_SIZE 1 << 16

#define SN_DEFAULT_WRITE_CHUNK_SIZE 1 << 16


/** */
struct snWebsocket
{
    /** A set of callbacks for I/O operation */
    snIOCallbacks ioCallbacks;
    /** The object to pass to the I/O callbacks, e.g a socket. */
    void* ioObject;
    /** The URI scheme. */
    char uriScheme[256]; //TODO: variable length?
    /** The host. */
    char host[256]; //TODO: variable length?
    /** The port. */
    int port;
    /**  */
    char pathTail[256];
    /** */
    char query[256];
    /** The http request path, used in the websocket opening handshake request. */
    char* httpRequestPath;
    /** The maximum size of a frame, i.e header + payload. */
    int maxFrameSize;
    /** Buffer used for storing frames. */
    char* readBuffer;
    /** */
    int writeChunkSize;
    /** */
    char* writeChunkBuffer;
    /** */
    int usesExternalWriteBuffer;
    /** Handles incoming bytes. */
    snFrameParser frameParser;
    /** */
    int hasCompletedOpeningHandshake;
    /** */
    int hasSentCloseFrame;
    /** */
    int handshakeResponseReadPosition;
    /** */
    snReadyState websocketState;
    /** */
    snReadyStateCallback stateCallback;
    /** */
    snFrameCallback frameCallback;
    /** */
    void* callbackData;
    /** */
    snLogCallback logCallback;
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
    snError sendResult = ws->ioCallbacks.writeCallback(ws->ioObject, headerBytes, headerSize, &nBytesWritten);
    if (sendResult != SN_NO_ERROR)
    {
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
        snError sendResult = ws->ioCallbacks.writeCallback(ws->ioObject, ws->writeChunkBuffer, chunkSize, &nBytesWritten);
        if (sendResult != SN_NO_ERROR)
        {
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
    
    /*snFrame f;
    f.header.opcode = SN_OPCODE_CONNECTION_CLOSE;
    f.header.isFinal = 1;
    f.header.isMasked = 1;
    f.header.maskingKey = generateMaskingKey();
    f.header.payloadSize = 2;*/
    
    char payload[2] = { (code >> 8) , (code >> 0) };
   // f.payload = payload;
    
    snWebsocket_sendFrame(ws, SN_OPCODE_CONNECTION_CLOSE, 2, payload);
    
    ws->hasSentCloseFrame = 1;
}

/**
 * Intercepts state changes before passing them on to the user defined callback.
 */
void invokeStateCallback(snWebsocket* ws, snReadyState state, int status)
{
    ws->websocketState = state;
    
    if (ws->stateCallback)
    {
        ws->stateCallback(ws->callbackData, state, status);
    }
}

/**
 *
 */
void disconnectWithStatus(snWebsocket* ws, snStatusCode status)
{
    sendCloseFrame(ws, status);
    ws->ioCallbacks.disconnectCallback(ws->ioObject);
    invokeStateCallback(ws, SN_STATE_CLOSED, status);
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
        //TODO: error handling
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
        disconnectWithStatus(ws, closeCode);
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

snWebsocket* snWebsocket_create(snReadyStateCallback stateCallback,
                                snMessageCallback messageCallback,
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
    
    return snWebsocket_createWithSettings(stateCallback, messageCallback, callbackData, &s);
}

snWebsocket* snWebsocket_createWithSettings(snReadyStateCallback stateCallback,
                                            snMessageCallback messageCallback,
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
    ws->stateCallback = stateCallback;
    
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
    
    ws->ioCallbacks.deinitCallback(ws->ioObject);
    snFrameParser_deinit(&ws->frameParser);
    
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
    //TODO proper Sec-WebSocket-Key
    
    const int queryLength = strlen(ws->query);
    char request[1024];
    sprintf(request, "GET /%s%s%s HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\nSec-WebSocket-Version: 13\r\n\r\n",
            ws->pathTail, queryLength > 0 ? "?" : "", ws->query,
            ws->host, ws->port);
    int numBytesWritten = 0;
    ws->ioCallbacks.writeCallback(ws->ioObject,
                                  request,
                                  (int)strlen(request),
                                  &numBytesWritten);
}

static void processHandshakeResponseLine(const char* line, int numBytes)
{
    //TODO
}

static void validateHandshakeResponse(const char* response, int numBytes)
{
    //TODO
}

snError snWebsocket_connect(snWebsocket* ws, const char* url)
{
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
        memcpy(ws->host, uri.hostText.first, hostLength);
        ws->host[hostLength] = '\0';
        
        const long schemeLength = uri.scheme.afterLast - uri.scheme.first;
        memcpy(ws->uriScheme, uri.scheme.first, schemeLength);
        ws->uriScheme[schemeLength] = '\0';
        
        long tailLength = 0;
        if (uri.pathTail)
        {            
            tailLength = uri.pathTail->text.afterLast - uri.pathTail->text.first;
            memcpy(ws->pathTail, uri.pathTail->text.first, tailLength);
            ws->pathTail[tailLength] = '\0';
        }
        
        const long queryLength = uri.query.afterLast - uri.query.first;
        memcpy(ws->query, uri.query.first, queryLength);
        ws->query[queryLength] = '\0';
        
        
        if (ws->port < 0)
        {
            //no port given in the url. use default port 80
            ws->port = 80;
        }
        
        //printf("scheme '%s', host '%s', port '%d'\n", ws->uriScheme, ws->host, ws->port);
        
        uriFreeUriMembersA(&uri);
    }
    
    snFrameParser_reset(&ws->frameParser);
    snError e = ws->ioCallbacks.connectCallback(ws->ioObject, ws->host, ws->port);
    
    if (e != SN_NO_ERROR)
    {
        return e;
    }
    
    ws->websocketState = SN_STATE_CLOSED;
    ws->hasCompletedOpeningHandshake = 0;
    ws->handshakeResponseReadPosition = 0;
    ws->hasSentCloseFrame = 0;
    
    invokeStateCallback(ws, SN_STATE_CONNECTING, 0);
    
    sendOpeningHandshake(ws);
    
    return SN_NO_ERROR;
}

void snWebsocket_disconnect(snWebsocket* ws)
{
    sendCloseFrame(ws, SN_STATUS_NORMAL_CLOSURE);
    invokeStateCallback(ws, SN_STATE_CLOSING, 0);
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
    
    disconnectWithStatus(ws, status);
}

void snWebsocket_poll(snWebsocket* ws)
{
    if (ws->websocketState == SN_STATE_CLOSED)
    {
        return;
    }
    
    snFrameParser* fp = &ws->frameParser;
    const int numBytesLeftInBuffer = fp->maxFrameSize - fp->bufferPosition;
    int numBytesRead = 0;
    char readBytes[1024];
    snError e = ws->ioCallbacks.readCallback(ws->ioObject,
                                             readBytes,
                                             1024,
                                             &numBytesRead);
    
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
    
    if (ws->hasCompletedOpeningHandshake == 0)
    {
        //TODO: what if rnrn sequence is split up between two consecutive buffers?
        for (int i = 3; i < numBytesRead; i++)
        {
            if (readBytes[i - 3] == '\r' &&
                readBytes[i - 2] == '\n' &&
                readBytes[i - 1] == '\r' &&
                readBytes[i - 0] == '\n')
            {
                //this could be a handshake response. TODO: perform further checks
                ws->hasCompletedOpeningHandshake = 1;
                invokeStateCallback(ws, SN_STATE_OPEN, 0);
                
                //TODO: duplicate call to snFrameParser_processBytes in else block below
                const int numBytesLeft = numBytesRead - i - 1;
                snError result = snFrameParser_processBytes(&ws->frameParser,
                                                            &readBytes[i + 1],
                                                            numBytesLeft);
                handlePaserResult(ws, result);
                
            }
        }
        
    }
    else
    {
        snError result = snFrameParser_processBytes(&ws->frameParser,
                                                    readBytes,
                                                    numBytesRead);
        handlePaserResult(ws, result);
    }
}

