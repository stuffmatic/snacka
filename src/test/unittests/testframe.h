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

#ifndef SN_TEST_WEBSOCKET_FRAME_H
#define SN_TEST_WEBSOCKET_FRAME_H

#include <assert.h>
#include <string.h>

#include "sput.h"
#include "websocket.h"
#include "testframe.h"


static void testFrameHeaderSerialization()
{
    
    const int numCases = 5;
    int maskFlags[numCases] = {1, 0, 1, 1, 1};
    int maskingKeys[numCases] = {21, 0, 4000, 12399, 9999};
    int finalFlags[numCases] = {0, 1, 1, 1, 0};
    unsigned long long payloadSizes[numCases] = {(1 << 16) - 1, 1 << 2, (1 << 16) + 1, 126, 127};
    snOpcode opcodes[numCases] =
    {
        SN_OPCODE_TEXT,
        SN_OPCODE_BINARY,
        SN_OPCODE_TEXT,
        SN_OPCODE_TEXT,
        SN_OPCODE_TEXT
    };
    
    for (int i = 0; i < numCases; i++)
    {
        snFrameHeader hOrig;
        memset(&hOrig, 0, sizeof(snFrameHeader));
        
        snFrameHeader hRef;
        memset(&hRef, 0, sizeof(snFrameHeader));
        
        hOrig.isFinal = finalFlags[i];
        hOrig.isMasked = maskFlags[i];
        hOrig.maskingKey = maskingKeys[i];
        hOrig.opcode = opcodes[i];
        hOrig.payloadSize = payloadSizes[i];
        
        char headerBytesOrig[SN_MAX_HEADER_SIZE];
        char headerBytesRef[SN_MAX_HEADER_SIZE];
        int headerSizeOrig, headerSizeRef;
        
        snError result = snFrameHeader_toBytes(&hOrig, headerBytesOrig, &headerSizeOrig);
        if (result != SN_NO_ERROR)
        {
            sput_fail_if(1, "snFrameHeader_toBytes failed");
        }
        result = snFrameHeader_fromBytes(&hRef, headerBytesOrig, &headerSizeRef);
        if (result != SN_NO_ERROR)
        {
            sput_fail_if(1, "snFrameHeader_fromBytes failed");
        }
        
        sput_fail_unless(headerSizeRef == headerSizeOrig, "Serialized header size must match reference bytes.");
        
        if (!snFrameHeader_equals(&hOrig, &hRef))
        {
            sput_fail_if(1, "Deserialized header does not equal reference header");
        }
        //printf("Orig header (byte size %d): ", headerSizeOrig);
        //snFrameHeader_log(&hOrig);
        //printf("Ref header (byte size %d): ", headerSizeRef);
        //snFrameHeader_log(&hRef);
        //printf("\n");
    }
}

static void testFrameHeaderValidation()
{
    //TODO
}

static void testMasking()
{
    //TODO
}

#endif //SN_TEST_WEBSOCKET_FRAME_H
