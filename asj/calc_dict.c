/*
 * calc_dict.c
 *      Abbreviation dictionary calculation, part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/calc_dict.c
 */

#include "calc_dict.h"


/**
 * @brief qsort comparator for pairs (full, abbreviation)
 *
 * Abbreviation is compared first, then full form
 */
static int
_compare_pair(const void* l, const void* r)
{
    const char** pair_l = *(const char***)l;
    const char** pair_r = *(const char***)r;
    int result;

    result = strcmp(pair_l[1], pair_r[1]);
    if (result == 0)
        result = strcmp(pair_l[0], pair_r[0]);
    return result;
}


/**
 * @brief Calculate abbreviation dictionary
 *
 * @note SPI must be properly initialized by caller
 *
 * @param fullOid Full forms table ID
 * @param fullCol
 * @param abbrOid Abbreviations table ID
 * @param abbrCol
 *
 * @return Abbreviation dictionary with properly initialized fields
 */
static StringPairRows
_do_calc_dict(const Oid fullOid, const char* fullCol, const Oid abbrOid, const char* abbrCol)
{
    char* fullTable;
    char* abbrTable;

    struct trie* trie;
    char query[4096];

    StringPairRows result = {.size = 0, .read = 0, .pairs = NULL};

    char** abbrs = NULL;
    int abbrs_used = 0;

    char*** pairs = NULL;
    int pairs_used = 0;


    // Process call parameters

    fullTable = get_table_name_by_oid(fullOid);
    abbrTable = get_table_name_by_oid(abbrOid);


    // Create trie

    trie = trie_create();
    if (trie == NULL)
        elog(ERROR, "Could not create trie structure. Not enough memory?");


    // Process abbreviations

    sprintf(query, "SELECT %s FROM %s;", abbrCol, fullTable);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT from abbreviations table '%s' (OID %d).", fullTable, fullOid);
    }
    elog(INFO, "Processing %d rows of abbreviations...", SPI_processed);

    abbrs = palloc(sizeof(*abbrs) * SPI_processed);
    for (int i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        if (row == NULL) {
            continue;
        }
        trie_insert(trie, row, row);
        abbrs[abbrs_used] = row;
        abbrs_used += 1;
    }
    if (abbrs_used == 0) {
        elog(ERROR, "No abbreviations found in given table and column.");
    }

    SPI_freetuptable(SPI_tuptable);


    // Process full names

    sprintf(query, "SELECT %s FROM %s;", fullCol, abbrTable);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT from full forms table '%s' (OID %d).", abbrTable, abbrOid);
    }
    elog(INFO, "Processing %d rows of full forms...", SPI_processed);

    for (int i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);

        char** subsequences = NULL;
        int subsequences_found;

        if (row == NULL) {
            continue;
        }

        subsequences_found = trie_search_subsequences(trie, row, &subsequences);
        elog(DEBUG1, "Found %d abbreviations for row '%s'", subsequences_found, row);
        if (subsequences_found != 0) {
            pairs = pairs == NULL ?
                palloc(subsequences_found * sizeof(*pairs)) :
                repalloc(pairs, (pairs_used + subsequences_found) * sizeof(*pairs));
            for (int s = 0; s < subsequences_found; s++) {
                char** current_pair = palloc(sizeof(*current_pair) * 2);
                current_pair[0] = palloc(strlen(row) + 1);
                strcpy(current_pair[0], row);
                current_pair[1] = palloc(strlen(subsequences[s]) + 1);
                strcpy(current_pair[1], subsequences[s]);
                pfree(subsequences[s]);

                pairs[pairs_used] = current_pair;
                pairs_used += 1;
            }
            pfree(subsequences);
        }

        pfree(row);
    }

    SPI_freetuptable(SPI_tuptable);

    if (pairs_used == 0) {
        elog(WARNING, "No abbreviation rules found");
        return result;
    }


    // Return result

    result.pairs = pairs;
    result.size = pairs_used;

    trie_free(trie);

    elog(DEBUG1, "%ld abbreviations in total", result.size);

    return result;
}


/**
 * @brief Remove repeating pairs (full, abbreviation)
 * from StringPairRows in place
 *
 * @param dict
 */
static void
_remove_duplicate_pairs(StringPairRows* dict)
{
    bool* pairs_to_stay;
    int pairs_to_stay_count = 1;  // At least 1 pair exists, 0 case is eliminated

    if (dict->pairs == NULL || dict->size == 0) {
        return;
    }

    elog(DEBUG1, "Removing duplicates...");

    pg_qsort((void*)dict->pairs, dict->size, sizeof(*dict->pairs), _compare_pair);

    pairs_to_stay = palloc(sizeof(*pairs_to_stay) * dict->size);
    pairs_to_stay[0] = true;
    for (int i = 1; i < dict->size; i++) {

        if (_compare_pair(&dict->pairs[i - 1], &dict->pairs[i]) == 0) {
            pairs_to_stay[i] = false;
        }
        else {
            pairs_to_stay[i] = true;
            pairs_to_stay_count += 1;
        }
    }

    if (pairs_to_stay_count != dict->size) {
        char*** pairs_duplicate;
        int pairs_duplicate_used = 0;

        pairs_duplicate = palloc(sizeof(*pairs_duplicate) * pairs_to_stay_count);
        for (int i = 0; i < dict->size; i++) {
            if (pairs_to_stay[i]) {
                pairs_duplicate[pairs_duplicate_used] = dict->pairs[i];
                pairs_duplicate_used += 1;
            }
            else {
                pfree(dict->pairs[i][0]);
                pfree(dict->pairs[i][1]);
                pfree(dict->pairs[i]);
            }
        }

        pfree(dict->pairs);
        dict->pairs = pairs_duplicate;
        dict->size = pairs_to_stay_count;
    }
}


Datum
calc_dict(PG_FUNCTION_ARGS)
{
    MemoryContext oldcontext;

    FuncCallContext *funcctx;
    TupleDesc tupdesc;
    Datum result;
    StringPairRows* dict;

    if (SRF_IS_FIRSTCALL())
    {
        // Function call parameters
        Oid fullOid;
        Oid abbrOid;
        char* fullCol;
        char* abbrCol;

        StringPairRows calculated;

        // Initialize funcctx
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE) {
            ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED), errmsg("Function returning record called in context that cannot accept type record")));
        }
        funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);
        funcctx->user_fctx = palloc(sizeof(StringPairRows));

        // Load function call parameters
        fullOid = PG_GETARG_OID(0);
        abbrOid = PG_GETARG_OID(2);
        fullCol = get_text_parameter(PG_GETARG_TEXT_P(1));
        abbrCol = get_text_parameter(PG_GETARG_TEXT_P(3));

        // Calculate abbreviation dictionary
        SPI_connect();
        calculated = _do_calc_dict(fullOid, fullCol, abbrOid, abbrCol);
        SPI_finish();
        Assert(calculated.pairs != NULL);
        _remove_duplicate_pairs(&calculated);

        // Copy abbreviation dictionary into new memory context
        funcctx->user_fctx = palloc(sizeof(StringPairRows));
        dict = funcctx->user_fctx;
        dict->size = calculated.size;
        dict->read = 0;
        dict->pairs = palloc(sizeof(*dict->pairs) * calculated.size);
        for (int i = 0; i < calculated.size; i++) {
            dict->pairs[i] = palloc(sizeof(*dict->pairs[i]) * 2);
            for (int j = 0; j < 2; j++) {
                dict->pairs[i][j] = palloc(strlen(calculated.pairs[i][j]) + 1);
                strcpy(dict->pairs[i][j], calculated.pairs[i][j]);
            }
        }

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    dict = funcctx->user_fctx;

    if (dict->read >= dict->size) {
        SRF_RETURN_DONE(funcctx);
    }

    result = HeapTupleGetDatum(BuildTupleFromCStrings(funcctx->attinmeta, dict->pairs[dict->read]));
    dict->read += 1;

    SRF_RETURN_NEXT(funcctx, result);
}
