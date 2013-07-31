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

#ifndef SN_FRAME_HEADER_H
#define SN_FRAME_HEADER_H

#include "errorcodes.h"

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    /**
     * The maximum size in bytes of a websocket header.
     */
    #define SN_MAX_HEADER_SIZE 14
    
    /**
     * Valid websocket frame opcodes.
     * @see https://tools.ietf.org/html/rfc6455#section-5.2
     */
    typedef enum snOpcode
    {
        SN_OPCODE_CONTINUATION = 0x00,
        SN_OPCODE_TEXT = 0x01,
        SN_OPCODE_BINARY = 0x02,
        SN_OPCODE_CONNECTION_CLOSE = 0x08,
        SN_OPCODE_PING = 0x09,
        SN_OPCODE_PONG = 0x0A
    } snOpcode;
    
    /**
     * A struct representation of a websocket frame header.
     * \code
     * 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-------+-+-------------+-------------------------------+
     * |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     * |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     * |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     * | |1|2|3|       |K|             |                               |
     * +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     * |     Extended payload length continued, if payload len == 127  |
     * + - - - - - - - - - - - - - - - +-------------------------------+
     * |                               |Masking-key, if MASK set to 1  |
     * +-------------------------------+-------------------------------+
     * | Masking-key (continued)       |          Payload Data         |
     * +-------------------------------- - - - - - - - - - - - - - - - +
     * :                     Payload Data continued ...                :
     * + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     * |                     Payload Data continued ...                |
     * +---------------------------------------------------------------+
     * \endcode
     * @see https://tools.ietf.org/html/rfc6455#section-5.2
     */
    typedef struct snFrameHeader
    {
        /** The frame's opcode */
        snOpcode opcode;
        /** Indicates if the payload is split up into multiple frames. */
        int isFinal;
        /** Non-zero if the payload is masked, zero otherwise. */
        int isMasked;
        /** The key used to mask the payload, if \c isMasked is not zero.*/
        int maskingKey;
        /** The number of payload bytes. */
        unsigned long long payloadSize;
    } snFrameHeader;
    
    /**
     * Converts a \c snFrameHeader struct to an array of bytes
     * according to the websocket data framing spec.
     * @param h The header to serialize.
     * @param headerBytes An array to store the result in.
     * @param headerSize The resulting size of the header, at most \c SN_MAX_HEADER_SIZE.
     * @return An error code.
     * @see https://tools.ietf.org/html/rfc6455#section-5.2
     */
    snError snFrameHeader_toBytes(snFrameHeader* h, char* headerBytes, int* headerSize);
    
    /**
     * Converts an array of bytes to a \c snFrameHeader struct
     * according to the websocket data framing spec.
     * @param h The resulting deserialized header.
     * @param headerBytes The input bytes.
     * @param headerSize On output, this variable contains the size of the 
     * input header bytes, at most \c SN_MAX_HEADER_SIZE.
     * @see https://tools.ietf.org/html/rfc6455#section-5.2
     */
    snError snFrameHeader_fromBytes(snFrameHeader* h, const char* headerBytes, int* headerSize);
    
    /**
     * Validates a given websocket header.
     * @param h The header to validate.
     * @return The validation result.
     */
    snError snFrameHeader_validate(const snFrameHeader* h);
    
    /**
     * Applies a mask in a given header to a portion of a payload. If
     * the header's mask flag is not set, this function does nothing.
     * @param h The header providing the mask flag.
     * @param payload The payload to mask.
     * @param numBytes
     * @param offset
     * @return An error code.
     * @see https://tools.ietf.org/html/rfc6455#section-5.3
     */
    snError snFrameHeader_applyMask(snFrameHeader* h, char* payload, int numBytes, int offset);
    
    /**
     * Compares two websocket frame headers.
     * @param h1 The first header.
     * @param h2 The second header.
     * @return A non-zero value if the two headers are equal, zero otherwise.
     */
    int snFrameHeader_equals(const snFrameHeader* h1, const snFrameHeader* h2);
    
    /**
     * Prints info about a given frame header to stdout.
     * @param h The frame header to print.
     */
    void snFrameHeader_log(const snFrameHeader* h);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SN_FRAME_HEADER_H*/

