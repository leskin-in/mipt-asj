#include "mipt-asj.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/**
 * Return table name by given Oid.
 *
 * SPI connection must be already open.
 *
 * @return palloc'ed pointer to table name
 * @return NULL if no requested table exists
 */
static char* get_table_name_by_oid(Oid oid) {
    char query[32];
    sprintf(query, "SELECT %d::regclass;", oid);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL || SPI_processed != 1) {
        return NULL;
    }
    char* result = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);
    SPI_freetuptable(SPI_tuptable);
    return result;
}

/**
 * Get null-terminated (C-string) TEXT parameter in palloc'ed memory, stored inside null-terminated string
 *
 * @return NULL if the given parameter is NULL
 */
static char* get_text_parameter(const void *PTR) {
    if (PTR == NULL) {
        return NULL;
    }
    const int L = VARSIZE(PTR) - VARHDRSZ;
    char* result = palloc(L + 1);
    memcpy(result, VARDATA(PTR), L);
    result[L] = '\0';
}

/**
 * Calculate abbreviation dictionary and place it into new table, returning its ID
 */
Datum
asj_calc_dict(PG_FUNCTION_ARGS)
{
    SPI_connect();

    // Function arguments

    Oid fullOid = PG_GETARG_OID(0);
    char *fullCol = get_text_parameter(PG_GETARG_TEXT_P(1));
    Oid abbrOid = PG_GETARG_OID(2);
    char *abbrCol = get_text_parameter(PG_GETARG_TEXT_P(3));


    // Local variables

    char query[4096];

    char** stored_abbrs;
    int stored_abbrs_used = 0;

    struct trie* trie = trie_create();
    if (trie == NULL) {
        elog(ERROR, "Could not create trie structure. Not enough memory?");
    }

    char* leftTable = get_table_name_by_oid(fullOid);
    if (leftTable == NULL)
        elog(ERROR, "Table with OID %d not found.", fullOid);

    char* rightTable = get_table_name_by_oid(abbrOid);
    if (rightTable == NULL)
        elog(ERROR, "Table with OID %d not found.", fullOid);


    // Process abbreviations column

    sprintf(query, "SELECT %s FROM %s;", fullCol, leftTable);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT from abbreviations table '%s' (OID %d).", leftTable, fullOid);
    }
    elog(WARNING, "Processing %d rows...", SPI_processed);

    stored_abbrs = palloc(sizeof(char*) * SPI_processed);
    for (int i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        if (row == NULL) {
            continue;
        }
        trie_insert(trie, row, row);
        stored_abbrs[stored_abbrs_used] = row;
        stored_abbrs_used += 1;
    }
    if (stored_abbrs_used == 0) {
        elog(ERROR, "No abbreviations found in given table and column.");
    }

    SPI_freetuptable(SPI_tuptable);


    // Read full forms column

    sprintf(query, "SELECT %s FROM %s;", abbrCol, rightTable);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT from full forms table '%s' (OID %d).", rightTable, abbrOid);
    }
    elog(WARNING, "Processing %d rows...", SPI_processed);

    for (int i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);

        // To be continued

        pfree(row);
    }

    // To be continued


    SPI_freetuptable(SPI_tuptable);


    SPI_finish();
    PG_RETURN_INT32(-1);
}

/**
 *
 */
Datum
asj_calc_pairs(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(-1);
}

/**
 *
 */
Datum
asjcmp(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(false);
}
