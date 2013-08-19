#include <assert.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "mutablestring.h"

void snMutableString_init(snMutableString* ms)
{
    memset(ms, 0, sizeof(snMutableString));
}

void snMutableString_deinit(snMutableString* ms)
{
    if (ms->dynamicData)
    {
        free(ms->dynamicData);
    }
    
    memset(ms, 0, sizeof(snMutableString));
}

void snMutableString_append(snMutableString* ms, const char* toAppend)
{
    snMutableString_appendBytes(ms, toAppend, strlen(toAppend));
}

void snMutableString_appendInt(snMutableString* ms, int toApped)
{
    char temp[256];
    sprintf(temp, "%d", toApped);
    snMutableString_append(ms, temp);
}

void snMutableString_appendBytes(snMutableString* ms, const char* toAppend, int numBytes)
{
    if (!toAppend)
    {
        return;
    }
    
    const size_t newCharCount = numBytes + ms->charCount;
    
    if (newCharCount > SN_MUTABLE_STRING_STATIC_SIZE)
    {
        //too big to be static. copy the data
        if (ms->charCount <= SN_MUTABLE_STRING_STATIC_SIZE)
        {
            assert(ms->dynamicData == 0);
        }
        
        ms->dynamicData = realloc(ms->dynamicData, newCharCount + 1);
        
        if (ms->charCount <= SN_MUTABLE_STRING_STATIC_SIZE)
        {
            memcpy(ms->dynamicData, ms->staticData, ms->charCount);
            ms->dynamicData[ms->charCount] = '\0';
            memset(ms->staticData, 0, SN_MUTABLE_STRING_STATIC_SIZE);
        }
    }
    
    char* targetData = newCharCount > SN_MUTABLE_STRING_STATIC_SIZE ? ms->dynamicData : ms->staticData;
    
    memcpy(&targetData[ms->charCount], toAppend, numBytes);
    ms->charCount = newCharCount;
    targetData[ms->charCount] = '\0';
}

const char* snMutableString_getString(snMutableString* ms)
{
    return ms->dynamicData != 0 ? ms->dynamicData : ms->staticData;
}
