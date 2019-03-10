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


#endif /* COMMON_H */
