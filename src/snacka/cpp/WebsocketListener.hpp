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

/*! \file */

#ifndef SN_WEBSOCKET_LISTENER_HPP
#define SN_WEBSOCKET_LISTENER_HPP

#include <string>
#include <vector>

#include "../websocket.h"


namespace sn
{
    
    class WebSocket;
    
    /**
     * Receives data and state change notifications from a Websocket instance.
     */
    class WebSocketListener
    {
    public:
        
        /**
         * Called when the websocket connection state changes. 
         * @param websocket The websocket associated with the listener.
         * @param state The new connection state.
         */
        virtual void connectionStateChanged(WebSocket& websocket, snReadyState state) = 0;
        
        /**
         * Called when the websocket connection has been closed.
         * @param websocket The websocket associated with the listener.
         * @param statusCode A status code.
         */
        virtual void disconnected(WebSocket& websocket, snStatusCode statusCode) = 0;
        
        /**
         * Called when a text message has been received.
         * @param websocket The websocket associated with the listener.
         * @param payload The UTF-8 string payload.
         * @param numBytes The payload size in bytes, excluding the terminating null character.
         */
        virtual void textDataReceived(WebSocket& websocket, const std::string& payload, int numBytes) = 0;
        
        /**
         * Called when a binary message has been received.
         * @param websocket The websocket associated with the listener.
         * @param payload The binary payload.
         * @param numBytes The payload size in bytes.
         */
        virtual void binaryDataReceived(WebSocket& websocket, const std::string& payload, int numBytes) = 0;
        
        /**
         * Called when a ping frame has been received.
         * @param websocket The websocket associated with the listener.
         * @param payload The ping payload.
         */
        virtual void pingReceived(WebSocket& websocket, const std::string& payload) = 0;
        
        /**
         * Called when a pong frame has been received.
         * @param websocket The websocket associated with the listener.
         * @param payload The pong payload.
         */
        virtual void pongReceived(WebSocket& websocket, const std::string& payload) = 0;
        
    };
    
} //namespace sn

#endif /*SN_WEBSOCKET_LISTENER_HPP*/



