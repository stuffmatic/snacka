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

#ifndef SN_TEST_OPENING_HANDSHAKE_PARSER_H
#define SN_TEST_OPENING_HANDSHAKE_PARSER_H

#include "mutablestring.h"
#include "openinghandshakeparser.h"

static const char* const SEC_WEBSOCKET_KEY = "TODO";

static snError parserResult = SN_NO_ERROR;
static int doneParsing = 0;

static void openingHandshakeParsingCallback(void* userData, snError result)
{
    parserResult = result;
    doneParsing = 1;
}

static void runParserTest(snMutableString* response, snError expectedResult)
{
    snOpeningHandshakeParser p;
    
    const char* bytes = snMutableString_getString(response);
    const size_t nBytes = strlen(bytes);
    
    doneParsing = 0;
    
    for (int chunkSize = 1; chunkSize < nBytes; chunkSize++)
    {
        //parse using different chunk sizes
        int bytePos = 0;
        snOpeningHandshakeParser_init(&p, openingHandshakeParsingCallback, &parserResult);
        while (!doneParsing)
        {
            int numBytesProcessed = 0;
            snOpeningHandshakeParser_processBytes(&p, &bytes[bytePos], chunkSize, &numBytesProcessed);
            bytePos += numBytesProcessed;
        }
        
        sput_fail_unless(expectedResult == parserResult, "Opening handshake parser should give the expected result");
        
        snOpeningHandshakeParser_deinit(&p);
    }
}

static void testMissingWebsocketKey()
{
    snMutableString s;
    snMutableString_init(&s);
    
    snMutableString_append(&s, "HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    snMutableString_append(&s, "Upgrade: WebSocket\r\n");
    snMutableString_append(&s, "Connection: Upgrade\r\n");
    //snMutableString_append(&s, "Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\n");
    snMutableString_append(&s, "Server: Kaazing Gateway\r\n");
    snMutableString_append(&s, "Date: Sat, 08 Feb 2014 13:17:08 GMT\r\n");
    snMutableString_append(&s, "\r\n\r\n");
    
    runParserTest(&s, SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE);
    
    snMutableString_deinit(&s);
}

static void testWrongHTTPStatus()
{
    snMutableString s;
    snMutableString_init(&s);
    
    snMutableString_append(&s, "HTTP/1.1 102 Web Socket Protocol Handshake\r\n");
    snMutableString_append(&s, "Upgrade: WebSocket\r\n");
    snMutableString_append(&s, "Connection: Upgrade\r\n");
    snMutableString_append(&s, "Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\n");
    snMutableString_append(&s, "Server: Kaazing Gateway\r\n");
    snMutableString_append(&s, "Date: Sat, 08 Feb 2014 13:17:08 GMT\r\n");
    snMutableString_append(&s, "\r\n\r\n");
    
    runParserTest(&s, SN_INVALID_OPENING_HANDSHAKE_HTTP_STATUS);
    
    snMutableString_deinit(&s);
    
}

static void testWellFormedResponse()
{
    snMutableString s;
    snMutableString_init(&s);
    
    snMutableString_append(&s, "HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    snMutableString_append(&s, "Upgrade: WebSocket\r\n");
    snMutableString_append(&s, "Connection: Upgrade\r\n");
    snMutableString_append(&s, "Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\n");
    snMutableString_append(&s, "Server: Kaazing Gateway\r\n");
    snMutableString_append(&s, "Date: Sat, 08 Feb 2014 13:17:08 GMT\r\n");
    snMutableString_append(&s, "\r\n\r\n");
    
    runParserTest(&s, SN_NO_ERROR);
    
    snMutableString_deinit(&s);
}

static void testHeaderFollowedByFrames()
{
    snMutableString s;
    snMutableString_init(&s);
    
    snMutableString_append(&s, "HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    snMutableString_append(&s, "Upgrade: WebSocket\r\n");
    snMutableString_append(&s, "Connection: Upgrade\r\n");
    snMutableString_append(&s, "Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\n");
    snMutableString_append(&s, "Server: Kaazing Gateway\r\n");
    snMutableString_append(&s, "Date: Sat, 08 Feb 2014 13:17:08 GMT\r\n");
    snMutableString_append(&s, "\r\n\r\n");
    snMutableString_append(&s, "some extra stuff here");
    
    runParserTest(&s, SN_NO_ERROR);
    
    snMutableString_deinit(&s);
}

#endif /*SN_TEST_OPENING_HANDSHAKE_PARSER_H*/
