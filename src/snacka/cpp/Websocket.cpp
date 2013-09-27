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

#include "websocket.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>

#include "Websocket.hpp"

using namespace sn;

static int threadCount = 0;

namespace sn
{
    void openCallback(void* data)
    {
        WebSocket* ws = (WebSocket*)data;
        WebSocket::WebSocketEvent e;
        e.type = WebSocket::WebSocketEvent::EVENT_OPEN;
        ws->sendEvent(e);
    }
    
    void messageCallback(void* data, snOpcode opcode, const char* bytes, int numBytes)
    {
        WebSocket* ws = (WebSocket*)data;
        ws->enqueueIncomingFrame(opcode, bytes, numBytes);
    }
    
    void closeCallback(void* data, snStatusCode closeCode)
    {
        WebSocket* ws = (WebSocket*)data;
        WebSocket::WebSocketEvent e;
        e.type = WebSocket::WebSocketEvent::EVENT_CLOSE;
        e.statusCode = closeCode;
        ws->sendEvent(e);
    }
    
    void errorCallback(void* data, snError error)
    {
        WebSocket* ws = (WebSocket*)data;
        WebSocket::WebSocketEvent e;
        e.type = WebSocket::WebSocketEvent::EVENT_ERROR;
        e.errorCode = error;
        ws->sendEvent(e);
    }
    
    int ioCancelCallback(void* data)
    {
        WebSocket* ws = (WebSocket*)data;
        int ret = ws->shouldStopRunLoop() ? 0 : 1;
        
        return ret;
    }
    
    int websocketRunLoopEntryPoint(void* data)
    {
        //printf("websocketRunLoopEntryPoint: entering\n");
        threadCount++;
        //assert(threadCount == 1);
        
        const int pollIntervalMs = 10;
        
        WebSocket* ws = (WebSocket*)data;
        
        snWebsocket* cws = ws->m_websocket;
        
        snError connectionResult = snWebsocket_connect(cws, ws->getURL().c_str());
        
        if (connectionResult != SN_NO_ERROR)
        {
            WebSocket::WebSocketEvent e;
            e.type = WebSocket::WebSocketEvent::EVENT_ERROR;
            e.errorCode = connectionResult;
            ws->sendEvent(e);
        }
        else
        {
            while (!ws->shouldStopRunLoop() && snWebsocket_getState(cws) != SN_STATE_CLOSED)
            {
                ws->sendEnqueuedFrames();
                snWebsocket_poll(cws);
                usleep(1000 * pollIntervalMs);
            }
            
            if (snWebsocket_getState(cws) != SN_STATE_CLOSED)
            {
                snWebsocket_disconnect(cws, 1);
            }
        }
    
        threadCount--;
        //assert(threadCount == 0);
        
        return 0;
    }
    
} //namspace sn


WebSocket::WebSocket(WebSocketListener* listener) :
m_url(),
m_listener(listener),
m_socketRunLoop(0),
m_shouldStopRunloop(false),
m_flagMutex(),
m_readyState(SN_STATE_CLOSED),
m_websocket(0),
m_toSocketQueueMutex(),
m_toSocketQueue(),
m_fromSocketQueueMutex(),
m_fromSocketQueue(),
m_eventQueue()
{
    mtx_init(&m_flagMutex, mtx_plain);
    mtx_init(&m_toSocketQueueMutex, mtx_plain);
    mtx_init(&m_fromSocketQueueMutex, mtx_plain);
    
    snWebsocketSettings settings;
    memset(&settings, 0, sizeof(snWebsocketSettings));
    settings.cancelCallback = ioCancelCallback;
    
    m_websocket = snWebsocket_createWithSettings(openCallback,
                                                 messageCallback,
                                                 closeCallback,
                                                 errorCallback,
                                                 this,
                                                 &settings);
    
    resetState();
}

WebSocket::~WebSocket()
{
    mtx_destroy(&m_flagMutex);
    mtx_destroy(&m_toSocketQueueMutex);
    mtx_destroy(&m_fromSocketQueueMutex);
    
    snWebsocket_delete(m_websocket);
}

void WebSocket::poll()
{
    std::vector<WebSocketFrame> frames;
    std::vector<WebSocketEvent> events;
    
    mtx_lock(&m_fromSocketQueueMutex);
    frames = m_fromSocketQueue;
    m_fromSocketQueue.clear();
    events = m_eventQueue;
    m_eventQueue.clear();
    mtx_unlock(&m_fromSocketQueueMutex);
    
    for (int i = 0; i < frames.size(); i++)
    {
        WebSocketFrame& framei = frames[i];
        
        if (m_listener == NULL)
        {
            continue;
        }
        
        switch (framei.opcode)
        {
            case SN_OPCODE_TEXT:
            case SN_OPCODE_BINARY:
            case SN_OPCODE_PING:
            case SN_OPCODE_PONG:
            {
                m_listener->onMessage(*this, framei.opcode, framei.payload, framei.payloadSize);
                break;
            }
            default:
            {
                assert(false && "invalid opcode");
                break;
            }
        }
    }
    
    //first send notification that are not errors,
    //so that the websocket state is up to date
    //when receiving error notifactions.
    for (int pass = 0; pass < 2; pass++)
    {
        const bool errorPass = pass == 1;
        
        for (int i = 0; i < events.size(); i++)
        {
            const WebSocketEvent& e = events[i];
            
            if (m_listener == NULL)
            {
                continue;
            }
            
            if (!errorPass)
            {
                if (e.type == WebSocketEvent::EVENT_OPEN)
                {
                    m_readyState = SN_STATE_OPEN;
                    m_listener->onOpen(*this);
                }
                else if (e.type == WebSocketEvent::EVENT_CLOSE)
                {
                    m_readyState = SN_STATE_CLOSED;
                    m_listener->onClose(*this, e.statusCode);
                }
            }
            else
            {
                if (e.type == WebSocketEvent::EVENT_ERROR)
                {
                    m_listener->onError(*this, e.errorCode);
                }
            }
        }
    }
}

bool WebSocket::shouldStopRunLoop()
{
    return m_shouldStopRunloop;
}

void WebSocket::requestRunLoopStop()
{
    m_shouldStopRunloop = true;
}

void WebSocket::connect(const std::string& url)
{
    disconnect();
    
    resetState();
    
    m_readyState = SN_STATE_CONNECTING;
    
    m_url = url;
    
    thrd_create(&m_socketRunLoop, websocketRunLoopEntryPoint, this);
}

void WebSocket::disconnect()
{
    if (m_socketRunLoop != NULL)
    {
        requestRunLoopStop();
        thrd_join(m_socketRunLoop, NULL);
        m_socketRunLoop = 0;
        
        //flush any pending frames and events
        poll();
    }
}

snReadyState WebSocket::getState() const
{
    return m_readyState;
}

bool WebSocket::isOpen() const
{
    return m_readyState == SN_STATE_OPEN;
}

void WebSocket::sendText(const std::string& payload)
{
    enqueueOutgoingFrame(SN_OPCODE_TEXT, payload.c_str(), payload.length());
}

void WebSocket::enqueueOutgoingFrame(snOpcode opcode, const char* bytes, int numBytes)
{
    WebSocketFrame frame;
    frame.opcode = opcode;
    frame.payloadSize = numBytes;
    frame.payload = std::string(bytes);
    
    mtx_lock(&m_toSocketQueueMutex);
    m_toSocketQueue.push_back(frame);
    mtx_unlock(&m_toSocketQueueMutex);
}

void WebSocket::enqueueIncomingFrame(snOpcode opcode, const char* bytes, int numBytes)
{
    WebSocketFrame frame;
    frame.opcode = opcode;
    frame.payloadSize = numBytes;
    frame.payload = std::string(bytes);
    
    mtx_lock(&m_fromSocketQueueMutex);
    m_fromSocketQueue.push_back(frame);
    mtx_unlock(&m_fromSocketQueueMutex);
}

void WebSocket::sendPing(const std::string& payload)
{
    enqueueOutgoingFrame(SN_OPCODE_PING, payload.c_str(), payload.length());
}

void WebSocket::sendEnqueuedFrames()
{
    mtx_lock(&m_toSocketQueueMutex);
    
    for (int i = 0; i < m_toSocketQueue.size(); i++)
    {
        WebSocketFrame& framei = m_toSocketQueue[i];
        snWebsocket_sendFrame(m_websocket,
                              framei.opcode,
                              framei.payloadSize,
                              framei.payload.c_str());
    }
    
    m_toSocketQueue.clear();
    
    mtx_unlock(&m_toSocketQueueMutex);
}

void WebSocket::setListener(WebSocketListener* listener)
{
    m_listener = listener;
}

const std::string& WebSocket::getURL() const
{
    return m_url;
}

void WebSocket::resetState()
{
    m_readyState = SN_STATE_CLOSED;
    m_shouldStopRunloop = false;
    m_url.clear();
    m_fromSocketQueue.clear();
    m_toSocketQueue.clear();
}

void WebSocket::sendEvent(const WebSocketEvent& event)
{

    mtx_lock(&m_fromSocketQueueMutex);
    m_eventQueue.push_back(event);
    mtx_unlock(&m_fromSocketQueueMutex);
}

