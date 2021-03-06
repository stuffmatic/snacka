#include <string.h>
#include "utf8.h"

/* Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
  See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details. */


#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 00..1f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20..3f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40..5f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60..7f */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, /* 80..9f */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, /* a0..bf */
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* c0..df */
    0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, /* e0..ef */
    0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, /* f0..ff */
    0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, /* s0..s0 */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, /* s1..s2 */
    1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, /* s3..s4 */
    1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, /* s5..s6 */
    1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* s7..s8 */
};

static uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
    uint32_t type = utf8d[byte];
    
    *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);
    
    *state = utf8d[256 + *state*16 + type];
    return *state;
}


static int countCodePoints(uint8_t* s, size_t* count)
{
    uint32_t codepoint;
    uint32_t state = 0;
    
    for (*count = 0; *s; ++s)
    {
        if (!decode(&state, &codepoint, *s))
        {
            *count += 1;
        }
    }
    
    return state != UTF8_ACCEPT;
}

int snUTF8ValidateStringIncremental(uint8_t* firstByte, int numBytes, uint32_t* state)
{
    int i;
    for (i = 0; i < numBytes; i++)
    {
        uint32_t codepoint;
        if (decode(state, &codepoint, firstByte[i]) == UTF8_REJECT)
        {
            return 0;
        }
    }
    
    return 1;
}

int snUTF8ValidateString(const char* string)
{
    int numCodePoints = 0;
    int result = countCodePoints(string, &numCodePoints);
    
    return result == 0;
}
