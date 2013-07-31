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
    void messageCallback(void* data, snOpcode opcode, const char* bytes, int numBytes)
    {
        Websocket* ws = (Websocket*)data;
        ws->enqueueIncomingFrame(opcode, bytes, numBytes);
    }
    
    void stateCallback(void* data, snReadyState state, int statusCode)
    {
        Websocket* ws = (Websocket*)data;
        ws->sendNotification(state, (snStatusCode)statusCode); //TODO: don't cast here
    }
    
    int ioCancelCallback(void* data)
    {
        Websocket* ws = (Websocket*)data;
        int ret = ws->shouldStopRunLoop() ? 0 : 1;
        
        return ret;
    }
    
    int websocketRunLoopEntryPoint(void* data)
    {
        //printf("websocketRunLoopEntryPoint: entering\n");
        threadCount++;
        assert(threadCount == 1);
        
        const int pollIntervalMs = 10;
        
        Websocket* ws = (Websocket*)data;
        
        snWebsocket* cws = ws->m_websocket;
        
        snError connectionResult = snWebsocket_connect(cws, ws->getURL().c_str());
        
        if (connectionResult != SN_NO_ERROR)
        {
            return 0;
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
        assert(threadCount == 0);
        
        
        return 0;
    }
    
} //namspace sn


Websocket::Websocket(WebsocketListener* listener) :
m_url(),
m_listener(listener)
{
    mtx_init(&m_flagMutex, mtx_plain);
    mtx_init(&m_toSocketQueueMutex, mtx_plain);
    mtx_init(&m_fromSocketQueueMutex, mtx_plain);
    
    snWebsocketSettings settings;
    memset(&settings, 0, sizeof(snWebsocketSettings));
    settings.cancelCallback = ioCancelCallback;
    
    m_websocket = snWebsocket_createWithSettings(stateCallback,
                                                 messageCallback,
                                                 this,
                                                 &settings);
    
    resetState();
}

Websocket::~Websocket()
{
    mtx_destroy(&m_flagMutex);
    mtx_destroy(&m_toSocketQueueMutex);
    mtx_destroy(&m_fromSocketQueueMutex);
    
    snWebsocket_delete(m_websocket);
}

void Websocket::poll()
{
    std::vector<WebsocketFrame> frames;
    std::vector<WebsocketEvent> notifications;
    
    mtx_lock(&m_fromSocketQueueMutex);
    frames = m_fromSocketQueue;
    m_fromSocketQueue.clear();
    notifications = m_notificationQueue;
    m_notificationQueue.clear();
    mtx_unlock(&m_fromSocketQueueMutex);
    
    for (int i = 0; i < frames.size(); i++)
    {
        WebsocketFrame& framei = frames[i];
        
        if (m_listener == NULL)
        {
            continue;
        }
        
        switch (framei.opcode)
        {
            case SN_OPCODE_TEXT:
            {
                m_listener->textDataReceived(*this, framei.payload, framei.payloadSize);
                break;
            }
            case SN_OPCODE_BINARY:
            {
                m_listener->binaryDataReceived(*this, framei.payload, framei.payloadSize);
                break;
            }
            case SN_OPCODE_PING:
            {
                m_listener->pingReceived(*this, framei.payload);
                break;
            }
            case SN_OPCODE_PONG:
            {
                m_listener->pongReceived(*this, framei.payload);
                break;
            }
            default:
            {
                assert(false && "invalid opcode");
                break;
            }
        }
    }
    
    for (int i = 0; i < notifications.size(); i++)
    {
        const WebsocketEvent& e = notifications[i];
        
        m_readyState = e.readyState;
        
        if (m_listener == NULL)
        {
            continue;
        }

        if (e.readyState == SN_STATE_CLOSED)
        {
            m_listener->disconnected(*this, e.statusCode); //TODO: pass status code

        }
        else
        {
            m_listener->connectionStateChanged(*this, e.readyState);
        }
        
    }
}

bool Websocket::shouldStopRunLoop()
{
    return m_shouldStopRunloop;
}

void Websocket::requestRunLoopStop()
{
    m_shouldStopRunloop = true;
}

void Websocket::connect(const std::string& url)
{
    disconnect();
    
    resetState();
    
    m_readyState = SN_STATE_CONNECTING;
    m_url = url;
    
    thrd_create(&m_socketRunLoop, websocketRunLoopEntryPoint, this);
}

void Websocket::disconnect()
{
    if (m_socketRunLoop != NULL)
    {
        requestRunLoopStop();
        thrd_join(m_socketRunLoop, NULL);
        m_socketRunLoop = 0;
        
        //flush any pending frames and notifications
        poll();
    }
}

snReadyState Websocket::getState() const
{
    return m_readyState;
}

void Websocket::sendText(const std::string& payload)
{
    enqueueOutgoingFrame(SN_OPCODE_TEXT, payload.c_str(), payload.length());
}

void Websocket::enqueueOutgoingFrame(snOpcode opcode, const char* bytes, int numBytes)
{
    WebsocketFrame frame;
    frame.opcode = opcode;
    frame.payloadSize = numBytes;
    frame.payload = std::string(bytes);
    
    mtx_lock(&m_toSocketQueueMutex);
    m_toSocketQueue.push_back(frame);
    mtx_unlock(&m_toSocketQueueMutex);
}

void Websocket::enqueueIncomingFrame(snOpcode opcode, const char* bytes, int numBytes)
{
    WebsocketFrame frame;
    frame.opcode = opcode;
    frame.payloadSize = numBytes;
    frame.payload = std::string(bytes);
    
    mtx_lock(&m_fromSocketQueueMutex);
    m_fromSocketQueue.push_back(frame);
    mtx_unlock(&m_fromSocketQueueMutex);
}

void Websocket::sendPing(const std::string& payload)
{
    enqueueOutgoingFrame(SN_OPCODE_PING, payload.c_str(), payload.length());
}

void Websocket::sendPong(const std::string& payload)
{
    enqueueOutgoingFrame(SN_OPCODE_PONG, payload.c_str(), payload.length());
}

void Websocket::sendEnqueuedFrames()
{
    mtx_lock(&m_toSocketQueueMutex);
    
    for (int i = 0; i < m_toSocketQueue.size(); i++)
    {
        WebsocketFrame& framei = m_toSocketQueue[i];
        snWebsocket_sendFrame(m_websocket,
                              framei.opcode,
                              framei.payloadSize,
                              framei.payload.c_str());
    }
    
    m_toSocketQueue.clear();
    
    mtx_unlock(&m_toSocketQueueMutex);
}

void Websocket::setListener(WebsocketListener* listener)
{
    m_listener = listener;
}

const std::string& Websocket::getURL() const
{
    return m_url;
}

void Websocket::resetState()
{
    m_readyState = SN_STATE_CLOSED;
    m_shouldStopRunloop = false;
    m_url.clear();
    m_fromSocketQueue.clear();
    m_toSocketQueue.clear();
}

void Websocket::sendNotification(snReadyState state, snStatusCode statusCode)
{
    WebsocketEvent e;
    e.readyState = state;
    e.statusCode = statusCode;
    mtx_lock(&m_fromSocketQueueMutex);
    m_notificationQueue.push_back(e);
    mtx_unlock(&m_fromSocketQueueMutex);
}

