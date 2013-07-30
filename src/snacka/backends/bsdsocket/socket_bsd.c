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

static void checkError(stfSocket* s, int error, int* ignores, int numInores)
{
#ifdef DEBUG
    
    if (error == 0)
    {
        //all good
        return;
    }
    
    //see if this error code should be ignored
    for (int i = 0; i < numInores; i++)
    {
        if (ignores[i] == error)
        {
            return;
        }
    }
    
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
        default:
        {
            log(s, "errno == %d.\n", errno);
        }
    }
    
#endif //DEBUG
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
                      stfSocketConnectWaitCallback connectWaitCallback,
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
    char service[265];
    sprintf(service, "%d", port);
    int error = getaddrinfo(host, service, &hints, &addrinfoResult);
    assert(error == 0);
    
    // loop through all the results and connect to the first we can
    for(struct addrinfo* p = addrinfoResult; p != NULL; p = p->ai_next)
    {
        s->fileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s->fileDescriptor == -1)
        {
            //printf("client: socket, errno %d\n", errno);
            continue;
        }
        
        if (connect(s->fileDescriptor, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(s->fileDescriptor);
            //printf("client: connect, errno %d\n", errno);
            continue;
        }
        
        break;
    }
    
    if (s->fileDescriptor < 0)
    {
        int b = 0;
        assert(0);
    }
    
    //set to non-blocking
    int flags = fcntl(s->fileDescriptor, F_GETFL, 0);
    fcntl(s->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
    
    int flag = 1;
    int result = setsockopt(s->fileDescriptor, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof flag);
    assert(result == 0);
    
    return 1;
    /*
     
     s->fileDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
     
     int flag = 1;
     int result = setsockopt(s->fileDescriptor, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof flag);
     assert(result == 0);
     
     //log(s, "setsockopt errno %d\n", errno);
     
     if (-1 == s->fileDescriptor)
     {
     log(s, "socket() failed, cannot create socket\n");
     
     
     return 0;
     }
     
     struct sockaddr_in socketAddress;
     memset(&socketAddress, 0, sizeof(socketAddress));
     
     socketAddress.sin_family = AF_INET;
     socketAddress.sin_port = htons(port);
     const int ptonResult =  inet_pton(AF_INET, host, &socketAddress.sin_addr);
     //app::Log::debugPrint("inet_pton: result %d\n", ptonResult);
     if (0 > ptonResult)
     {
     //perror("error: first parameter is not a valid address family");
     close(s->fileDescriptor);
     return 0;
     }
     else if (0 == ptonResult)
     {
     //perror("char string (second parameter does not contain valid ipaddress)");
     //printf("errno %d\n", errno);
     close(s->fileDescriptor);
     return 0;
     }
     
     if (connectWaitCallback)
     {
     int flags = fcntl(s->fileDescriptor, F_GETFL, 0);
     int setFlagResult = fcntl(s->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
     assert(setFlagResult == 0);
     }
     
     int connectResult = connect(s->fileDescriptor,
     testAddr->ai_addr, testAddr->ai_addrlen);
     
     if (connectWaitCallback)
     {
     const float timeout = 3.0f;
     float t = 0.0f;
     float dt = 0.01f;
     while (t < timeout)
     {
     if (connectWaitCallback(callbackData) == 0)
     {
     //caller requested timeout
     stfSocket_disconnect(s);
     return 0;
     }
     
     struct pollfd fd;
     fd.fd = s->fileDescriptor;
     fd.events =
     fd.revents = POLLIN;
     
     struct fd_set fdset;
     struct timeval timeout = {
     0, 1000000 * dt
     };
     FD_ZERO(&fdset);
     
     FD_SET(s->fileDescriptor, &fdset);
     assert(FD_ISSET(s->fileDescriptor, &fdset));
     int selRes = select(s->fileDescriptor + 1, NULL, &fdset, NULL, &timeout);
     
     
     assert(selRes >= 0);
     
     if (selRes > 0)
     {
     socklen_t len = sizeof(errno);
     
     getsockopt(s->fileDescriptor, SOL_SOCKET, SO_ERROR, &errno, &len);
     
     if (errno == 0)
     {
     //connected
     break;
     }
     else
     {
     log(s, "select() following non blocking connect() failed, errno %d\n", errno);
     return 0;
     }
     }
     
     t += dt;
     //usleep(1000 * dt);
     }
     
     if (t >= timeout)
     {
     //timed out
     return 0;
     }
     }
     
     //error handling for blocking connect
     if (-1 == connectResult && !connectWaitCallback)
     {
     log(s, "connect() failed, errno %d, could not connect to socket. return value %d\n", errno, connectResult);
     
    else if (errno == EACCES || errno == EPERM)
     {
     log(s, "errno == EACCES || errno == EPERM: The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the connection request failed because of a local firewall rule.\n");
     }
     else if (errno == EADDRINUSE)
     {
     log(s, "errno == EADDRINUSE: Local address is already in use.\n");
     }
     else if (errno == EAFNOSUPPORT)
     {
     log(s, "errno == EAFNOSUPPORT: The passed address didn't have the correct address family in its sa_family field.\n");
     }
     else if (errno == EAGAIN)
     {
     log(s, "errno == EAGAIN: No more free local ports or insufficient entries in the routing cache. For AF_INET see the description of /proc/sys/net/ipv4/ip_local_port_range ip(7) for information on how to increase the number of local ports.\n");
     }
     else if (errno == EALREADY)
     {
     log(s, "errno == EALREADY: The socket is nonblocking and a previous connection attempt has not yet been completed.\n");
     }
     else if (errno == EBADF)
     {
     log(s, "errno == EBADF: The file descriptor is not a valid index in the descriptor table.\n");
     }
     else if (errno == ECONNREFUSED)
     {
     log(s, "errno == ECONNREFUSED: No-one listening on the remote address.\n");
     }
     else if (errno == EFAULT)
     {
     log(s, "errno == EFAULT: The socket structure address is outside the user's address space.\n");
     }
     else if (errno == EINPROGRESS)
     {
     log(s, "errno == EINPROGRESS: The socket is nonblocking and the connection cannot be completed immediately. It is possible to select(2) or poll(2) for completion by selecting the socket for writing. After select(2) indicates writability, use getsockopt(2) to read the SO_ERROR option at level SOL_SOCKET to determine whether connect() completed successfully (SO_ERROR is zero) or unsuccessfully (SO_ERROR is one of the usual error codes listed here, explaining the reason for the failure).\n");
     }
     else if (errno == EINTR)
     {
     log(s, "errno == EINTR: The system call was interrupted by a signal that was caught; see signal(7).\n");
     }
     else if (errno == EISCONN)
     {
     log(s, "errno == EISCONN: The socket is already connected.\n");
     }
     else if (errno == ENETUNREACH)
     {
     log(s, "errno == ENETUNREACH: Network is unreachable.\n");
     }
     else if (errno == ENOTSOCK)
     {
     log(s, "errno == ENOTSOCK: The file descriptor is not associated with a socket.\n");
     }
     else if (errno == ETIMEDOUT)
     {
     log(s, "errno == ETIMEDOUT: Timeout while attempting connection. The server may be too busy to accept new connections. Note that for IP sockets the timeout may be very long when syncookies are enabled on the server.\n");
     }
     close(s->fileDescriptor);
     return 0;
     }
     
     if (!connectWaitCallback)
     {
     int flags = fcntl(s->fileDescriptor, F_GETFL, 0);
     fcntl(s->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
     }
     
     int set = 1;
     result = setsockopt(s->fileDescriptor, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
     //log(s, "setsockopt SO_NOSIGPIPE errno %d\n", errno);
     assert(result == 0);
     //result = setsockopt(s->fileDescriptor, SOL_SOCKET, IPPROTO_TCP, (void *)&set, sizeof(int));
     //log(s, "setsockopt IPPROTO_TCP errno %d\n", errno);
     //assert(result == 0);
     
     
     s->host = malloc(strlen(host) + 1);
     memccpy(s->host, host, 1, strlen(host) + 1);
     */
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

int stfSocket_sendData(stfSocket* s, const char* data, int numBytes, int* numSentBytes)
{
    errno = 0;
    int success = 1;
    ssize_t ret = send(s->fileDescriptor,
                       (const void*)data,
                       numBytes,
                       0);
    
    *numSentBytes = ret < 0 ? 0 : ret;
   int ignores[2] = {EAGAIN, EWOULDBLOCK};
    checkError(s, errno, ignores, 2);
    
    return success;
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
    checkError(s, errno, ignores, 2);
    
    *numBytesReceived = bytesRecvd < 0 ? 0 : bytesRecvd;
    
    return success;
}
