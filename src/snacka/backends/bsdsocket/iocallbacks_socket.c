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
 * of the authors and should not be interpreted asrepresenting official policies,
 * either expressed or implied, of the copyright holders.
 */

#include "websocket.h"
#include "iocallbacks_socket.h"
#include "socket.h"

snError snSocketInitCallback(void** socket)
{
    *socket = stfSocket_new();
    return SN_NO_ERROR;
}

snError snSocketDeinitCallback(void* socket)
{
    stfSocket_delete(socket);
    return SN_NO_ERROR;
}

snError snSocketConnectCallback(void* userData,
                                const char* host,
                                int port)
{
    stfSocket* socket = (stfSocket*)userData;
    int result = stfSocket_connect(socket, host, port, 0, 0);
    //TODO: error handling
    return SN_NO_ERROR;
}

snError snSocketDisconnectCallback(void* userData)
{
    stfSocket* socket = (stfSocket*)userData;
    stfSocket_disconnect(socket);
    return SN_NO_ERROR;
}

snError snSocketReadCallback(void* userData,
                             char* buffer,
                             int bufferSize,
                             int* numBytesRead)
{
    stfSocket* socket = (stfSocket*)userData;
    
    stfSocket_receiveData(socket,
                          buffer,
                          bufferSize,
                          numBytesRead);
    
    return SN_NO_ERROR;
}

snError snSocketWriteCallback(void* userData,
                              const char* buffer,
                              int bufferSize,
                              int* numBytesWritten)
{
    stfSocket* socket = (stfSocket*)userData;
    
    stfSocket_sendData(socket, buffer, bufferSize, numBytesWritten, 0, 0);
    
    return SN_NO_ERROR;
}
