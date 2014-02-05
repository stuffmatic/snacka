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

#ifndef SN_IOCALLBACKS_H
#define SN_IOCALLBACKS_H

/*! \file */

#include "errorcodes.h"
#include "websocket.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    /**
     * Creates or initializes a custom IO object.
     */
    typedef snError (*snIOInitCallback)(void** ioObject);
    
    /**
     * Releases a custom IO object.
     */
    typedef snError (*snIODeinitCallback)(void* ioObject);
    
    /**
     * Connects to a custom IO object.
     */
    typedef snError (*snIOConnectCallback)(void* ioObject,
                                           const char* host,
                                           int port);
    
    /**
     * Checks if a custom IO object is open, i.e ready for reading/writing.
     * @param ioObject The IO object
     * @param result Gets set to a non-zero value if the IO object is open, otherwise zero.
     */
    typedef snError (*snIOIsOpenCallback)(void* ioObject, int* result);
    
    /**
     * Disconnects from a custom IO object.
     */
    typedef snError (*snIODisconnectCallback)(void* ioObject);
    
    /**
     * Reads data, if any, from a custom I/O object.
     * @param ioObject The I/O object to read from.
     * @param buffer The buffer to store read data in.
     * @param bufferSize The size of \c buffer.
     * @param numBytesRead The number of bytes actually read.
     * @return An error code.
     */
    typedef snError (*snIOReadCallback)(void* ioObject, char* buffer, int bufferSize, int* numBytesRead);
    
    /**
     * Attempts to write data to a custom IO object
     */
    typedef snError (*snIOWriteCallback)(void* ioObject,
                                         const char* buffer,
                                         int bufferSize,
                                         int* numBytesWritten);
    
    /**
     * A set of callbacks representing operations on a custom IO object, e.g a socket.
     */
    typedef struct snIOCallbacks
    {
        /** */
        snIOInitCallback initCallback;
        /** */
        snIODeinitCallback deinitCallback;
        /** */
        snIOConnectCallback connectCallback;
        /** */
        snIOIsOpenCallback isOpenCallback;
        /** */
        snIODisconnectCallback disconnectCallback;
        /** */
        snIOReadCallback readCallback;
        /** */
        snIOWriteCallback writeCallback;
        
    } snIOCallbacks;
    
    
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SN_IOCALLBACKS_H*/



