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

#ifndef SN_WEBSOCKET_H
#define SN_WEBSOCKET_H

/*! \file
 
 http://www.w3.org/TR/2011/WD-websockets-20110419/
 https://tools.ietf.org/html/rfc6455
 
 */

#include "errorcodes.h"
#include "frame.h"
#include "iocallbacks.h"
#include "logging.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    
    typedef struct snWebsocket snWebsocket;

    /**
     * @name Constants
     */
    /** @{ */
    
    /**
     * Websocket ready states.
     * @see http://www.w3.org/TR/2011/WD-websockets-20110419/#the-websocket-interface
     */
    typedef enum snReadyState
    {
        /** The connection is in progress and has not yet been established. */
        SN_STATE_CONNECTING = 0,
        /** The WebSocket connection is established and communication is possible. */
        SN_STATE_OPEN = 1,
        /** The connection is going through the closing handshake. */
        SN_STATE_CLOSING = 2,
        /** The connection has been closed or could not be opened. */
        SN_STATE_CLOSED = 3
    } snReadyState;
    
    /**
     * Websocket status codes.
     * @see https://tools.ietf.org/html/rfc6455#section-7.4.1
     */
    typedef enum snStatusCode
    {
        /**
         * Indicates a normal closure, meaning that the purpose for
         * which the connection was established has been fulfilled.
         */
        SN_STATUS_NORMAL_CLOSURE = 1000,
        /**
         * Indicates that an endpoint is "going away", such as a server
         * going down or a browser having navigated away from a page.
         */
        SN_STATUS_ENDPOINT_GOING_AWAY = 1001,
        /**
         * Indicates that an endpoint is terminating the connection due
         * to a protocol error.
         */
        SN_STATUS_PROTOCOL_ERROR = 1002,
        /**
         * Indicates that an endpoint is terminating the connection
         * because it has received a type of data it cannot accept (e.g., an
         * endpoint that understands only text data MAY send this if it
         * receives a binary message).
         */
        SN_STATUS_INVALID_DATA = 1003,
        /**
         * Reserved. The specific meaning might be defined in the future.
         */
        SN_STATUS_RESERVED_1 = 1004,
        /**
         * A reserved value and MUST NOT be set as a status code in a
         * Close control frame by an endpoint.  It is designated for use in
         * applications expecting a status code to indicate that no status
         * code was actually present.
         */
        SN_STATUS_RESERVED_2 = 1005,
        /**
         * A reserved value and MUST NOT be set as a status code in a
         * Close control frame by an endpoint.  It is designated for use in
         * applications expecting a status code to indicate that the
         * connection was closed abnormally, e.g., without sending or
         * receiving a Close control frame.
         */
        SN_STATUS_RESERVED_3 = 1006,
        /**
         * Indicates that an endpoint is terminating the connection
         * because it has received data within a message that was not
         * consistent with the type of the message (e.g., non-UTF-8 [RFC3629]
         * data within a text message).
         */
        SN_STATUS_INCONSISTENT_DATA = 1007,
        /**
         * Indicates that an endpoint is terminating the connection
         * because it has received a message that violates its policy.  This
         * is a generic status code that can be returned when there is no
         * other more suitable status code (e.g., 1003 or 1009) or if there
         * is a need to hide specific details about the policy.
         */
        SN_STATUS_POLICY_VIOLATION = 1008,
        /**
         * Indicates that an endpoint is terminating the connection
         * because it has received a message that is too big for it to
         * process.
         */
        SN_STATUS_MESSAGE_TOO_BIG = 1009,
        /**
         * Indicates that an endpoint (client) is terminating the
         * connection because it has expected the server to negotiate one or
         * more extension, but the server didn't return them in the response
         * message of the WebSocket handshake.  The list of extensions that
         * are needed SHOULD appear in the /reason/ part of the Close frame.
         * Note that this status code is not used by the server, because it
         * can fail the WebSocket handshake instead.
         */
        SN_STATUS_MISSING_EXTENSION = 1010,
        /**
         * Indicates that a server is terminating the connection because
         * it encountered an unexpected condition that prevented it from
         * fulfilling the request.
         */
        SN_STATUS_UNEXPECTED_ERROR = 1011,
        /**
         * Is a reserved value and MUST NOT be set as a status code in a
         * Close control frame by an endpoint.  It is designated for use in
         * applications expecting a status code to indicate that the
         * connection was closed due to a failure to perform a TLS handshake
         * (e.g., the server certificate can't be verified).
         */
        SN_STATUS_RESERVED_4 = 1015
        
    } snStatusCode;
    
    /** @} */
    
    /**
     * @name Callbacks
     */
    /** @{ */
    
    /**
     * Called when a new incoming frame is available,
     * including non-final continuation frames. Useful
     * for debugging.
     * @param userData Custom user data.
     * @param frame The received frame.
     */
    typedef void (*snFrameCallback)(void* userData, const snFrame* frame);
    
    /**
     * Called when an incoming message is available.
     * @param userData Custom user data.
     * @param opcode The message type. One of \c SN_OPCODE_PING, \c SN_OPCODE_PONG,
     * \c SN_OPCODE_TEXT and \c SN_OPCODE_BINARY.
     * @param bytes The message data.
     * @param numBytes The number of message bytes. If \c opcode is \c SN_OPCODE_TEXT,
     * \c bytes is null terminated and \c numBytes is the message size excluding the
     * last null byte.
     */
    typedef void (*snMessageCallback)(void* userData, snOpcode opcode, const char* bytes, int numBytes);
    
    /**
     * Notifies the application when the opening handshake has been completed.
     * @param userData
     */
    typedef void (*snOpenCallback)(void* userData);
    
    /**
     * Notifies the application when the connection has been closed.
     * @param userData
     * @param status
     */
    typedef void (*snCloseCallback)(void* userData, snStatusCode status);
    
    /**
     * Notifies the application when an error occurs.
     * @param userData
     * @param error
     */
    typedef void (*snErrorCallback)(void* userData, snError error);
    
        
    /** @} */
    
    /**
     * @name API
     */
    /** @{ */
    
    /**
     * Websocket creation settings.
     */
    typedef struct snWebsocketSettings
    {
        /** 
         * The desired maximum frame size in bytes. If 0, the default
         * max size will be used.
         */
        int maxFrameSize;
        /** */
        snLogCallback logCallback;
        /** A callback to pass received frames to. Ignored if NULL. */
        snFrameCallback frameCallback;
        /** If NULL, default socket I/O is used. */
        snIOCallbacks* ioCallbacks;
    } snWebsocketSettings;
    
    /**
     * Creates a new web socket.
     * @param openCallback A function to call when the opening handshake has been completed. Ignored if NULL.
     * @param messageCallback A function to call when receiving pings, pongs or 
     * full text or binary messages. Ignored if NULL.
     * @param closeCallback A function to call when the websocket connection is closed. Ignored if NULL.
     * @param errorCallback A function to call when an error occurs. Ignored if NULL.
     * @param callbackData A pointer passed to \c openCallback, \c messageCallback, \c closeCallback and \c errorCallback.
     * @return The created websocket or NULL on error.
     */
    snWebsocket* snWebsocket_create(snOpenCallback openCallback,
                                    snMessageCallback messageCallback,
                                    snCloseCallback closeCallback,
                                    snErrorCallback errorCallback,
                                    void* callbackData);
    
    /**
     * Initializes a web socket using custom settings.
     * @param openCallback A function to call when the opening handshake has been completed. Ignored if NULL.
     * @param messageCallback A function to call when receiving pings, pongs or
     * full text or binary messages. Ignored if NULL.
     * @param closeCallback A function to call when the websocket connection is closed. Ignored if NULL.
     * @param errorCallback A function to call when an error occurs. Ignored if NULL.
     * @param callbackData A pointer passed to \c openCallback, \c messageCallback, \c closeCallback and \c errorCallback.
     * @param settings Custom websocket settings.
     * @return The created websocket or NULL on error.
     */
    snWebsocket* snWebsocket_createWithSettings(snOpenCallback openCallback,
                                                snMessageCallback messageCallback,
                                                snCloseCallback closeCallback,
                                                snErrorCallback errorCallback,
                                                void* callbackData,
                                                snWebsocketSettings* settings);
    
    /**
     * Closes and deletes a given websocket.
     * @param ws The websocket to delete.
     */
    void snWebsocket_delete(snWebsocket* ws);
    
    /**
     * Connects to a given host.
     * @param ws The websocket.
     * @param url The URL to connect to.
     * @return An error code.
     */
    snError snWebsocket_connect(snWebsocket* ws, const char* url);
    
    /**
     * Disconnect from the current host, if any.
     * @param ws The websocket to disconnect.
     * @param disconnectImmediately If non-zero, the closing handshake
     * is not performed and the connection is dropped immediately with status
     * \c SN_STATUS_ENDPOINT_GOING_AWAY. Otherwise, the connection
     * is dropped when the closing handshake is completed or takes too long.
     */
    void snWebsocket_disconnect(snWebsocket* ws, int disconnectImmediately);
    
    /**
     * Check the state of a websocket.
     * @param ws The websocket.
     * @return The websocket state.
     */
    snReadyState snWebsocket_getState(snWebsocket* ws);
        
    /**
     * Send a ping message.
     * @param ws The websocket.
     * @param payloadSize The size of the payload in bytes.
     * @param payload The payload.
     * @return An error code.
     */
    snError snWebsocket_sendPing(snWebsocket* ws, int payloadSize, const char* payload);
    
    /**
     * Send a text message. The size of the payload is determined by the position of 
     * the first null byte.
     * @param ws The websocket.
     * @param payload Null terminated UTF-8 data to send.
     * @return An error code.
     */
    snError snWebsocket_sendTextData(snWebsocket* ws, const char* payload);
    
    /**
     * Send a binary message.
     * @param ws The websocket.
     * @param payloadSize The size of the payload in bytes.
     * @param payload The bytes to send.
     * @return An error code.
     */
    snError snWebsocket_sendBinaryData(snWebsocket* ws, int payloadSize, const char* payload);
    
    /**
     * Send a frame with a given opcode and payload.
     * @param ws The websocket.
     * @param opcode The opcode of the frame to send.
     * @param payloadSize The size of the payload in bytes.
     * @param payload The payload data.
     * @return An error code.
     */
    snError snWebsocket_sendFrame(snWebsocket* ws, snOpcode opcode, int payloadSize, const char* payload);
    
    /**
     * Receives incoming data, if any, and notifies the caller of newly available frames
     * and connection state changes.
     * @param ws The websocket
     */
    void snWebsocket_poll(snWebsocket* ws);
    
    /** @} */
    
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*SN_WEBSOCKET_H*/



