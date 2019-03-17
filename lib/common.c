/*
 * common.c
 *      ASJ-common structs and functions
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/lib/common.c
 */

#include "common.h"


TokenSequence
tokenize(const char* string, const char* delim)
{
    char* string_copy;

    TokenSequence result = {0, NULL};

    char* next = NULL;
    unsigned long size_allocated = 0;

    // Copy string, as strtok() requires
    string_copy = palloc(strlen(string) + 1);
    strcpy(string_copy, string);

    // Iterate over all strtok() results
    next = strtok(string_copy, delim);
    while (next != NULL) {
        if (result.size >= size_allocated) {
            size_allocated = (size_allocated + 1) * 2;
            result.ts = result.ts == NULL ?
                palloc(sizeof(*result.ts) * size_allocated) :
                repalloc(result.ts, sizeof(*result.ts) * size_allocated);
        }
        result.ts[result.size++] = next;
        next = strtok(NULL, delim);
    }

    if (size_allocated > result.size) {
        result.ts = repalloc(result.ts, sizeof(*result.ts) * result.size);
    }

    return result;
}


int
cmp_tokens_wrapper(const void* a, const void* b)
{
    return cmp_tokens(*(const char**)a, *(const char**)b);
}

