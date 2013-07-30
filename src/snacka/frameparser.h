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

#ifndef SN_FRAME_PARSER_H
#define SN_FRAME_PARSER_H

#include "utf8.h"
#include "frame.h"
#include "websocket.h"
#include "errorcodes.h"

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    /**
     * Extracts websocket frames from a stream of bytes.
     */
    typedef struct snFrameParser
    {
        /** */
        snFrameCallback frameCallback;
        /** */
        void* frameCallbackData;
        /** */
        snMessageCallback messageCallback;
        /** */
        void* messageCallbackData;
        /** */
        int maxFrameSize;
        /** */
        char* buffer;
        /** */
        char payloadSizeBytes[8];
        /** */
        char maskingKeyBytes[4];
        /** */
        char pingPongPayloadBuffer[128];

        /** */
        snFrameHeader currentFrameHeader;
        /** */
        int bufferPosition;
        /** The size in bytes of the current header. */
        int currentHeaderSize;
        /** */
        int isParsingHeader;
        /** */
        int currentFrameByte;
        /** */
        int firstPayloadSizeByte;
        /** */
        int numPayloadSizeBytes;
        /** */
        int continuationOffset;
        /** */
        int isWaitingForFinalFrame;
        /** */
        snOpcode continuationOpcode;
        /** */
        uint32_t utf8State;
    } snFrameParser;
    
    /**
     *
     */
    void snFrameParser_init(snFrameParser* parser,
                            snFrameCallback frameCallback,
                            void* frameCallbackData,
                            snMessageCallback messageCallback,
                            void* messageCallbackData,
                            char* readBuffer,
                            int maxFrameSize);
    
    /**
     *
     */
    void snFrameParser_deinit(snFrameParser* parser);
    
    /**
     * Resets the parser state.
     *
     */
    void snFrameParser_reset(snFrameParser* parser);
    
    /**
     * 
     * @return An error code, SN_NO_ERROR on success.
     */
    snError snFrameParser_processBytes(snFrameParser* parser,
                                       const char* bytes,
                                       int numBytes);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SN_FRAME_PARSER_H*/

