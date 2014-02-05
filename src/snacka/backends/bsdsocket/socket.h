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

#ifndef STF_SOCKET_H
#define STF_SOCKET_H

/*! \file */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    typedef enum stfSocketConnectionState
    {
        STF_SOCKET_NOT_CONNECTED,
        STF_SOCKET_CONNECTED,
        STF_SOCKET_CONNECTING,
        STF_SOCKET_CONNECTION_FAILED
    } stfSocketConnectionState;
    
    /** */
    typedef struct stfSocket stfSocket;
    
    /** */
    stfSocket* stfSocket_new(void);
    
    /** */
    void stfSocket_delete(stfSocket* socket);
    
    /** 
     * Starts connecting a socket to a given host and port. This function returns
     * immediately. Call \c stfSocket_getConnectionState to see if the connection
     * failed, succeeded or is still in progress.
     * @param s The socket to connect.
     * @param host The host to connect to.
     * @param port The port to connect to.
     */
    int stfSocket_connect(stfSocket* s, const char* host, int port);
    
    /** */
    void stfSocket_disconnect(stfSocket* socket);
    
    /** */
    stfSocketConnectionState stfSocket_poll(stfSocket* socket);
    
    /** */
    int stfSocket_sendData(stfSocket* socket, const char* data, int numBytes,
                           int* numSentBytes);

    /** */    
    int stfSocket_receiveData(stfSocket* s, char* data, int maxNumBytes, int* numBytesReceived);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*STF_SOCKET_H*/
