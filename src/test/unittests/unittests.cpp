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

#include <stdio.h>

#include "sput.h"

#include "testframe.h"
#include "testframeparser.h"
#include "testopeninghandshakeparser.h"
#include "testwebsocketcpp.h"

/**
 *
 */
int main(int argc, const char * argv[])
{
    sput_start_testing();
    
    sput_enter_suite("snFrameHeader tests");
    sput_run_test(testFrameHeaderSerialization);
    sput_run_test(testFrameHeaderValidation);
    sput_run_test(testMasking);
    
    sput_enter_suite("snFrameParser tests");
    sput_run_test(testFrameParserHeaderEquality);
    
    sput_enter_suite("snOpeningHandshakeParser tests");
    sput_run_test(testWrongHTTPStatus);
    sput_run_test(testMissingWebsocketKey);
    sput_run_test(testHeaderFollowedByFrames);
    
    sput_enter_suite("c++ wrapper tests");
    sput_run_test(testWebsocketCppEcho);
    sput_run_test(testWebsocketCppInvalidURLTimeout);
    sput_run_test(testWebsocketCppInvalidURLCancel);
    
    sput_finish_testing();
    
}

