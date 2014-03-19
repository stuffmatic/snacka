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

#ifndef SN_TEST_CONNECTION_STATE_H
#define SN_TEST_CONNECTION_STATE_H

#include <assert.h>
#include <string.h>

#include "sput.h"
#include "websocket.h"

static void errorCallback(void* userData, snError errorCode)
{
    printf("Websocket error, code %d\n", errorCode);
}

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


static void testConnectionState()
{
    {
        const char* bogusURL = "htttp://lol.face";
        snWebsocket* ws = snWebsocket_create(NULL, messageCallback, NULL, errorCallback, NULL);
        snError connectionResult = snWebsocket_connect(ws, bogusURL);
        sput_fail_unless(connectionResult == SN_SOCKET_FAILED_TO_CONNECT,
                         "Bogus URL should result in a socket connect failed error");
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CLOSED,
                         "Websocket should be in the 'not connected' state immediately after socket connect error");
        snWebsocket_disconnect(ws, 1);
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CLOSED,
                         "Disconnected websocket should remain in the 'not connected' state after additional disconnect calls");
        snWebsocket_delete(ws);
    }
    
    /* */
    {
        const char* workingURL = "ws://echo.websocket.org";
        snWebsocket* ws = snWebsocket_create(NULL, messageCallback, NULL, errorCallback, NULL);
        snError connectionResult = snWebsocket_connect(ws, workingURL);
        sput_fail_unless(connectionResult == SN_NO_ERROR,
                         "Connecting to valid URL should return SN_NO_ERROR");
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CONNECTING,
                         "Websocket should be in the 'connecting' state immediately after connect to valid URL");
        snWebsocket_disconnect(ws, 1);
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CLOSED,
                         "Disconnected websocket should be in the 'not connected' state after disconnect");
        snWebsocket_delete(ws);
    }
    
    /* */
    {
        const char* workingURL = "ws://echo.websocket.org";
        snWebsocket* ws = snWebsocket_create(NULL, messageCallback, NULL, errorCallback, NULL);
        snError connectionResult = snWebsocket_connect(ws, workingURL);
        sput_fail_unless(connectionResult == SN_NO_ERROR,
                         "Connecting to valid URL should return SN_NO_ERROR");
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CONNECTING,
                         "Websocket should be in the 'connecting' state immediately after connect to valid URL");
        
        while (snWebsocket_getState(ws) == SN_STATE_CONNECTING)
        {
            snWebsocket_poll(ws);
        }
        
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_OPEN,
                         "Websocket should transition from 'connecting' to 'open' state after connect to valid URL");
        snWebsocket_disconnect(ws, 1);
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CLOSED,
                         "Open websocket should be in the 'not connected' state after disconnect");
        snWebsocket_delete(ws);
    }
    
    /*Try deleting open websocket. */
    {
        const char* workingURL = "ws://echo.websocket.org";
        snWebsocket* ws = snWebsocket_create(NULL, messageCallback, NULL, errorCallback, NULL);
        snError connectionResult = snWebsocket_connect(ws, workingURL);
        sput_fail_unless(connectionResult == SN_NO_ERROR,
                         "Connecting to valid URL should return SN_NO_ERROR");
        sput_fail_unless(snWebsocket_getState(ws) == SN_STATE_CONNECTING,
                         "Websocket should be in the 'connecting' state immediately after connect to valid URL");
        snWebsocket_delete(ws);
    }
}



#endif /*SN_TEST_CONNECTION_STATE_H*/
