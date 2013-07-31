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

#ifndef SN_WEBSOCKET_HPP
#define SN_WEBSOCKET_HPP

#include <string>
#include <vector>

#include "tinycthread.h"
#include "websocket.h"

namespace sn
{
    
    class WebsocketListener;
    
    /**
     * 
     */
    class Websocket
    {
    public:
        
        /**
         *
         */
        Websocket();
        
        /**
         *
         */
        ~Websocket();
        
        /**
         * Call continually.
         */
        void poll();
        
        /**
         *
         */
        void connect(const std::string& url);
        
        /**
         *
         */
        void disconnect();
        
        /**
         *
         */
        void sendText(const std::string& payload);
        
        /**
         *
         */
        void sendPing(const std::string& payload);
        
        /**
         *
         */
        void sendPong(const std::string& payload);
        
        /**
         *
         */
        const std::string& getURL() const;
        
        /**
         *
         */
        void setListener(WebsocketListener* listener);
        
    private:
        
        /**
         *
         */
        class WebsocketFrame
        {
        public:
            WebsocketFrame() :
            opcode(),
            payload(),
            payloadSize(0)
            {
                
            }
            
            ::snOpcode opcode;
            std::string payload;
            int payloadSize;
        };
        
        void resetState();
        
        void sendNotification(::snReadyState state, int statusCode);
        
        void disconnectSocketThread();
        
        void sendEnqueuedFrames();
        
        //called from the main thread
        void enqueueOutgoingFrame(snOpcode opcode, const char* bytes, int numBytes);
    
        //called from the run loop
        void enqueueIncomingFrame(snOpcode opcode, const char* bytes, int numBytes);
        
        friend int websocketRunLoopEntryPoint(void* data);
        
        friend int ioCancelCallback(void* data);
    
        friend void messageCallback(void* data, snOpcode opcode, const char* bytes, int numBytes);
    
        friend void stateCallback(void* data, snReadyState state, int statusCode);
    
        bool shouldStopRunLoop();
        
        void sleep(int ms);
        
        static const int SLEEP_DURATION_MS = 10;
        
        //void sendConnectionClose();
        
        void requestRunLoopStop();
    
        std::string m_url;
    
        ::thrd_t m_socketRunLoop;
        
        ::mtx_t m_flagMutex;
        
        ::snWebsocket* m_websocket;
        
        ::mtx_t m_toSocketQueueMutex;
        
        std::vector<WebsocketFrame> m_toSocketQueue;
        
        ::mtx_t m_fromSocketQueueMutex;
        
        std::vector<WebsocketFrame> m_fromSocketQueue;
        
        std::vector<snReadyState> m_notificationQueue;
        
        WebsocketListener* m_listener;
    };
    
    /**
     *
     */
    class WebsocketListener
    {
    public:
    
        /**
         *
         */
        virtual void connectionStateChanged(::snReadyState state) = 0;
        
        /**
         *
         */
        virtual void disconnected(snStatusCode statusCode) = 0;
        
        /**
         *
         */
        virtual void textDataReceived(const std::string& payload, int numBytes) = 0;
        
        /**
         *
         */
        virtual void binaryDataReceived(const std::string& payload, int numBytes) = 0;
        
        /**
         *
         */
        virtual void pingReceived(const std::string& payload) = 0;
        
        /**
         *
         */
        virtual void pongReceived(const std::string& payload) = 0;
        
    };
    
} //namespace sn

#endif /*SN_WEBSOCKET_HPP*/



