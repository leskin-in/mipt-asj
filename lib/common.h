#ifndef COMMON_H
#define COMMON_H

/*
 * common.h
 *      ASJ-common structs and functions
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/lib/common.h
 */

#include "postgres.h"
#include "fmgr.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "funcapi.h"


/**
 * @brief Internal representation of StringPairRows.
 */
typedef struct StringPairRows_t {
    unsigned long size;
    char*** pairs;
    /**
     * Elements, already returned to user
     */
    unsigned long read;
} StringPairRows;


/**
 * @brief ojbect to store tokenized strings in
 */
typedef struct {
    unsigned long size;
    /**
     * Token values
     */
    char** ts;
} TokenSequence;


/**
 * @brief Remove element from TokenSequence
 *
 * @param ts
 * @param i
 *
 * @return char* removed pointer
 */
inline char*
remove_from_token_sequence(TokenSequence* ts, unsigned long i)
{
    char* result;
    for (unsigned long j = i + 1; j < ts->size; j++) {
        ts->ts[j - 1] = ts->ts[j];
    }

    result = ts->ts[ts->size - 1];
    ts->ts[ts->size - 1] = NULL;
    ts->size -= 1;

    if (ts->size == 0) {
        pfree(ts->ts);
        ts->ts = NULL;
    }
    else {
        ts->ts = repalloc(ts->ts, ts->size);
    }

    return result;
}


/**
 * @brief Return table name by given Oid. SPI must be prepared.
 *
 * @return palloc'ed pointer to table name
 * Will elog(ERROR) in case requested table does not exist
 */
inline char*
get_table_name_by_oid(Oid oid)
{
    char query[32];
    char* result;

    sprintf(query, "SELECT %d::regclass;", oid);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL || SPI_processed != 1) {
        return NULL;
    }
    result = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);
    SPI_freetuptable(SPI_tuptable);

    if (result == NULL) {
        elog(ERROR, "Table with OID %d not found", oid);
    }

    return result;
}


/**
 * Get null-terminated (C-string) TEXT parameter in palloc'ed memory, stored inside null-terminated string
 *
 * @return NULL if the given parameter is NULL
 */
inline char*
get_text_parameter(const void *PTR)
{
    const int L = PTR == NULL ? 0 : VARSIZE(PTR) - VARHDRSZ;
    char* result;
    if (PTR == NULL) {
        return NULL;
    }

    result = palloc(L + 1);
    memcpy(result, VARDATA(PTR), L);
    result[L] = '\0';

    return result;
}


/**
 * @brief Tokenize given string using provided delimeter
 *
 * @param string
 * @param delim delimeter used by strtok
 * @return Token sequence
 *
 * @TODO: Check how strtok() allocates memory
 */
TokenSequence
tokenize(const char* string, const char* delim);


/**
 * @brief Compare token sequences using special ("reverse") metric
 */
inline int
cmp_tokens(const char* t1, const char* t2)
{
    return strlen(t1) == strlen(t2) ? strcmp(t1, t2) : strlen(t2) - strlen(t1);
}


/**
 * @brief Wrapper around 'cmp_tokens' for qsort
 */
int
cmp_tokens_wrapper(const void* a, const void* b);


/**
 * @brief Swap contents of two pointers
 */
inline void
swap(void** a, void** b)
{
    void* t = *b;
    *b = *a;
    *a = t;
}


#endif /* COMMON_H */
