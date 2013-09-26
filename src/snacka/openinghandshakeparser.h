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

#ifndef SN_OPENING_HANDSHAKE_PARSER_H
#define SN_OPENING_HANDSHAKE_PARSER_H

/*! \file */

#include "errorcodes.h"
#include "mutablestring.h"
#include "../external/http_parser/http_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    struct snWebsocket;
    
    /**
     *
     */
    typedef enum snHandshakeResponseHTTPField
    {
        SN_UNRECOGNIZED_HTTP_FIELD = -1,
        SN_HTTP_ACCEPT,
        SN_HTTP_UPGRADE,
        SN_HTTP_CONNECTION,
        SN_HTTP_WS_PROTOCOL,
        SN_HTTP_WS_EXTENSIONS
        
    } snHandshakeResponseHTTPField;
    
    /**
     * Incremental parser of websocket opening handshake http responses. 
     * @see http://tools.ietf.org/html/rfc6455#section-1.3
     */
    typedef struct snOpeningHandshakeParser
    {
        /** For parsing HTTP response headers */
        http_parser httpParser;
        /** */
        http_parser_settings httpParserSettings;
        /** */
        snError errorCode;
        /** */
        snHandshakeResponseHTTPField currentHeaderField;
        /** */
        int reachedHeaderEnd;
        /** */
        snMutableString acceptValue;
        /** */
        snMutableString connectionValue;
        /** */
        snMutableString upgradeValue;
        /** */
        snMutableString protocolValue;
        /** */
        snMutableString extensionsValue;
    } snOpeningHandshakeParser;

    /**
     * Initializes a handshake response parser.
     * @param parser The parser to initialize.
     */
    void snOpeningHandshakeParser_init(snOpeningHandshakeParser* parser);
    
    /**
     *
     */
    void snOpeningHandshakeParser_deinit(snOpeningHandshakeParser* parser);

    /**
     * Called when \reachedHeaderEnd has been set to validate the
     * received http header fields.
     * @see http://tools.ietf.org/html/rfc6455#section-4.1
     */
    void snOpeningHandshakeParser_createOpeningHandshakeRequest(snOpeningHandshakeParser* parser,
                                                                const char* host,
                                                                int port,
                                                                const char* path,
                                                                const char* queryString,
                                                                snMutableString* request);
    
    /**
     *
     */
    snError snOpeningHandshakeParser_processBytes(snOpeningHandshakeParser* parser,
                                                  const char* bytes,
                                                  int numBytes,
                                                  int* numBytesProcessed,
                                                  int* handshakeCompleted);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SN_OPENING_HANDSHAKE_PARSER_H*/
