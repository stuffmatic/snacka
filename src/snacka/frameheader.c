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
#include <stdio.h>
#include <stdlib.h>
#include "frame.h"

static const char* opcodeToString(snOpcode o)
{
    switch (o)
    {
        case SN_OPCODE_TEXT:
            return "Text";
        case SN_OPCODE_BINARY:
            return "Binary";
        case SN_OPCODE_PING:
            return "Ping";
        case SN_OPCODE_PONG:
            return "Pong";
        case SN_OPCODE_CONTINUATION:
            return "Continuation";
        case SN_OPCODE_CONNECTION_CLOSE:
            return "Connection close";
        default:
        {
            assert(0 && "invalid opcode");
            return "INVALID OPCODE";
        }
    }
}

static int isOpcodeValid(snOpcode oc)
{
    switch (oc)
    {
        case SN_OPCODE_CONNECTION_CLOSE:
        case SN_OPCODE_CONTINUATION:
        case SN_OPCODE_PING:
        case SN_OPCODE_PONG:
        case SN_OPCODE_BINARY:
        case SN_OPCODE_TEXT:
        {
            return 1;
        }
        default:
            break;
    }
    return 0;
}

static int isControlOpcode(snOpcode oc)
{
    switch (oc)
    {
        case SN_OPCODE_CONNECTION_CLOSE:
        case SN_OPCODE_PING:
        case SN_OPCODE_PONG:
        {
            return 1;
        }
        default:
            break;
    }
    
    return 0;
}


snError snFrameHeader_toBytes(snFrameHeader* h, char* headerBytes, int* headerSize)
{
    unsigned char* maskBytes = (unsigned char*)(&h->maskingKey);
    unsigned char* bytes = (unsigned char*)headerBytes;
    int writeIdx = 0;
    
    snError validationResult = snFrameHeader_validate(h);
    
    if (headerSize)
    {
        *headerSize = 0;
    }
    
    if (validationResult != SN_NO_ERROR)
    {
        return validationResult;
    }
    
    bytes[writeIdx++] = (h->isFinal << 7) | h->opcode;
    
    if (h->payloadSize < 126)
    {
        bytes[writeIdx++] = (h->isMasked << 7) | h->payloadSize;
    }
    else if (h->payloadSize < 1 << 16)
    {
        int i;
        bytes[writeIdx++] = (h->isMasked << 7) | 126;
        for (i = 0; i < 2; i++)
        {
            bytes[writeIdx++] = (h->payloadSize >> ((1 - i) * 8));
        }
    }
    else
    {
        int i;
        bytes[writeIdx++] = (h->isMasked << 7) | 127;
        for (i = 0; i < 8; i++)
        {
            bytes[writeIdx++] = (h->payloadSize >> ((7 - i) * 8));
        }
    }
    
    if (h->isMasked)
    {
        int i;
        for (i = 0; i < 4; i++)
        {
            bytes[writeIdx++] = maskBytes[3 - i];
        }
    }
    
    if (headerSize)
    {
        *headerSize = writeIdx;
    }
    
    return SN_NO_ERROR;
}

snError snFrameHeader_fromBytes(snFrameHeader* h,
                                const char* headerBytes,
                                int* headerSize)
{
    memset(h, 0, sizeof(snFrameHeader));
    
    if (headerSize)
    {
        *headerSize = 0;
    }
    
    int readIdx = 0;
    /*first header byte. read FIN flag and opcode*/
    const char b1 = headerBytes[readIdx++];
    const int rsv1 = (b1 & 0x40) >> 6;
    const int rsv2 = (b1 & 0x20) >> 5;
    const int rsv3 = (b1 & 0x10) >> 4;
    if (rsv1 != 0 || rsv2 != 0 || rsv3 != 0)
    {
        return SN_NONZERO_RESVERVED_BIT;
    }
    
    h->isFinal = (b1 & 0x80) >> 7;
    h->opcode = b1 & 0xf;
    
    /*second header byte. read MASK flag and 7 payload size bits*/
    const char b2 = headerBytes[readIdx++];
    h->isMasked = (b2 & 0x80) >> 7;
    unsigned int payloadSize = b2 & 0x7f;
    
    if (payloadSize == 126)
    {
        /*the payload size is given by the next 2 bytes*/
        int i;
        for (i = 0; i < 2; i++)
        {
            h->payloadSize |= (((unsigned char*)headerBytes)[readIdx++] << ((1 - i) * 8));
        }
    }
    else if (payloadSize == 127)
    {
        /*the payload size is given by the next 8 bytes*/
        int i;
        for (i = 0; i < 8; i++)
        {
            h->payloadSize |= (((unsigned char*)headerBytes)[readIdx++] << ((7 - i) * 8));
        }
    }
    else
    {
        /*the payload size is given by the 7 bits just read.*/
        h->payloadSize = payloadSize;
    }
    
    /*read masking key if the frame is masked*/
    if (h->isMasked)
    {
        int i;
        for (i = 0; i < 4; i++)
        {
            h->maskingKey |= (((unsigned char*)headerBytes)[readIdx++] << ((3 - i) * 8));
        }
    }
    
    if (headerSize)
    {
        *headerSize = readIdx;
    }
    
    return snFrameHeader_validate(h);
}

int snFrameHeader_equals(const snFrameHeader* h1, const snFrameHeader* h2)
{
    return h1->isFinal == h2->isFinal &&
    h1->isMasked == h2->isMasked &&
    h1->maskingKey == h2->maskingKey &&
    h1->opcode == h2->opcode &&
    h1->payloadSize == h2->payloadSize;
}

snError snFrameHeader_validate(const snFrameHeader* h)
{
    /*check that the opcode is valid*/
    if (!isOpcodeValid(h->opcode))
    {
        return SN_INVALID_OPCODE;
    }
    
    /*check control frame payload size limit
      https://tools.ietf.org/html/rfc6455#section-5.5*/
    if (isControlOpcode(h->opcode))
    {
        if (h->payloadSize > 125)
        {
            return SN_CONTROL_FRAME_PAYLOAD_TOO_LARGE;
        }
        
        if (h->isFinal == 0)
        {
            return SN_NON_FINAL_CONTROL_FRAME;
        }
    }
    
    /*check masking key*/
    if (h->isMasked && h->maskingKey == 0)
    {
        return SN_MASKING_KEY_IS_ZERO;
    }
    
    return SN_NO_ERROR;
    
}

snError snFrameHeader_applyMask(snFrameHeader* h, char* payload, int numBytes, int offset)
{
    if (h->isMasked == 0)
    {
        return SN_NO_ERROR;
    }
    
    if (h->maskingKey == 0)
    {
        return SN_MASKING_KEY_IS_ZERO;
    }
    
    assert(offset >= 0);
    
    /*apply the mask to the payload*/
    unsigned char* maskBytes = (unsigned char*)(&h->maskingKey);
    int i;
    for (i = 0; i < numBytes; i++)
    {
        payload[i] = payload[i] ^ maskBytes[3 - ((i + offset) % 4)];
    }
    
    return SN_NO_ERROR;
}


void snFrameHeader_log(const snFrameHeader* h)
{
    printf("opcode 0x%x (%s), payload size %llu, ",
           h->opcode,
           opcodeToString(h->opcode),
           h->payloadSize);
    
    printf("%sfinal, ", h->isFinal ? "" : "not ");
    
    if (h->isMasked)
    {
        printf("masked (key %d)\n", h->maskingKey);
    }
    else
    {
        printf("not masked\n");
    }
}

