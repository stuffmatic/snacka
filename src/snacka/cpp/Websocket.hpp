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

#ifndef SN_WEBSOCKET_HPP
#define SN_WEBSOCKET_HPP

#include <string>
#include <vector>

#include "../../external/tinycthread/tinycthread.h"
#include "../websocket.h"
#include "WebsocketListener.hpp"


namespace sn
{
    
    /**
     * An asynchronous websocket handling I/O in a separate thread.
     */
    class WebSocket
    {
    public:
        
        /**
         * Constructor.
         * @param listener A listener to pass events to.
         */
        WebSocket(WebSocketListener* listener = 0);
        
        /**
         * Destructor.
         */
        ~WebSocket();
        
        /**
         * Fetches incoming events from the I/O thread and
         * passes them on to the listsener, if any. Call this
         * method continually.
         */
        void poll();
        
        /**
         * Connects to a given URL. 
         * @param url The URL to connect to.
         */
        void connect(const std::string& url);
        
        /**
         * Closes the current connection, if any.
         */
        void disconnect();
        
        /** 
         * Returns the connection state.
         * @return The connection state.
         */
        snReadyState getState() const;
        
        /** 
         * Checks if the websocket is open, i.e has completed the 
         * opening handshake.
         * @return True if the connection is open, false otherwise.
         */
        bool isOpen() const;
        
        /**
         * Sends a text frame.
         * @param payload The UTF-8 string to send.
         */
        void sendText(const std::string& payload);
        
        /**
         * Sends a ping frame.
         * @param payload Optional payload data.
         */
        void sendPing(const std::string& payload = "");
        
        /**
         * Returns the current host or an empty string
         * if there is no connection.
         */
        const std::string& getURL() const;
        
        /**
         * Sets the listener to pass events to.
         * @param listener The listener.
         */
        void setListener(WebSocketListener* listener);
        
    private:
        
        /**
         * Used to pass frames from the I/O thread
         * to the main thread.
         */
        class WebSocketFrame
        {
        public:
            WebSocketFrame() :
            opcode(),
            payload(),
            payloadSize(0)
            {
                
            }
            
            ::snOpcode opcode;
            std::string payload;
            int payloadSize;
        };
        
        /**
         * Used to pass events from the I/O thread
         * to the main thread.
         */
        class WebSocketEvent
        {
            public:
            
            enum EventType
            {
                EVENT_INVALID = -1,
                EVENT_OPEN = 0,
                EVENT_CLOSE,
                EVENT_ERROR
            };
            
            WebSocketEvent() :
            type(EVENT_INVALID),
            statusCode(SN_STATUS_NORMAL_CLOSURE),
            readyState(SN_STATE_CLOSED),
            errorCode(SN_NO_ERROR)
            {
            
            }
            
            EventType type;
            snError errorCode;
            snStatusCode statusCode;
            snReadyState readyState;
        };
        
        void resetState();
        
        void sendEvent(const WebSocketEvent& event);
        
        void disconnectSocketThread();
        
        void sendEnqueuedFrames();
        
        //called from the main thread
        void enqueueOutgoingFrame(snOpcode opcode, const char* bytes, int numBytes);
    
        //called from the run loop
        void enqueueIncomingFrame(snOpcode opcode, const char* bytes, int numBytes);
        
        friend int websocketRunLoopEntryPoint(void* data);
        
        friend int ioCancelCallback(void* data);
    
        friend void messageCallback(void* data, snOpcode opcode, const char* bytes, int numBytes);
    
        friend void openCallback(void* data);
    
        friend void closeCallback(void* data, snStatusCode status);
    
        friend void errorCallback(void* data, snError error);
    
        bool shouldStopRunLoop();
        
        void sleep(int ms);
        
        static const int SLEEP_DURATION_MS = 10;
        
        void requestRunLoopStop();
        
        bool m_shouldStopRunloop;
    
        std::string m_url;
    
        thrd_t m_socketRunLoop;
        
        mtx_t m_flagMutex;
        
        snReadyState m_readyState;
    
        snWebsocket* m_websocket;
        
        mtx_t m_toSocketQueueMutex;
        
        std::vector<WebSocketFrame> m_toSocketQueue;
        
        mtx_t m_fromSocketQueueMutex;
        
        std::vector<WebSocketFrame> m_fromSocketQueue;
        
        std::vector<WebSocketEvent> m_eventQueue;
        
        WebSocketListener* m_listener;
    };
    
} //namespace sn

#endif /*SN_WEBSOCKET_HPP*/



