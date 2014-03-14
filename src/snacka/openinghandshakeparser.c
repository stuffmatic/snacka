/*
 * Copyright (c) 2013 - 2014, Per Gantelius
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
#include <stdlib.h>
#include <stdio.h>
#include "openinghandshakeparser.h"
#include "websocket.h"


static const char* HTTP_ACCEPT_FIELD_NAME = "Sec-WebSocket-Accept";
static const char* HTTP_UPGRADE_FIELD_NAME = "Upgrade";
static const char* HTTP_CONNECTION_FIELD_NAME = "Connection";
static const char* HTTP_WS_PROTOCOL_NAME = "Sec-WebSocket-Protocol";
static const char* HTTP_WS_EXTENSIONS_NAME = "Sec-WebSocket-Extensions";

static snError validateResponse(snOpeningHandshakeParser* p);


static int on_header_field(http_parser* p, const char *at, size_t length)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    
    /* Collect bytes forming the name of the current header field. */
    snMutableString_appendBytes(&parser->currentHeaderFieldName, at, length);
    
    return 0;
}

static int on_header_value(http_parser* p, const char *at, size_t length)
{
    
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    if (strlen(snMutableString_getString(&parser->currentHeaderFieldName)) > 0)
    {
        /*This is the first byte of a header field, the name of which is 
         stored in parser->currentHeaderFieldName. See if it's a field
         we should care about.*/
        
        parser->currentHeaderField = SN_UNRECOGNIZED_HTTP_FIELD;
        const char* fn = snMutableString_getString(&parser->currentHeaderFieldName);
        
        if (strcmp(HTTP_ACCEPT_FIELD_NAME, fn) == 0)
        {
            parser->currentHeaderField = SN_HTTP_ACCEPT;
        }
        else if (strcmp(HTTP_UPGRADE_FIELD_NAME, fn) == 0)
        {
            parser->currentHeaderField = SN_HTTP_UPGRADE;
        }
        else if (strcmp(HTTP_CONNECTION_FIELD_NAME, fn) == 0)
        {
            parser->currentHeaderField = SN_HTTP_CONNECTION;
        }
        else if (strcmp(HTTP_WS_PROTOCOL_NAME, fn) == 0)
        {
            parser->currentHeaderField = SN_HTTP_WS_PROTOCOL;
        }
        else if (strcmp(HTTP_WS_EXTENSIONS_NAME, fn) == 0)
        {
            parser->currentHeaderField = SN_HTTP_WS_EXTENSIONS;
        }
        
        snMutableString_deinit(&parser->currentHeaderFieldName);
    }
    
    
    if (parser->currentHeaderField != SN_UNRECOGNIZED_HTTP_FIELD)
    {
        snMutableString_appendBytes(&parser->headerFieldValues[parser->currentHeaderField], at, length);
    }
    
    return 0;
}

static int on_message_begin(http_parser* p)
{
    /*snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
     printf("-------------ON MESSAGE BEGIN----------\n");*/
    return 0;
}

static int on_message_complete(http_parser* p)
{
    /*snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    c->response.bodySize = c->response.size - c->response.bodyStart;
    printf("-----------ON MESSAGE COMPLETE---------\n");*/
    return 0;
}

static int on_headers_complete(http_parser* p)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    snError e = SN_NO_ERROR;
    
    if (p->status_code != 101)
    {
        /*
         http://tools.ietf.org/html/rfc6455#section-1.3
         Any status code other than 101 indicates that the WebSocket handshake
         has not completed and that the semantics of HTTP still apply.
         */
        e = SN_INVALID_OPENING_HANDSHAKE_HTTP_STATUS;
    }
    else
    {
        e = validateResponse(parser);
    }
    
    if (parser->parsingCallback)
    {
        parser->parsingCallback(parser->callbackData, e);
    }
    
    snOpeningHandshakeParser_deinit(parser);
    
    return 0;
}

static void generateKey(snMutableString* key)
{
    /*TODO: randomize properly*/
    snMutableString_append(key, "x3JJHMbDL1EzLkh9GBhXDw==");
}

void snOpeningHandshakeParser_init(snOpeningHandshakeParser* p,
                                   snOpeningHandshakeParsingCallback parsingCallback,
                                   void* callbackData)
{
    int i;
    memset(p, 0, sizeof(snOpeningHandshakeParser));
    
    p->parsingCallback = parsingCallback;
    p->callbackData = callbackData;
    
    /*set up http header parser*/
    memset(&p->httpParserSettings, 0, sizeof(http_parser_settings));
    
    p->httpParserSettings.on_headers_complete = on_headers_complete;
    p->httpParserSettings.on_message_begin = on_message_begin;
    p->httpParserSettings.on_message_complete = on_message_complete;
    p->httpParserSettings.on_header_field = on_header_field;
    p->httpParserSettings.on_header_value = on_header_value;
    http_parser_init(&p->httpParser, HTTP_RESPONSE);
    p->httpParser.data = p;
    
    p->currentHeaderField = SN_UNRECOGNIZED_HTTP_FIELD;
    snMutableString_init(&p->currentHeaderFieldName);
    for (i = 0; i < SN_HTTP_PARSED_HEADER_FIELD_COUNT; i++)
    {
        snMutableString_init(&p->headerFieldValues[i]);
    }
}

void snOpeningHandshakeParser_deinit(snOpeningHandshakeParser* p)
{
    int i;
    for (i = 0; i < SN_HTTP_PARSED_HEADER_FIELD_COUNT; i++)
    {
        snMutableString_deinit(&p->headerFieldValues[i]);
    }
    snMutableString_deinit(&p->currentHeaderFieldName);
}

void snOpeningHandshakeParser_createOpeningHandshakeRequest(snOpeningHandshakeParser* parser,
                                                            const char* host,
                                                            int port,
                                                            const char* path,
                                                            const char* queryString,
                                                            snMutableString* request)
{
    const size_t queryLength = strlen(queryString);
    
    /*GET*/
    snMutableString_append(request, "GET /");
    snMutableString_append(request, path);
    if (queryLength > 0)
    {
        snMutableString_append(request, "?");
        snMutableString_append(request, queryString);
    }
    snMutableString_append(request, " HTTP/1.1\r\n");
    
    /*Host*/
    snMutableString_append(request, "Host: ");
    snMutableString_append(request, host);
    snMutableString_append(request, ":");
    snMutableString_appendInt(request, port);
    snMutableString_append(request, "\r\n");
    
    /*Upgrade*/
    snMutableString_append(request, "Upgrade: websocket\r\n");
    
    /*Connection*/
    snMutableString_append(request, "Connection: Upgrade\r\n");
    
    /*Key*/
    snMutableString_append(request, "Sec-WebSocket-Key:");
    
    snMutableString key;
    snMutableString_init(&key);
    generateKey(&key);
    snMutableString_append(request, snMutableString_getString(&key));
    snMutableString_deinit(&key);
    
    snMutableString_append(request, "\r\n");
    
    /*Version*/
    snMutableString_append(request, "Sec-WebSocket-Version: 13\r\n");
    snMutableString_append(request, "\r\n");
}

static snError validateResponse(snOpeningHandshakeParser* p)
{
    /*http://tools.ietf.org/html/rfc6455#section-4.1*/
    
    /*
     If the response lacks an |Upgrade| header field or the |Upgrade|
     header field contains a value that is not an ASCII case-
     insensitive match for the value "websocket", the client MUST
     _Fail the WebSocket Connection_.
     */
    {
        const char* upgrVal = snMutableString_getString(&p->headerFieldValues[SN_HTTP_UPGRADE]);
        if (strlen(upgrVal) == 0)
        {
            return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
        }
        else
        {
            const char* comp[2] = {"websocket", "WEBSOCKET"};
            if (strlen(upgrVal) != strlen(comp[0]))
            {
                return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
            }
            else
            {
                int i;
                for (i = 0;  i < strlen(upgrVal); i++)
                {
                    if (upgrVal[i] != comp[0][i] &&
                        upgrVal[i] != comp[1][i])
                    {
                        return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
                        break;
                    }
                }
            }
        }
    }
    
    /*
     If the response lacks a |Connection| header field or the
     |Connection| header field doesn't contain a token that is an
     ASCII case-insensitive match for the value "Upgrade", the client
     MUST _Fail the WebSocket Connection_.
     */
    {
        const char* connVal = snMutableString_getString(&p->headerFieldValues[SN_HTTP_CONNECTION]);
        if (strlen(connVal) == 0)
        {
            return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
        }
        else
        {
            const char* comp[2] = {"UPGRADE", "upgrade"};
            if (strlen(connVal) != strlen(comp[0]))
            {
                return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
            }
            else
            {
                int i;
                for (i = 0;  i < strlen(connVal); i++)
                {
                    if (connVal[i] != comp[0][i] &&
                        connVal[i] != comp[1][i])
                    {
                        return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
                        break;
                    }
                }
            }
        }
    }
    
    /*
     If the response lacks a |Sec-WebSocket-Accept| header field or
    the |Sec-WebSocket-Accept| contains a value other than the
    base64-encoded SHA-1 of the concatenation of the |Sec-WebSocket-
    Key| (as a string, not base64-decoded) with the string "258EAFA5-
    E914-47DA-95CA-C5AB0DC85B11" but ignoring any leading and
    trailing whitespace, the client MUST _Fail the WebSocket
    Connection_.
     */
    {
        const char* accVal = snMutableString_getString(&p->headerFieldValues[SN_HTTP_ACCEPT]);
        if (strlen(accVal) == 0)
        {
            return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
        }
        
        /*TODO: validate accept value*/
        
    }
    
    /*
     If the response includes a |Sec-WebSocket-Extensions| header
     field and this header field indicates the use of an extension
     that was not present in the client's handshake (the server has
     indicated an extension not requested by the client), the client
     MUST _Fail the WebSocket Connection_.  (The parsing of this
     header field to determine which extensions are requested is
     discussed in Section 9.1.)
     */
    {
        /* No extensions were sent, so none should be received.*/
        if (strlen(snMutableString_getString(&p->headerFieldValues[SN_HTTP_WS_EXTENSIONS])) != 0)
        {
            return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
        }
    }
    
    /*
     If the response includes a |Sec-WebSocket-Protocol| header field
     and this header field indicates the use of a subprotocol that was
     not present in the client's handshake (the server has indicated a
     subprotocol not requested by the client), the client MUST _Fail
     the WebSocket Connection_.
     */
    if (strlen(snMutableString_getString(&p->headerFieldValues[SN_HTTP_WS_PROTOCOL])) != 0)
    {
        /* No protocols were sent, so none should be received.*/
        return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
    }
    
    return SN_NO_ERROR;
}

snError snOpeningHandshakeParser_processBytes(snOpeningHandshakeParser* p,
                                              const char* bytes,
                                              int numBytes,
                                              int* numBytesProcessed)
{
    
    if (HTTP_PARSER_ERRNO(&p->httpParser) != HPE_OK)
    {
        /*http response header parsing error*/
        return SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE;
    }
    
    *numBytesProcessed = http_parser_execute(&p->httpParser,
                                             &p->httpParserSettings,
                                             bytes,
                                             numBytes);
    
    return SN_NO_ERROR;
}
