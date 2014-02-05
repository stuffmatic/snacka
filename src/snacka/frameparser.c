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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frameparser.h"
#include "utf8.h"

static snError onFinishedParsingFrame(snFrameParser* parser)
{
    /*pass the frame to the frame callback,
    even if it's a continuation frame*/
    snFrame f;
    memcpy(&f.header, &parser->currentFrameHeader, sizeof(snFrameHeader));
    char* messageBuffer = NULL;
    
    const int isUTF8 = (parser->continuationOpcode == SN_OPCODE_TEXT && f.header.opcode == SN_OPCODE_CONTINUATION) ||
                        f.header.opcode == SN_OPCODE_TEXT;
    
    if (parser->currentFrameHeader.opcode == SN_OPCODE_PING ||
        parser->currentFrameHeader.opcode == SN_OPCODE_PONG)
    {
        messageBuffer = parser->pingPongPayloadBuffer;
        f.payload = parser->pingPongPayloadBuffer;
    }
    else
    {
        messageBuffer = parser->buffer;
        f.payload = &parser->buffer[parser->continuationOffset];
    }
    
    if (!f.header.isFinal)
    {
        parser->continuationOffset += f.header.payloadSize;
    }
    
    /*invoke the frame callback, if specified*/
    if (parser->frameCallback)
    {
        parser->frameCallback(parser->frameCallbackData, &f);
    }
    
    /*invoke the message callback if*/
    if (f.header.isFinal)
    {
        int totalPayloadSize = parser->continuationOffset + f.header.payloadSize;
        if (isUTF8)
        {
            parser->buffer[totalPayloadSize] = '\0';
            /*totalPayloadSize++;*/
        }
        
        unsigned char b = '\0';
        int v =snUTF8ValidateStringIncremental(&b, 1, &parser->utf8State);
        if (v == 0)
        {
            return SN_INVALID_UTF8;
        }
        
        if (parser->messageCallback)
        {
            snOpcode messageOpcode = f.header.opcode == SN_OPCODE_CONTINUATION ? parser->continuationOpcode : f.header.opcode;
            
            if (messageOpcode == SN_OPCODE_PING ||
                messageOpcode == SN_OPCODE_PONG ||
                messageOpcode == SN_OPCODE_TEXT ||
                messageOpcode == SN_OPCODE_BINARY)
            {
                parser->messageCallback(parser->messageCallbackData,
                                        messageOpcode,
                                        messageBuffer,
                                        totalPayloadSize);
            }
        }
        
        /*allow pings, pongs and close frames in between continuation frames*/
        if (f.header.opcode != SN_OPCODE_PONG &&
            f.header.opcode != SN_OPCODE_PING &&
            f.header.opcode != SN_OPCODE_CONNECTION_CLOSE)
        {
            parser->isWaitingForFinalFrame = 0;
        }
    }
    
    parser->isParsingHeader = 1;
    parser->currentFrameByte = 0;
    parser->bufferPosition = 0;
    parser->firstPayloadSizeByte = 0;
    parser->numPayloadSizeBytes = 0;
    memset(parser->payloadSizeBytes, 0, 8);
    memset(&parser->currentFrameHeader, 0, sizeof(snFrameHeader));
        
    return SN_NO_ERROR;
}

static snError onFinishedParsingHeader(snFrameParser* parser)
{
    assert(parser->isParsingHeader != 0);
    
    snFrameHeader* header = &parser->currentFrameHeader;
    
    snError result = snFrameHeader_validate(header);
    
    if (result != SN_NO_ERROR)
    {
        return result;
    }
    
    if (parser->isWaitingForFinalFrame &&
        header->opcode != SN_OPCODE_CONNECTION_CLOSE &&
        header->opcode != SN_OPCODE_CONTINUATION &&
        header->opcode != SN_OPCODE_PING &&
        header->opcode != SN_OPCODE_PONG)
    {
        /*only pings, pongs and continuation frames
         are allowed while waiting for a final fragment frame.*/
        return SN_EXPECTED_CONTINUATION_FRAME;
    }
    
    if (header->payloadSize > parser->maxFrameSize - SN_MAX_HEADER_SIZE)
    {
        return SN_EXCEEDED_MAX_PAYLOAD_SIZE;
    }
    
    const int isTextOrBinary = header->opcode == SN_OPCODE_BINARY ||
                               header->opcode == SN_OPCODE_TEXT;
    
    if (header->opcode == SN_OPCODE_CONTINUATION)
    {
        if (parser->isWaitingForFinalFrame == 0)
        {
            return SN_UNEXPECTED_CONTINUATION_FRAME;
        }
    }
    else if (isTextOrBinary)
    {
        if (header->isFinal)
        {
            
        }
        else
        {
            /*start collecting fragments*/
            parser->utf8State = 0;
            
            parser->continuationOpcode = header->opcode;
            parser->isWaitingForFinalFrame = 1;
            parser->continuationOffset = 0;
        }
    }

    parser->currentHeaderSize = parser->currentFrameByte;
    
    parser->isParsingHeader = 0;
    
    if (parser->currentFrameHeader.payloadSize == 0)
    {
        snError result = onFinishedParsingFrame(parser);
        if (result != SN_NO_ERROR)
        {
            return result;
        }
    }
    
    return SN_NO_ERROR;
}

void snFrameParser_init(snFrameParser* parser,
                        snFrameCallback frameCallback,
                        void* frameCallbackData,
                        snMessageCallback messageCallback,
                        void* messageCallbackData,
                        char* readBuffer,
                        int maxFrameSize)
{
    memset(parser, 0, sizeof(snFrameParser));
    parser->buffer = readBuffer;
    
    memset(parser->buffer, 0, maxFrameSize);
    parser->maxFrameSize = maxFrameSize;
    
    parser->frameCallback = frameCallback;
    parser->frameCallbackData = frameCallbackData;
    
    parser->messageCallback = messageCallback;
    parser->messageCallbackData = messageCallbackData;
    
    snFrameParser_reset(parser);
}

void snFrameParser_deinit(snFrameParser* parser)
{
    memset(parser, 0, sizeof(snFrameParser));
}

void snFrameParser_reset(snFrameParser* parser)
{
    parser->isWaitingForFinalFrame = 0;
    parser->continuationOffset = 0;
    parser->currentHeaderSize = 0;
    parser->isParsingHeader = 1;
    parser->currentFrameByte = 0;
    parser->bufferPosition = 0;
    parser->firstPayloadSizeByte = 0;
    parser->numPayloadSizeBytes = 0;
    parser->utf8State = 0;
    memset(parser->payloadSizeBytes, 0, 8);
    memset(&parser->currentFrameHeader, 0, sizeof(snFrameHeader));
}

snError snFrameParser_processBytes(snFrameParser* parser,
                                   const char* bytes,
                                   int numBytes)
{
    /*https://tools.ietf.org/html/rfc6455#section-5.2*/
    
    int currentSrcByte = 0;
    
    /*printf("snFrameParser_processBytes: %d bytes\n", numBytes);*/
    
    while (currentSrcByte < numBytes)
    {
        if (parser->isParsingHeader)
        {
            int doneParsingHeader = 0;
            /*parse header bytes one at a time*/
            if (parser->currentFrameByte == 0)
            {
                /*first header byte. read FIN flag and opcode*/
                const char b = bytes[currentSrcByte];
                const int rsv1 = (b & 0x40) >> 6;
                const int rsv2 = (b & 0x20) >> 5;
                const int rsv3 = (b & 0x10) >> 4;
                
                if (rsv1 != 0 || rsv2 != 0 || rsv3 != 0)
                {
                    return SN_NONZERO_RESVERVED_BIT;
                }
                
                const int isFinal = (b & 0x80) >> 7;
                const int opcode = b & 0xf;
                
                parser->currentFrameHeader.opcode = opcode;
                parser->currentFrameHeader.isFinal = isFinal;
            }
            else if (parser->currentFrameByte == 1)
            {
                /*second header byte. read MASK flag and 7 payload size bits*/
                const char b = bytes[currentSrcByte];
                const int isMasked = (b & 0x80) >> 7;
                parser->currentFrameHeader.isMasked = isMasked;
                unsigned int payloadSize = b & 0x7f;
                
                if (payloadSize == 126)
                {
                    /*the next two bytes contain the payload size*/
                    parser->firstPayloadSizeByte = 2;
                    parser->numPayloadSizeBytes = 2;
                }
                else if (payloadSize == 127)
                {
                    /*the next 8 bytes contain the payload size*/
                    parser->firstPayloadSizeByte = 2;
                    parser->numPayloadSizeBytes = 8;
                }
                else
                {
                    /*the payload size is given by the 7 bits just read.*/
                    parser->currentFrameHeader.payloadSize = payloadSize;
                    parser->firstPayloadSizeByte = 2;
                    if (!parser->currentFrameHeader.isMasked)
                    {
                        doneParsingHeader = 1;
                    }
                }
            }
            else if (parser->currentFrameByte < parser->firstPayloadSizeByte + parser->numPayloadSizeBytes)
            {
                const int idx = parser->currentFrameByte - parser->firstPayloadSizeByte;
                parser->payloadSizeBytes[idx] = bytes[currentSrcByte];
                
                if (idx == parser->numPayloadSizeBytes - 1)
                {
                    /*done parsing payload size*/
                    if (parser->numPayloadSizeBytes == 2)
                    {
                        unsigned char b0 = parser->payloadSizeBytes[0];
                        unsigned char b1 = parser->payloadSizeBytes[1];
                        parser->currentFrameHeader.payloadSize = (b0 << 8) | (b1 << 0);
                    }
                    else if (parser->numPayloadSizeBytes == 8)
                    {
                        int i;
                        for (i = 0; i < 8; i++)
                        {
                            parser->currentFrameHeader.payloadSize |= (((unsigned char*)parser->payloadSizeBytes)[i] << ((7 - i) * 8));
                        }
                    }
                    else
                    {
                        assert(0);
                    }
                    
                    if (!parser->currentFrameHeader.isMasked)
                    {
                        doneParsingHeader = 1;
                    }
                }
            }
            else if (parser->currentFrameHeader.isMasked)
            {
                if (parser->currentFrameByte >= parser->firstPayloadSizeByte + parser->numPayloadSizeBytes)
                {
                    const int idx = parser->currentFrameByte - parser->firstPayloadSizeByte - parser->numPayloadSizeBytes;
                    parser->maskingKeyBytes[idx] = bytes[currentSrcByte];
                    if (idx == 3)
                    {
                        int i;
                        for (i = 0; i < 4; i++)
                        {
                            parser->currentFrameHeader.maskingKey |= ((unsigned char*)parser->maskingKeyBytes)[i] << (3 - i) * 8;
                        }
                        doneParsingHeader = 1;
                    }
                }
            }
            else
            {
                assert(0);
            }
            
            if (!doneParsingHeader)
            {
                /*step to the next header byte*/
                parser->currentFrameByte++;
            }
            else
            {
                snError result = onFinishedParsingHeader(parser);
                if (result != SN_NO_ERROR)
                {
                    return result;
                }
            }
            currentSrcByte++;
        }
        else
        {
            unsigned long long payloadBytesLeft = parser->currentFrameHeader.payloadSize +
            parser->currentHeaderSize - parser->currentFrameByte;
            const int bytesLeft = numBytes - currentSrcByte;
            unsigned long long chunkSize = bytesLeft < payloadBytesLeft ? bytesLeft : payloadBytesLeft;
            
            
            if (parser->currentFrameHeader.opcode == SN_OPCODE_TEXT ||
                (parser->currentFrameHeader.opcode == SN_OPCODE_CONTINUATION &&
                 parser->continuationOpcode == SN_OPCODE_TEXT))
            {
                const int validUTF8 = snUTF8ValidateStringIncremental(&bytes[currentSrcByte], chunkSize, &parser->utf8State);
                if (!validUTF8)
                {
                    return SN_INVALID_UTF8;
                }
            }
            
            /*store ping/pong payload bytes in a separate buffer to allow for
              pings/pongs in between continuation frames.*/
            if (parser->currentFrameHeader.opcode == SN_OPCODE_PING ||
                parser->currentFrameHeader.opcode == SN_OPCODE_PONG)
            {
                memcpy(&parser->pingPongPayloadBuffer[parser->currentFrameByte - parser->currentHeaderSize],
                       &bytes[currentSrcByte],
                       chunkSize);
            }
            else
            {
                memcpy(&parser->buffer[parser->continuationOffset + parser->currentFrameByte - parser->currentHeaderSize],
                       &bytes[currentSrcByte],
                       chunkSize);
            }
            
            parser->currentFrameByte += chunkSize;
            currentSrcByte += chunkSize;
            
            assert(parser->currentFrameByte <= parser->currentFrameHeader.payloadSize + parser->currentHeaderSize);
            
            if (parser->currentFrameByte == parser->currentFrameHeader.payloadSize + parser->currentHeaderSize)
            {
                snError result = onFinishedParsingFrame(parser);
                if (result != SN_NO_ERROR)
                {
                    return result;
                }
            }
            
        }
    }
    
    return SN_NO_ERROR;
}