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

#ifndef SN_UTF8_H
#define SN_UTF8_H

/*! \file */

typedef unsigned char uint8_t;

typedef unsigned int uint32_t;


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
   
    /**
     * Does UTF-8 validation on a set of bytes, starting from a given
     * state. 
     * @param firstyByte The first byte to process.
     * @param numBytes The number of bytes to process.
     * @param state On input, the initial validator state. On output, the validator
     * state after processing the data. 
     */
    int snUTF8ValidateStringIncremental(uint8_t* firstByte, int numBytes, uint32_t* state);
    
    /** 
     * Checks if a string is valid UTF-8.
     * @param string The null terminated string to validate.
     * @return 0 if the string is not valid UTF-8, non-zero otherwise.
     */
    int snUTF8ValidateString(const char* string);
       
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SN_UTF8_H*/
