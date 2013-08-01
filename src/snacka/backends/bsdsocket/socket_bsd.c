
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

#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>

#include "socket.h"

struct stfSocket
{
    int fileDescriptor;
    char* host;
    int port;
    int logErrors;
};

static void log(stfSocket* s, const char* fmt, ...)
{
#ifdef DEBUG
    if (s->logErrors)
    {
        va_list args;
        va_start(args,fmt);
        vprintf(fmt,args);
        va_end(args);
    }
#endif
}

static int shouldStopOnError(stfSocket* s, int error, int* ignores, int numInores)
{
    
    if (error == 0)
    {
        //all good
        return 0;
    }
    
    //see if this error code should be ignored
    for (int i = 0; i < numInores; i++)
    {
        if (ignores[i] == error)
        {
            return 0;
        }
    }

#ifdef DEBUG
    
    switch (error)
    {
        case EACCES:
        {
            log(s, "errno == EACCES: (For UNIX domain sockets, which are identified by pathname) Write permission is denied on the destination socket file, or search permission is denied for one of the directories the path prefix. (See path_resolution(7).)\n");
            break;
        }
        case EAGAIN: //same as EWOULDBLOCK
        {
            log(s, "errno == EAGAIN || errno == EWOULDBLOCK: The socket is marked nonblocking and the requested operation would block. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities.\n");
            break;
        }
        case EBADF:
        {
            log(s, "errno == EBADF: An invalid descriptor was specified.\n");
            break;
        }
        case ECONNRESET:
        {
            log(s, "errno == ECONNRESET: Connection reset by peer.\n");
            break;
        }
        case EDESTADDRREQ:
        {
            log(s, "errno == EDESTADDRREQ: The socket is not connection-mode, and no peer address is set.\n");
            break;
        }
        case EFAULT:
        {
            log(s, "errno == EFAULT: An invalid user space address was specified for an argument.\n");
            break;
        }
        case EINTR:
        {
            log(s, "errno == EINTR: A signal occurred before any data was transmitted; see signal(7).\n");
            break;
        }
        case EINVAL:
        {
            log(s, "errno == EINVAL: Invalid argument passed.\n");
            break;
        }
        case EISCONN:
        {
            log(s, "errno == EISCONN: The connection-mode socket was connected already but a recipient was specified. (Now either this error is returned, or the recipient specification is ignored.)\n");
            break;
        }
        case EMSGSIZE:
        {
            log(s, "errno == EMSGSIZE: The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible.\n");
            break;
        }
        case ENOBUFS:
        {
            log(s, "errno == ENOBUFS: The output queue for a network interface was full. This generally indicates that the interface has stopped sending, but may be caused by transient congestion. (Normally, this does not occur in  Linux. Packets are just silently dropped when a device queue overflows.)\n");
            break;
        }
        case ENOMEM:
        {
            log(s, "errno == ENOMEM: No memory available\n");
            break;
        }
        case ENOTCONN:
        {
            log(s, "errno == ENOTCONN: The socket is not connected, and no target has been given.\n");
            break;
        }
        case ENOTSOCK:
        {
            log(s, "errno == ENOTSOCK: The argument sockfd is not a socket.\n");
            break;
        }
        case EOPNOTSUPP:
        {
            log(s, "errno == EOPNOTSUPP: Some bit in the flags argument is inappropriate for the socket type.\n");
            break;
        }
        case EPIPE:
        {
            log(s, "errno == EPIPE: The local end has been shut down on a connection oriented socket. In this case the process will also receive a SIGPIPE unless MSG_NOSIGNAL is set.\n");
            break;
        }
        case ENOPROTOOPT:
        {
            log(s, "errno == ENOPROTOOPT: Protocol not available.\n");
            break;
        }
        case ETIMEDOUT:
        {
            log(s, "errno == ETIMEDOUT\n");
            break;
        }
        default:
        {
            log(s, "errno == %d.\n", error);
        }
    }
    
#endif //DEBUG
    
    return 1;
}

stfSocket* stfSocket_new()
{
    stfSocket* newSocket = malloc(sizeof(stfSocket));
    memset(newSocket, 0, sizeof(stfSocket));
    newSocket->logErrors = 1;
    newSocket->fileDescriptor = -1;
    return newSocket;
}

void stfSocket_delete(stfSocket* socket)
{
    if (socket)
    {
        stfSocket_disconnect(socket);
        
        if (socket->host)
        {
            free(socket->host);
        }
        memset(socket, 0, sizeof(stfSocket));
        free(socket);
    }
}

int stfSocket_connect(stfSocket* s,
                      const char* host,
                      int port,
                      stfSocketCancelCallback cancelCallback,
                      void* callbackData)
{
    errno = 0;
    
    if (s->fileDescriptor != -1)
    {
        //shut down existing connection
        stfSocket_disconnect(s);
    }
    
    struct addrinfo* addrinfoResult;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    char service[256];
    sprintf(service, "%d", port);
    
    //TODO: this call is blocking...
    int error = getaddrinfo(host, service, &hints, &addrinfoResult);
    if (error != 0)
    {
        return 0;
    }
    
    // loop through all the results and connect to the first we can
    for(struct addrinfo* p = addrinfoResult; p != NULL; p = p->ai_next)
    {
        int connected = 0;
        
        s->fileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s->fileDescriptor == -1)
        {
            //printf("client: socket, errno %d\n", errno);
            continue;
        }
        
        //set socket to non-blocking
        int flags = fcntl(s->fileDescriptor, F_GETFL, 0);
        fcntl(s->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
        
        //attempt async connect, regularly
        //invoking cancelCallback to see if we should
        //abort the connection attempt
        const float timeout = 3.0f;
        float t = 0.0f;
        float dt = 0.01f;
        
        /*const int connectResult = */connect(s->fileDescriptor, p->ai_addr, p->ai_addrlen);
        
        while (t < timeout)
        {
            if (cancelCallback)
            {
                if (cancelCallback(callbackData) == 0)
                {
                    //caller requested timeout
                    stfSocket_disconnect(s);
                    return 0;
                }
            }
            
            struct fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(s->fileDescriptor, &fdset);
            assert(FD_ISSET(s->fileDescriptor, &fdset));
            struct timeval timeoutStruct = { 0, 1000000 * dt };
            int selRes = select(s->fileDescriptor + 1, NULL, &fdset, NULL, &timeoutStruct);
            
            assert(selRes >= 0);
            
            if (selRes > 0)
            {
                socklen_t len = sizeof(errno);
                
                getsockopt(s->fileDescriptor, SOL_SOCKET, SO_ERROR, &errno, &len);
                
                if (errno == 0)
                {
                    //connected
                    connected = 1;
                    break;
                }
                else
                {
                    //log(s, "select() following non blocking connect() failed, errno %d\n", errno);
                    s->fileDescriptor = -1;
                    break;
                }
            }
            
            t += dt;
            usleep(1000 * dt);
        }
        
        if (t >= timeout)
        {
            close(s->fileDescriptor);
            s->fileDescriptor = -1;
        }
        
        if (connected)
        {
            break;
        }
    }
    
    if (s->fileDescriptor < 0)
    {
        return 0;
    }
    
    //disable nagle's algrithm
    int flag = 1;
    int result = setsockopt(s->fileDescriptor, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof flag);
    assert(result == 0);
    
    //disable sigpipe
    int set = 1;
    result = setsockopt(s->fileDescriptor, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    assert(result == 0);
    
    return 1;
   
}

void stfSocket_disconnect(stfSocket* socket)
{
    free(socket->host);
    socket->host = 0;
    socket->port = 0;
    shutdown(socket->fileDescriptor, SHUT_RDWR);
    close(socket->fileDescriptor);
    
    socket->fileDescriptor = -1;
}

int stfSocket_sendData(stfSocket* s, const char* data, int numBytes, int* numSentBytes,
                       stfSocketCancelCallback cancelCallback, void* callbackData)
{
    int numBytesSentTot = 0;
    while (numBytesSentTot < numBytes)
    {
        errno = 0;
        //wait for the socket to become availalbe for writing
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s->fileDescriptor, &rfds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000; //10 ms
        
        int selRet = select(s->fileDescriptor + 1, NULL, &rfds, NULL, &tv);
        
        if (selRet == -1)
        {
            return 0;
        }
        else if (selRet == 0)
        {
            //printf("no data yet\n");
        }
        else
        {
            //printf("new data available\n");
        }
        
        //check if we should abort
        if (cancelCallback)
        {
            if (cancelCallback(callbackData) == 0)
            {
                return 0;
            }
        }
        
        //try to send all the bytes we have left
        const int chunkSize = numBytes - numBytesSentTot;
        ssize_t ret = send(s->fileDescriptor,
                           (const void*)(&data[numBytesSentTot]),
                           chunkSize,
                           0);
        
        //check errors
        int ignores[2] = {EAGAIN, EWOULDBLOCK};
        if (shouldStopOnError(s, errno, ignores, 2))
        {
            return 0;
        }
        
        if (ret >= 0)
        {
            numBytesSentTot += ret;
        }
        
        //printf("sent %d/%d\n", numBytesSentTot, numBytes);
    }
    
    return 1;
}

int stfSocket_receiveData(stfSocket* s, char* data, int maxNumBytes, int* numBytesReceived)
{
    errno = 0;
    assert(data);
    int success = 1;
    ssize_t bytesRecvd = recv(s->fileDescriptor,
                              data,
                              maxNumBytes,
                              0);
    
    int ignores[2] = {EAGAIN, EWOULDBLOCK};
    if (shouldStopOnError(s, errno, ignores, 2))
    {
        success = 0;
    }
    
    *numBytesReceived = bytesRecvd < 0 ? 0 : bytesRecvd;
    
    return success;
}
