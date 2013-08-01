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
#include <string.h>

#include "websocket.h"
#include "frameparser.h"
#include <unistd.h>

static void messageCallback(void* userData, snOpcode opcode, const char* data, int numBytes)
{
    if (opcode == SN_OPCODE_TEXT)
    {
        printf("Received text '%s'\n", data);
    }
    else if (opcode == SN_OPCODE_PONG)
    {
        printf("Received pong with payload '%s'\n", data);
    }
}

/**
 * A simple client sending messages to an echo server
 * and printing any received messages.
 */
int main(int argc, const char* argv[])
{
    //create a websocket with default settings
    snWebsocket* ws = snWebsocket_create(NULL, messageCallback, NULL, NULL, NULL);
    
    //if zero, this client will connect to a server
    //started by running in echoserver.py, otherwise the client
    //will try to connect to echo.websocket.org.
    const int localHost = 0;
    
    const char* echoServerURL = localHost ? "ws://localhost:9000" : "ws://echo.websocket.org";
    
    snError connectionResult = snWebsocket_connect(ws, echoServerURL);
    
    if (connectionResult != SN_NO_ERROR)
    {
        printf("Failed to connect to %s\n", echoServerURL);
        return 0;
    }
    
    const int sleepDurationMs = 10;
    const int sendIntervalMs = 1000;
    const int numFramesToSend = 5;
    int sentFrameCount = 0;
    int sendTimer = 0;
    int sendText = 0;

    while (snWebsocket_getState(ws) != SN_STATE_CLOSED)
    {
        sendTimer += sleepDurationMs;
        
        if (snWebsocket_getState(ws) == SN_STATE_OPEN && sendTimer >= sendIntervalMs)
        {
            sendTimer = 0;
            
            char payload[256];
            sprintf(payload, "Frame %d payload.", sentFrameCount + 1);
            
            if (sendText)
            {
                snWebsocket_sendPing(ws, strlen(payload), payload);
                printf("Sent ping with payload '%s'\n", payload);
            }
            else
            {
                snWebsocket_sendTextData(ws, payload);
                printf("Sent text '%s'\n", payload);
            }
            
            sendText = !sendText;
            sentFrameCount++;
            
            if (sentFrameCount == numFramesToSend)
            {
                //start performing closing handshake
                snWebsocket_disconnect(ws, 0);
            } 
        }
        
        usleep(1000 * sleepDurationMs);
        
        snWebsocket_poll(ws);
    }
    
    snWebsocket_delete(ws);

    return 0;
}
