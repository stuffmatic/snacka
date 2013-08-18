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
#include <stdlib.h>
#include "openinghandshakeparser.h"

static const char* HTTP_ACCEPT_FIELD_NAME = "Sec-WebSocket-Accept";
static const char* HTTP_UPGRADE_FIELD_NAME = "Upgrade";
static const char* HTTP_CONNECTION_FIELD_NAME = "Connection";

static int stringStartsWith(const char* haystack, int haystackLen, const char* needle, int needleLen)
{
    if (haystackLen < needleLen)
    {
        return 0;
    }
    
    const int minLen = haystackLen < needleLen ? haystackLen : needleLen;
    
    for (int i = 0; i < minLen; i++)
    {
        if (haystack[i] != needle[i])
        {
            return 0;
        }
    }
    
    return 1;
}


static int on_header_field(http_parser* p, const char *at, size_t length)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    
    parser->currentHeaderField = SN_UNRECOGNIZED_HTTP_FIELD;
    
    if (stringStartsWith(at, length, HTTP_ACCEPT_FIELD_NAME, strlen(HTTP_ACCEPT_FIELD_NAME)))
    {
        parser->currentHeaderField = SN_HTTP_ACCEPT;
    }
    else if (stringStartsWith(at, length, HTTP_UPGRADE_FIELD_NAME, strlen(HTTP_UPGRADE_FIELD_NAME)))
    {
        parser->currentHeaderField = SN_HTTP_UPGRADE;
    }
    else if (stringStartsWith(at, length, HTTP_CONNECTION_FIELD_NAME, strlen(HTTP_CONNECTION_FIELD_NAME)))
    {
        parser->currentHeaderField = SN_HTTP_CONNECTION;
    }
    
    return 0;
}

static int on_header_value(http_parser* p, const char *at, size_t length)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    
    if (parser->currentHeaderField == SN_HTTP_ACCEPT)
    {
        snMutableString_appendBytes(&parser->acceptValue, at, length);
    }
    else if (parser->currentHeaderField == SN_HTTP_CONNECTION)
    {
    
    }
    else if (parser->currentHeaderField == SN_HTTP_UPGRADE)
    {
        
    }
    
    return 0;
}

static int on_message_begin(http_parser* p)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    //printf("-------------ON MESSAGE BEGIN----------\n");
    return 0;
}

static int on_message_complete(http_parser* p)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    //c->response.bodySize = c->response.size - c->response.bodyStart;
    //printf("-----------ON MESSAGE COMPLETE---------\n");
    return 0;
}

static int on_headers_complete(http_parser* p)
{
    snOpeningHandshakeParser* parser = (snOpeningHandshakeParser*)p->data;
    if (p->status_code != 101)
    {
        /*
         http://tools.ietf.org/html/rfc6455#section-1.3
         Any status code other than 101 indicates that the WebSocket handshake
         has not completed and that the semantics of HTTP still apply.
         */
        parser->errorCode = SN_OPENING_HANDSHAKE_FAILED;
    }
    
    parser->reachedHeaderEnd = 1;
    
    return 0;
}


void snOpeningHandshakeParser_init(snOpeningHandshakeParser* p)
{
    //set up http header parser
    memset(&p->httpParserSettings, 0, sizeof(http_parser_settings));
    p->httpParserSettings.on_headers_complete = on_headers_complete;
    p->httpParserSettings.on_message_begin = on_message_begin;
    p->httpParserSettings.on_message_complete = on_message_complete;
    p->httpParserSettings.on_header_field = on_header_field;
    p->httpParserSettings.on_header_value = on_header_value;
    http_parser_init(&p->httpParser, HTTP_RESPONSE);
    p->httpParser.data = p;
    
    p->currentHeaderField = SN_UNRECOGNIZED_HTTP_FIELD;
    snMutableString_init(&p->acceptValue);
    
    p->errorCode = SN_NO_ERROR;
}

void snOpeningHandshakeParser_deinit(snOpeningHandshakeParser* parser)
{
    snMutableString_deinit(&parser->acceptValue);
}

snError snOpeningHandshakeParser_processBytes(snOpeningHandshakeParser* p,
                                              const char* bytes,
                                              int numBytes,
                                              int* numBytesProcessed,
                                              int* handshakeCompleted)
{
    *numBytesProcessed = http_parser_execute(&p->httpParser,
                                             &p->httpParserSettings,
                                             bytes,
                                             numBytes);
    
    if (HTTP_PARSER_ERRNO(&p->httpParser) != HPE_OK)
    {
        //http response header parsing error
        return SN_OPENING_HANDSHAKE_FAILED;
    }
    
    if (p->reachedHeaderEnd)
    {
        //printf("Accept value: %s\n", snMutableString_getString(&p->acceptValue));
    }
    
    *handshakeCompleted = p->reachedHeaderEnd;
    
    return SN_NO_ERROR;
}
