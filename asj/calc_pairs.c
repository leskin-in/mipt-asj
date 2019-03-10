/*
 * calc_pairs.c
 *      String filtering-out module, part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/calc_pairs.c
 */

#include "calc_pairs.h"


typedef struct {
    unsigned long size;
    /**
     * Token values
     */
    char** ts;
} TokenSequence;


typedef struct {
    unsigned long size;
    /**
     * Rules
     */
    char*** rs;
} RuleSequence;


/**
 * Result of rule application
 */
typedef struct {
    bool applies;
    unsigned long aside;
    unsigned long rside;
} SubRuleApplication;


typedef struct {
    /**
     * Abbreviation-to-full subrule
     */
    SubRuleApplication a_f;
    /**
     * Full-to-abbreviation subrule
     */
    SubRuleApplication f_a;
    /**
     * Raw rule pointer
     */
    const char** rule;
} RuleApplication;


/**
 * @brief Compare token sequences using special ("reverse") metric
 */
static inline int
_cmp_tokens(const char* t1, const char* t2)
{
    return strlen(t1) == strlen(t2) ? strcmp(t1, t2) : strlen(t2) - strlen(t1);
}


/**
 * @brief Wrapper around '_cmp_tokens' for qsort
 */
static int
_cmp_tokens_wrapper(const void* a, const void* b) {
    return _cmp_tokens(*(const char**)a, *(const char**)b);
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
static TokenSequence
_tokenize(const char* string, const char* delim)
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


/**
 * @brief Calculate prefix signature length
 *
 * @param tokens_total total number of tokens in a string whose prefix signature is being calculated
 * @param exactness
 */
static inline unsigned long
_prefix_sig_length(unsigned long tokens_total, double exactness)
{
    return (unsigned long)(floor((1.0f - exactness) * tokens_total)) + 1;
}


/**
 * @brief Calculate prefix signature of a given string
 *
 * @param string
 * @param exactness
 * @return TokenSequence
 */
static TokenSequence
_prefix_sig(const char* string, double exactness)
{
    TokenSequence temp;
    TokenSequence result = {0, NULL};

    temp = _tokenize(string, " ");
    pg_qsort((void*)temp.ts, temp.size, sizeof(*temp.ts), _cmp_tokens_wrapper);

    result.size = _prefix_sig_length(temp.size, exactness);
    result.size = Min(result.size, temp.size);
    result.ts = palloc(sizeof(*result.ts) * result.size);
    for (unsigned long i = 0; i < result.size; i++) {
        result.ts[i] = temp.ts[i];
    }
    for (unsigned long i = result.size; i < temp.size; i++) {
        pfree(temp.ts[i]);
    }
    pfree(temp.ts);

    return result;
}


/**
 * @brief Try to apply a rule to given TokenSequence
 *
 * @param rule rule to apply
 * @param ts token sequence
 * @param ts_endpos position where applicable rule must end
 * @return RuleApplication
 */
static RuleApplication
_rule_apply(const char** rule, TokenSequence ts, unsigned long ts_endpos)
{
    RuleApplication result = (RuleApplication){
        (SubRuleApplication){false, 0, 0},
        (SubRuleApplication){false, 0, 0},
        rule
    };
    const char* rule_A = rule[0];
    const char* rule_F = rule[1];
    TokenSequence rule_F_tokenized;
    bool f_a_applies = true;

    // Correct input parameter
    ts_endpos = ts_endpos >= ts.size ? ts.size - 1 : ts_endpos;

    // Tokenize full rule
    rule_F_tokenized = _tokenize(rule_F, " ");

    // Check application of a_f rule
    if (strcmp(rule_A, ts.ts[ts_endpos]) == 0) {
        result.a_f = (SubRuleApplication){
            true,
            1,
            rule_F_tokenized.size
        };
    }

    // Check application of f_a rule
    for (int i = 0; i < rule_F_tokenized.size; i++) {
        int ts_currpos = ts_endpos - (rule_F_tokenized.size - 1) + i;
        if (ts_currpos < 0) {
            f_a_applies = false;
            break;
        }
        if (strcmp(rule_F_tokenized.ts[i], ts.ts[ts_currpos]) != 0) {
            f_a_applies = false;
            break;
        }
    }
    if (f_a_applies) {
        result.f_a = (SubRuleApplication) {
            true,
            rule_F_tokenized.size,
            1
        };
    }

    return result;
}


/**
 * @brief Calculate g-function for given parameters
 *
 * g-function is the least number of tokens, smaller than t, in string:
 *      * derived from given string s
 *      * of length exactly l tokens
 *      * containing at least one token t
 *
 * @param s token sequence
 * @param i start position to apply rules to.
 *      Must be set to 's.size - 1' when first called
 * @param l token length of string, derived from given s
 * @param t token to check
 * @param t_present flag that t was found in s
 *      Must be set to false when first called, and checked after function return
 * @param rules abbreviation rules
 *
 * @return value of g-function, or INT_MAX if t was not found
 * @param t will be set to true, if t was found
 */
static long
_calculate_g(TokenSequence s, long i, long l, const char* t, bool* t_present, RuleSequence rules)
{
    long result = (long)INT_MAX;
    long result_current;

    // Check return conditions
    if (i < 0) {
        elog(WARNING, "Exit on 'i < 0'");
        return result;
    }
    if (l < 0) {
        elog(WARNING, "Exit on 'l < 0'");
        return result;
    }
    if (l == 0) {
        elog(WARNING, "Exit on 'l == 0'");
        return result;
    }
    if (i == 0 && l == 0) {
        elog(WARNING, "Exit on 'i == 0 and l == 0'");
        return result;
    }

    // Calculate case 1: current token has no rules that apply to it
    {
        int comparation_result;
        comparation_result = _cmp_tokens(s.ts[i >= s.size ? s.size - 1 : i], t);
        if (comparation_result > 0) {
            result_current = _calculate_g(s, i - 1, l - 1, t, t_present, rules);
        }
        else if (comparation_result < 0) {
            result_current = _calculate_g(s, i - 1, l - 1, t, t_present, rules) + 1;
        }
        else {
            *t_present = true;
            result_current = _calculate_g(s, i - 1, l - 1, t, t_present, rules);
        }
    }

    if (*t_present) {
        result = Min(result, result_current);
    }

    // Calculate case 2: current token is end of some rule
    {
        result_current = (long)INT_MAX;

        for (unsigned long j = 0; j < rules.size; j++) {
            RuleApplication ra;

            ra = _rule_apply((const char**)rules.rs[j], s, i);

            // a_f recursion
            if (ra.a_f.applies) {
                TokenSequence rule_ts;
                int ts_less = 0;

                rule_ts = _tokenize(ra.rule[1], " ");

                for (int t_i = 0; t_i < rule_ts.size; t_i++) {
                    int comparation_result = _cmp_tokens(rule_ts.ts[t_i], t);
                    if (comparation_result == 0) {
                        *t_present = true;
                    }
                    if (comparation_result < 0) {
                        ts_less += 1;
                    }
                }

                result_current = Min(
                    result_current,
                    _calculate_g(s, i - ra.a_f.aside, l - ra.a_f.rside, t, t_present, rules) + ts_less
                );
            }

            // f_a recursion
            if (ra.f_a.applies) {
                int ts_less = 0;
                int comparation_result;

                comparation_result = _cmp_tokens(ra.rule[0], t);
                if (comparation_result == 0) {
                    *t_present = true;
                }
                if (comparation_result < 0) {
                    ts_less += 1;
                }

                result_current = Min(
                    result_current,
                    _calculate_g(s, i - ra.a_f.aside, l - ra.a_f.rside, t, t_present, rules) + ts_less
                );
            }
        }
    }

    if (*t_present) {
        result = Min(result, result_current);
    }

    return result;
}


/**
 * @brief Comparator for pairs (unsigned long, unsigned long)
 */
static int
_cmp_ulong_pair_wrapped(const void* a, const void* b)
{
    unsigned long vA[2];
    unsigned long vB[2];

    for (unsigned char j = 0; j < 2; j++) {
        vA[j] = (*(const unsigned long**)a)[j];
        vB[j] = (*(const unsigned long**)b)[j];
    }

    return vA[0] == vB[0] ? vA[1] - vB[1] : vA[0] - vB[0];
}


/**
 * @brief Remove repeating pairs of joins in-place
 *
 * @param joins_ptr
 * @param joins_size
 * @return new joins_size
 */
static unsigned long
_remove_duplicate_joins(unsigned long*** joins_ptr, unsigned long joins_size)
{
    unsigned long** joins = *joins_ptr;

    bool* joins_to_stay;
    unsigned long joins_to_stay_count;
    unsigned long joins_to_stay_insert_index;

    // Check call parameters
    if (joins == NULL || joins_size == 0) {
        return joins_size;
    }

    elog(DEBUG1, "Removing duplicates...");

    pg_qsort(joins, joins_size, sizeof(*joins), _cmp_ulong_pair_wrapped);

    // Select joins to stay
    joins_to_stay = palloc(sizeof(*joins_to_stay) * joins_size);
    joins_to_stay[0] = true;
    joins_to_stay_count = 1;
    for (unsigned long i = 1; i < joins_size; i++) {
        if (_cmp_ulong_pair_wrapped(&joins[i - 1], &joins[i]) == 0) {
            joins_to_stay[i] = false;
        }
        else {
            joins_to_stay[i] = true;
            joins_to_stay_count += 1;
        }
    }

    // Remove duplicates
    if (joins_to_stay_count != joins_size) {
        joins_to_stay_insert_index = 0;
        for (int i = 0; i < joins_size; i++) {
            if (joins_to_stay[i]) {
                joins[joins_to_stay_insert_index] = joins[i];
                joins_to_stay_insert_index += 1;
            }
            else {
                pfree(joins[i]);
            }
        }
        joins = repalloc(joins, joins_to_stay_count);
        *joins_ptr = joins;
    }

    return joins_to_stay_count;
}


/**
 * @brief Calculate pair rows to be joined
 *
 * @param t1oid table 1 OID
 * @param t1col table 1 column name
 *
 * @param t2oid table 2 OID
 * @param t2col table 2 column name
 *
 * @param tRoid rules table OID
 * @param tRcol_abbr rules table abbreviations column name
 * @param tRcol_full rules table full forms column name
 *
 * @param exactness
 *
 * @return StringPairRows
 */
static StringPairRows
_do_calc_pairs(Oid t1oid, const char* t1col, Oid t2oid, const char* t2col, Oid tRoid, const char* tRcol_abbr, const char* tRcol_full, double exactness)
{
    char* t1;
    char* t2;
    char* tR;

    char query[4096];

    char** rows[2];
    TokenSequence* rows_signatures[2];
    unsigned long rows_used[2] = {0, 0};

    RuleSequence rules = {0, NULL};

    unsigned long** joins;
    unsigned long joins_used = 0;

    StringPairRows results;


    // Process call parameters

    t1 = get_table_name_by_oid(t1oid);
    t2 = get_table_name_by_oid(t2oid);
    tR = get_table_name_by_oid(tRoid);


    // Fill rows

    sprintf(query, "SELECT %s FROM %s;", t1col, t1);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT rows from table '%s' (OID %d).", t1, t1oid);
    }
    elog(INFO, "Processing %d rows...", SPI_processed);
    rows[0] = palloc(sizeof(*rows[0]) * SPI_processed);
    for (uint32 i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        if (row == NULL) {
            continue;
        }
        rows[0][rows_used[0]++] = row;
    }
    SPI_freetuptable(SPI_tuptable);

    sprintf(query, "SELECT %s FROM %s;", t2col, t2);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT rows from table '%s' (OID %d).", t2, t2oid);
    }
    elog(INFO, "Processing %d rows...", SPI_processed);
    rows[1] = palloc(sizeof(*rows[1]) * SPI_processed);
    for (uint32 i = 0; i < SPI_processed; i++) {
        char* row = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        if (row == NULL) {
            continue;
        }
        rows[1][rows_used[1]++] = row;
    }
    SPI_freetuptable(SPI_tuptable);


    // Fill rules

    sprintf(query, "SELECT %s, %s FROM %s;", tRcol_abbr, tRcol_full, tR);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT rows from table '%s' (OID %d).", tR, tRoid);
    }
    elog(INFO, "Processing %d rows...", SPI_processed);
    rules.rs = palloc(sizeof(*rules.rs) * SPI_processed);
    for (uint32 i = 0; i < SPI_processed; i++) {
        char* temp[2];
        temp[0] = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        temp[1] = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 2);
        if (temp[0] == NULL || temp[1] == NULL) {
            continue;
        }
        rules.rs[rules.size] = palloc(sizeof(rules.rs[rules.size]) * 2);
        for (unsigned char j = 0; j < 2; j++) {
            rules.rs[rules.size][j] = temp[j];
        }
        rules.size += 1;
    }
    SPI_freetuptable(SPI_tuptable);


    // Calculate prefix signatures for every row

    for (unsigned char j = 0; j < 2; j++) {
        rows_signatures[j] = palloc(sizeof(*rows_signatures[j]) * rows_used[j]);
        for (unsigned long i = 0; i < rows_used[j]; i++) {
            rows_signatures[j][i] = _prefix_sig(rows[j][i], exactness);
        }
    }


    // Calculate joins

    joins = palloc(sizeof(*joins) * rows_used[0] * rows_used[1]);
    for (unsigned long i = 0; i < rows_used[0] * rows_used[1]; i++) {
        joins[i] = palloc(sizeof(*joins[i]) * 2);
    }

    // 1. Check if prefix signature of every row from rows[0] intersects with U-signature of any row from rows[1]
    // 2. Do the same, but for rows[1] and rows[0], respectively
    for (unsigned char j = 0; j < 2; j++) {
        const unsigned char ROW_PF_INDEX = j;
        const unsigned char ROW_U_INDEX = 1 - j;

        for (unsigned long pf_i = 0; pf_i < rows_used[ROW_PF_INDEX]; pf_i++) {
            TokenSequence seq = rows_signatures[ROW_PF_INDEX][pf_i];
            for (unsigned long token_i = 0; token_i < seq.size; token_i++) {
                const char* token = seq.ts[token_i];
                for (unsigned long u_i = 0; u_i < rows_used[ROW_U_INDEX]; u_i++) {
                    TokenSequence seq_u = rows_signatures[ROW_U_INDEX][u_i];
                    // XXX: Correct l calculation required here
                    for (long l = 1; l < seq_u.size + 5; l++) {
                        bool t_present = false;
                        long g = _calculate_g(seq_u, seq_u.size - 1, l, token, &t_present, rules);
                        if (t_present && (g + 1 <= _prefix_sig_length(l, exactness))) {
                            joins[joins_used][ROW_PF_INDEX] = pf_i;
                            joins[joins_used][ROW_U_INDEX] = u_i;
                            joins_used += 1;
                        }
                    }
                }
            }
        }
    }


    // Remove duplicate joins

    joins_used = _remove_duplicate_joins(&joins, joins_used);


    // Build results and return

    results.size = joins_used;
    results.read = 0;
    results.pairs = palloc(sizeof(*results.pairs) * results.size);
    for (unsigned long i = 0; i < joins_used; i++) {
        results.pairs[i] = palloc(sizeof(*results.pairs[results.read]) * 2);
        for (unsigned char j = 0; j < 2; j++) {
            results.pairs[i][j] = rows[j][joins[i][j]];
        }
    }

    return results;
}


Datum
calc_pairs(PG_FUNCTION_ARGS)
{
    // // Function call parameters
    // Oid fullOid;
    // Oid abbrOid;
    // char* fullCol;
    // char* abbrCol;

    // // Load function call parameters
    // fullOid = PG_GETARG_OID(0);
    // abbrOid = PG_GETARG_OID(2);
    // fullCol = get_text_parameter(PG_GETARG_TEXT_P(1));
    // abbrCol = get_text_parameter(PG_GETARG_TEXT_P(3));

    // Calculate abbreviation dictionary
    SPI_connect();

    PG_RETURN_INT32(-1);
}
