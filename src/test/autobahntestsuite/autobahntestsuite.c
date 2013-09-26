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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <snacka/websocket.h>
#include <snacka/logging.h>
#include <snacka/frameparser.h>


typedef struct AutobahnTestState
{
    snWebsocket* websocket;
    
    int testCount;
    
    int isFetchingCaseCount;
    
} AutobahnTestState;


void messageCallback(void* userData, snOpcode opcode, const char* data, int numBytes)
{
    AutobahnTestState* test = (AutobahnTestState*)userData;
    
    if (test->isFetchingCaseCount)
    {
        assert(opcode == SN_OPCODE_TEXT);
        test->isFetchingCaseCount = 0;
        test->testCount = (int)strtol(data, NULL, 10);
    }
    else
    {
        //echo any text or binary messages
        if (opcode == SN_OPCODE_TEXT ||
            opcode == SN_OPCODE_BINARY)
        {
            snWebsocket_sendFrame(test->websocket, opcode, numBytes, data);
        }
    }
}

/**
 * A client that connects to a fuzzingserver instance,
 * runs its tests and generates test reports as specified
 * in fuzzingserver.json.
 */
int main(int argc, const char* argv[])
{
    AutobahnTestState test;
    test.testCount = 0;
    test.isFetchingCaseCount = 1;
    
    //override the default read buffer size
    //since some autobahn tests involve
    //large payloads
    snWebsocketSettings s;
    memset(&s, 0, sizeof(snWebsocketSettings));
    s.maxFrameSize = 1 << 25;
    
    snWebsocket* ws = snWebsocket_createWithSettings(NULL, //skip open callback
                                                     messageCallback,
                                                     NULL, //skip close callback
                                                     NULL, //skip error callback
                                                     &test,
                                                     &s);
    test.websocket = ws;
    
    const int pollDurationMs = 1;
    const char* agentName = "snacka";
    const char* baseURL = "ws://localhost:9001/";
    
    //fetch test case count
    {
        char caseCountURL[1024];
        sprintf(caseCountURL, "%s%s", baseURL, "getCaseCount");
        snWebsocket_connect(ws, caseCountURL);
        
        printf("Fetching test count...\n");
        printf("----------------------\n");
        while (test.isFetchingCaseCount == 1 &&
               snWebsocket_getState(ws) != SN_STATE_CLOSED)
        {
            snWebsocket_poll(ws);
            usleep(1000 * pollDurationMs);
        }
        printf("Fetched test count %d\n", test.testCount);
        printf("\n");
    }

    //run tests
    {
        printf("Running tests...\n");
        printf("----------------------\n");
        
        for (int i = 0; i < test.testCount; i++)
        //for (int i = 246; i < 254; i++)
        {
            //form the URL of the current test and connect
            char testCaseURL[1024];
            const int testNumber = i + 1;
            sprintf(testCaseURL, "%srunCase?case=%d&agent=%s", baseURL, testNumber, agentName);
            snWebsocket_connect(ws, testCaseURL);
            
            //run the test
            printf("Running test %d/%d, %s\n", testNumber, test.testCount, testCaseURL);
            while (snWebsocket_getState(ws) != SN_STATE_CLOSED)
            {
                snWebsocket_poll(ws);
                usleep(1000 * pollDurationMs);
            }
        }
        
        printf("\n");
    }
    
    //generate reports
    {
        printf("Generating reports...\n");
        printf("----------------------\n");
        
        char updateReportsURL[1024];
        sprintf(updateReportsURL, "%supdateReports?agent=%s", baseURL, agentName);
        snWebsocket_connect(ws, updateReportsURL);
        while (snWebsocket_getState(ws) != SN_STATE_CLOSED)
        {
            snWebsocket_poll(ws);
            usleep(1000 * pollDurationMs);
        }
        
        printf("Done.\n");
    }
    
    return 0;
}
