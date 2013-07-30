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

#ifndef SN_ERROR_CODES_H
#define SN_ERROR_CODES_H

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
 
    /**
     *
     */
    typedef enum snError
    {
        /** Success. */
        SN_NO_ERROR = 0,
        /** A non zero masking key was expected. */
        SN_MASKING_KEY_IS_ZERO,
        /** The maximum payload size of a control frame was exceeded. */
        SN_CONTROL_FRAME_PAYLOAD_TOO_LARGE,
        /** A control frame had the FIN flag set to zero. */
        SN_NON_FINAL_CONTROL_FRAME,
        /** Encountered an invalid opcode. */
        SN_INVALID_OPCODE,
        /** Failed to parse a URL. */
        SN_INVALID_URL,
        /** Encountered invalid UTF-8 data. */
        SN_INVALID_UTF8,
        /** */
        SN_UNEXPECTED_CONTINUATION_FRAME,
        /** */
        SN_EXPECTED_CONTINUATION_FRAME,
        /** */
        SN_NONZERO_RESVERVED_BIT,
        /** */
        SN_EXCEEDED_MAX_PAYLOAD_SIZE
    } snError;
    
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*SN_WEBSOCKET_H*/



