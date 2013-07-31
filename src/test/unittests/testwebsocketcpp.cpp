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
    sn::Websocket* websocket;
    
    virtual void connectionStateChanged(::snReadyState state)
    {
        std::cout << "connectionStateChanged: " << state << std::endl;
        if (state == SN_STATE_OPEN)
        {
            std::string payload = "yoyoyoy";
            std::cout << "Sending " << payload << std::endl;
            websocket->sendText(payload);
        }
    }
    
    virtual void disconnected(snStatusCode statusCode)
    {
        std::cout << "disconnected" << std::endl;
    }
    
    virtual void textDataReceived(const std::string& payload, int numBytes)
    {
        std::cout << "textDataReceived: " << payload << std::endl;
    }
    
    virtual void binaryDataReceived(const std::string& payload, int numBytes)
    {
        std::cout << "binaryDataReceived" << std::endl;
    }
    
    virtual void pingReceived(const std::string& payload)
    {
        std::cout << "pingReceived" << std::endl;
    }
    
    virtual void pongReceived(const std::string& payload)
    {
        std::cout << "pongReceived" << std::endl;
    }
};

void testWebsocketCpp()
{
    Listener listener;
    sn::Websocket websocket;
    listener.websocket = &websocket;
    websocket.setListener(&listener);
    
    websocket.connect("ws://echo.websocket.org");
    
    const int pollDurationMs = 10;
    while (1)
    {
        websocket.poll();
        usleep(1000 * pollDurationMs);
    }
    
}