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

#include <iostream>
#include <unistd.h>

#include "testwebsocketcpp.h"
#include "Websocket.hpp"

class Listener : public sn::WebsocketListener
{
public:
    
    virtual void connectionStateChanged(sn::Websocket& websocket, snReadyState state)
    {
        //std::cout << "connectionStateChanged: " << state << std::endl;
        if (state == SN_STATE_OPEN)
        {
            std::string payload = "Hello text!";
            std::cout << "Opening handshake completed, sending text: " << payload << std::endl;
            m_pingCount = 0;
            websocket.sendText(payload);
        }
    }
    
    virtual void disconnected(sn::Websocket& websocket, snStatusCode statusCode)
    {
        std::cout << "Disconnected with status " << statusCode << std::endl;
    }
    
    virtual void textDataReceived(sn::Websocket& websocket, const std::string& payload, int numBytes)
    {
        std::cout << "Received text data: " << payload << std::endl;
        std::cout << "Sending ping: " << payload << std::endl;
        websocket.sendPing("Hello ping!");
    }
    
    virtual void binaryDataReceived(sn::Websocket& websocket, const std::string& payload, int numBytes)
    {
        std::cout << "binaryDataReceived" << std::endl;
    }
    
    virtual void pingReceived(sn::Websocket& websocket, const std::string& payload)
    {
        std::cout << "pingReceived" << std::endl;
    }
    
    virtual void pongReceived(sn::Websocket& websocket, const std::string& payload)
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
    
private:
    int m_pingCount;
};

void testWebsocketCppEcho()
{
    Listener listener;
    sn::Websocket websocket(&listener);
    
    websocket.connect("ws://echo.websocket.org");
    
    const int pollDurationMs = 10;
    while (websocket.getState() != SN_STATE_CLOSED)
    {
        websocket.poll();
        usleep(1000 * pollDurationMs);
    }
}

void testWebsocketCppInvalidURL()
{
    sn::Websocket websocket;
    websocket.connect("ws://echoo.websocket.abc");
}
