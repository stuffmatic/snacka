/*
 * Copyright (c) 2013 - 2014, Per Gantelius
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

#include "errorcodes.h"

const char* snErrorToString(snError error)
{
    switch (error)
    {
        case SN_NO_ERROR:
        {
            return "No error";
        }
        case SN_MASKING_KEY_IS_ZERO:
        {
            return "Zero masking key";
        }
        case SN_CONTROL_FRAME_PAYLOAD_TOO_LARGE:
        {
            return "Control frame payload too large";
        }
        case SN_NON_FINAL_CONTROL_FRAME:
        {
            return "Non-final control frame";
        }
        case SN_INVALID_OPCODE:
        {
            return "Invalid opcode";
        }
        case SN_INVALID_URL:
        {
            return "Invalid URL";
        }
        case SN_INVALID_UTF8:
        {
            return "Invalid UTF8";
        }
        case SN_UNEXPECTED_CONTINUATION_FRAME:
        {
            return "Unexpected continuation frame";
        }
        case SN_EXPECTED_CONTINUATION_FRAME:
        {
            return "Expected continuation frame";
        }
        case SN_NONZERO_RESVERVED_BIT:
        {
            return "Nonzero reserved bit";
        }
        case SN_EXCEEDED_MAX_PAYLOAD_SIZE:
        {
            return "Payload too large";
        }
        case SN_SOCKET_FAILED_TO_CONNECT:
        {
            return "Failed to connect";
        }
        case SN_SOCKET_IO_ERROR:
        {
            return "IO error";
        }
        case SN_CANCELLED_OPERATION:
        {
            return "Cancelled operation";
        }
        case SN_WEBSOCKET_CONNECTION_IS_NOT_OPEN:
        {
            return "Websocket connection is not open";
        }
        case SN_INVALID_OPENING_HANDSHAKE_HTTP_STATUS:
        {
            return "Invalid opening handshake HTTP response status";
        }
        case SN_FAILED_TO_PARSE_OPENING_HANDSHAKE_RESPONSE:
        {
            return "Failed to parse opening handshake HTTP response";
        }
        default:
            break;
    }
    
    return "Unknown error code";
}
