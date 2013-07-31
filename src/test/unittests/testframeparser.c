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

#include "websocket.h"
#include "frameparser.h"
#include "testframeparser.h"

static void frameCallback(void* userData, const snFrame* frame)
{
    
}

void testFrameParser()
{
    const int bufferSize = 1 << 10;
    char buffer[bufferSize];
    snFrameParser p;
    snFrameParser_init(&p, frameCallback, NULL, NULL, NULL, buffer, bufferSize);
    
    
    const int numCases = 5;
    int maskFlags[numCases] = {0, 1, 1, 1, 1};
    int maskingKeys[numCases] = {0, 1, 4000, 12399, 9999};
    int finalFlags[numCases] = {0, 1, 1, 1, 0};
    unsigned long long payloadSizes[numCases] = {1 << 2, (1 << 16) - 1, (1 << 16) + 1, 126, 127};
    int opcodes[numCases] =
    {
        SN_OPCODE_TEXT,
        SN_OPCODE_BINARY,
        SN_OPCODE_TEXT,
        SN_OPCODE_TEXT,
        SN_OPCODE_TEXT
    };
    
    for (int i = 0; i < numCases; i++)
    {
        snFrameParser_reset(&p);
        
        snFrameHeader h;
        h.maskingKey = maskingKeys[i];
        h.isMasked = maskFlags[i];
        h.isFinal = finalFlags[i];
        h.opcode = opcodes[i];
        h.payloadSize = payloadSizes[i];
        
        char headerBytes[SN_MAX_HEADER_SIZE];
        int size = 0;
        snFrameHeader_toBytes(&h, headerBytes, &size);
        printf("parsing frame ");
        snFrameHeader_log(&h);
        for (int j = 0; j < size; j++)
        {
            snFrameParser_processBytes(&p, &headerBytes[j], 1);
        }
    }
    
    snFrameParser_deinit(&p);
}