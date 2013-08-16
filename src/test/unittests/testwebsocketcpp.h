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

#ifndef SN_TEST_WEBSOCKET_CPP_H
#define SN_TEST_WEBSOCKET_CPP_H

#include "sput.h"

#include <assert.h>
#include <iostream>
#include <unistd.h>

#include "testwebsocketcpp.h"
#include "Websocket.hpp"

class PingPongListener : public sn::WebSocketListener
{
public:
    
    virtual void onOpen(sn::WebSocket& websocket)
    {
        //std::cout << "connectionStateChanged: " << state << std::endl;
        std::string payload = "Hello text!";
        std::cout << "Opening handshake completed, sending text: " << payload << std::endl;
        m_pingCount = 0;
        websocket.sendText(payload);
    }
    
    virtual void onClose(sn::WebSocket& websocket, snStatusCode statusCode)
    {
        std::cout << "Disconnected with status " << statusCode << std::endl;
    }
    
    virtual void onMessage(sn::WebSocket& websocket,
                           snOpcode opcode,
                           const std::string& payload,
                           int payloadSize)
    {
        if (opcode == SN_OPCODE_TEXT)
        {
            std::cout << "Received text data: " << payload << std::endl;
            std::cout << "Sending ping: " << payload << std::endl;
            websocket.sendPing("Hello ping!");
        }
        else if (opcode == SN_OPCODE_PONG)
        {
            
            std::cout << "Received pong: " << payload << std::endl;
            m_pingCount++;
            if (m_pingCount < 10)
            {
                std::string pingPayload = "Hello ping!";
                std::cout << "Sending ping: " << pingPayload << std::endl;
                websocket.sendPing(pingPayload);
            }
            else
            {
                std::cout << "Sent " << m_pingCount << " pings, disconnecting." << std::endl;
                websocket.disconnect();
            }
        }
    }
    
    virtual void onError(sn::WebSocket& websocket, snError error)
    {
        
    }
    
private:
    int m_pingCount;
};

void testWebsocketCppEcho()
{
    
    PingPongListener listener;
    sn::WebSocket websocket(&listener);
    
    websocket.connect("ws://echo.websocket.org");
    
    const int pollDurationMs = 10;
    while (websocket.getState() != SN_STATE_CLOSED)
    {
        websocket.poll();
        usleep(1000 * pollDurationMs);
    }
}








class InvalidURLListener : public sn::WebSocketListener
{
public:
    
    virtual void onOpen(sn::WebSocket& websocket)
    {
        
    }
    
    virtual void onClose(sn::WebSocket& websocket, snStatusCode statusCode)
    {
        
    }
    
    virtual void onMessage(sn::WebSocket& websocket,
                           snOpcode opcode,
                           const std::string& payload,
                           int payloadSize)
    {
        
    }
    
    virtual void onError(sn::WebSocket& websocket, snError error)
    {
        std::cout << "onError: " << error << std::endl;
        assert(error == SN_SOCKET_FAILED_TO_CONNECT);
    }
};

void testWebsocketCppInvalidURLTimeout()
{
    //attempt to connect to a bogus URL
    //and wait for the attempt to time out.
    InvalidURLListener listener;
    sn::WebSocket websocket(&listener);
    websocket.connect("ws://echoo.websocket.abc");
    
    const int pollDurationMs = 10;
    while (websocket.getState() != SN_STATE_CLOSED)
    {
        websocket.poll();
        usleep(1000 * pollDurationMs);
    }
    
    std::cout << "Connection timed out" << std::endl;
}

void testWebsocketCppInvalidURLCancel()
{
    //TODO!!
    //attempt to connect to a bogus URL
    //and cancel the attempt before it times out
    InvalidURLListener listener;
    sn::WebSocket websocket(&listener);
    websocket.connect("ws://localhost:1234");
    
    const int pollDurationMs = 10;
    int msTimer = 0;
    const int cancelTimeout = 1000;
    while (websocket.getState() != SN_STATE_CLOSED)
    {
        websocket.poll();
        usleep(1000 * pollDurationMs);
        msTimer += pollDurationMs;
        if (msTimer > cancelTimeout)
        {
            std::cout << "Disconnecting after waiting for " << cancelTimeout << " ms." << std::endl;
            websocket.disconnect();
        }
    }
    
    std::cout << "Connection attempt cancelled" << std::endl;
}


#endif /*SN_TEST_WEBSOCKET_CPP_H*/



