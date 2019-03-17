/*
 * cmp.c
 *      pkduck comparator. Part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/cmp.c
 */

#include "cmp.h"


/**
 * @brief A rule with sequenized tokens
 */
typedef struct {
    /// Applicable side
    TokenSequence a;
    /// Result side
    TokenSequence r;
} Rule;


typedef struct {
    unsigned long size;
    /// Rules
    Rule* rules;
} RuleSequence;


/**
 * @brief Apply a RuleSequence for 's1'
 *
 * @param s1 SORTED (set-like) TokenSequence
 * @param s2 SORTED (set-like) TokenSequence
 * @param rule SORTED (set-like) Rule object
 *
 * @param modify_sequences Modify s1 and s2 (actually apply rule)
 * @param shared Pointer to storage for number of shared tokens to increment.
 *      Will be changed only if 'modify_sequences' is set
 * @param thrown Pointer to storage for number of "thrown-away" rside tokens to increment.
 *      Will be changed only if 'modify_sequences' is set
 *
 * @return pkduck metric for this rule
 * @return -1 if rule does not applies
 */
static double
_apply_rule(TokenSequence* s1, TokenSequence* s2, Rule rule, bool modify_sequences, unsigned long* tokens_shared, unsigned long* tokens_thrown)
{
    double usefullness;

    // Check rule applies
    {
        unsigned long rule_i = 0;
        for (unsigned long i = 0; i < s1->size; i++) {
            if (strcmp(rule.a.ts[rule_i], s1->ts[i]) == 0) {
                rule_i += 1;
                if (rule_i == rule.a.size) {
                    break;
                }
            }
        }
        if (rule_i != rule.a.size) {
            // Rule does not apply
            return -1.0f;
        }
    }

    // Calculate usefullness and modify s1, s2, if asked to
    {
        unsigned long common_tokens = 0;
        double rsize = rule.r.size;

        for (unsigned long i_rule = 0; i_rule < rule.r.size; i_rule++) {
            for (long i_s2 = 0; i_s2 < s2->size; i_s2++) {
                if (strcmp(rule.r.ts[i_rule], s2->ts[i_s2]) == 0) {
                    common_tokens += 1;
                    if (modify_sequences) {
                        remove_from_token_sequence(s2, i_s2);
                        i_s2 -= 1;
                    }
                    break;
                }
            }
        }

        usefullness = common_tokens / rsize;

        if (modify_sequences) {
            unsigned long rule_i = 0;
            for (long i = 0; i < s1->size; i++) {
                if (strcmp(rule.a.ts[rule_i], s1->ts[i]) == 0) {
                    rule_i += 1;
                    remove_from_token_sequence(s1, i);
                    i -= 1;
                    if (rule_i == rule.a.size) {
                        break;
                    }
                }
            }
            *tokens_shared += common_tokens;
            *tokens_thrown += rule.r.size - common_tokens;
        }
    }

    return usefullness;
}


/**
 * @brief Calculate pkduck for two sequences given
 *
 * @param s1_ptr
 * @param s2_ptr
 * @param rules SORTED (set-like) rules' sequence
 *
 * @return pkduck metric for two token sequences
 */
static double
_pkduck(TokenSequence* s1, TokenSequence* s2, const RuleSequence* rules_ptr)
{
    /// Number of tokens which appear after rule application and are equal to tokens in s2
    unsigned long tokens_similar = 0;
    /// Number of tokens which appear after rule application, but do not equal to tokens in s2
    unsigned long tokens_thrown = 0;
    /// Number of tokens common for s1 and s2, after all rules' applications
    unsigned long tokens_shared = 0;

    //
    for (unsigned long k = 0; k < s1->size; k++) {
        elog(DEBUG1, "WORD S1: %s", s1->ts[k]);
    }
    //
    //
    for (unsigned long k = 0; k < s2->size; k++) {
        elog(DEBUG1, "WORD S2: %s", s2->ts[k]);
    }
    //

    // Apply rules
    while (true) {
        double max_usefullness = -0.5f;
        size_t max_index = 0;
        // Look for the best rule
        for (size_t rule_i = 0; rule_i < rules_ptr->size; rule_i++) {
            double curr_usefullness = _apply_rule(s1, s2, rules_ptr->rules[rule_i], false, NULL, NULL);
            if (curr_usefullness > max_usefullness) {
                max_usefullness = curr_usefullness;
                max_index = rule_i;
            }
        }
        // Check exit condition
        if (max_usefullness < 0.0f) {
            break;
        }
        // Apply best rule
        _apply_rule(s1, s2, rules_ptr->rules[max_index], true, &tokens_similar, &tokens_thrown);
    }

    // Calculate 'tokens_shared'
    for (long s1_i = 0; s1_i < s1->size; s1_i++) {
        for (long s2_i = 0; s2_i < s2->size; s2_i++) {
            if (strcmp(s1->ts[s1_i], s2->ts[s2_i]) == 0) {
                tokens_shared += 1;
                remove_from_token_sequence(s1, s1_i);
                s1_i -= 1;
                remove_from_token_sequence(s2, s2_i);
                s2_i -= 1;
                break;
            }
        }
    }

    {
        // Common tokens were calculated above
        double jaccard_common = tokens_similar + tokens_shared;
        // By this moment, we know s1 and s2 do not contain equal tokens, and all rules were applied
        // 'tokens_thrown' is basically number of "remains" of rule applications
        double jaccard_total = jaccard_common + s1->size + s2->size + tokens_thrown;
        elog(DEBUG1, "Jaccard: %f / %f ", jaccard_common, jaccard_total);
        elog(DEBUG1, "Similar: %lu, Shared: %lu.", tokens_similar, tokens_shared);
        elog(DEBUG1, "s1.size: %lu, s2.size: %lu, thrown: %lu", s1->size, s2->size, tokens_thrown);

        return jaccard_common / jaccard_total;
    }
}


/**
 * @brief Compare two strings given
 *
 * @param s1
 * @param s2
 * @param rules_ptr
 * @param exactness
 */
static bool
_do_cmp(const char* string1, const char* string2, double exactness, Oid tRoid, const char* tRcol_abbr, const char* tRcol_full)
{
    char* tR;

    char query[4096];

    RuleSequence rs;

    TokenSequence s1;
    TokenSequence s2;
    double pkduck;


    // Process call parameters

    tR = get_table_name_by_oid(tRoid);


    // Fill rules

    sprintf(query, "SELECT %s, %s FROM %s;", tRcol_abbr, tRcol_full, tR);
    if (SPI_execute(query, true, 0) < 0 || SPI_tuptable == NULL) {
        elog(ERROR, "Could not SELECT rows from table '%s' (OID %d).", tR, tRoid);
    }
    rs = (RuleSequence){
        0,
        palloc(sizeof(*rs.rules) * SPI_processed * 2)
    };
    for (uint32 i = 0; i < SPI_processed; i++) {
        char* temp[2];
        TokenSequence rule_abbr_seq;
        TokenSequence rule_full_seq;

        temp[0] = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
        temp[1] = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 2);
        if (temp[0] == NULL || temp[1] == NULL) {
            continue;
        }

        // Sequences
        rule_abbr_seq = tokenize(temp[0], " ");
        pg_qsort(rule_abbr_seq.ts, rule_abbr_seq.size, sizeof(*rule_abbr_seq.ts), cmp_tokens_wrapper);
        rule_full_seq = tokenize(temp[1], " ");
        pg_qsort(rule_full_seq.ts, rule_full_seq.size, sizeof(*rule_full_seq.ts), cmp_tokens_wrapper);

        // abbr -> full
        rs.rules[rs.size].a = rule_abbr_seq;
        rs.rules[rs.size].r = rule_full_seq;
        rs.size += 1;
        // full -> abbr
        rs.rules[rs.size].a = rule_full_seq;
        rs.rules[rs.size].r = rule_abbr_seq;
        rs.size += 1;
    }
    // Shorten rules array, if necessary
    if (rs.size != SPI_processed * 2) {
        rs.rules = repalloc(rs.rules, sizeof(*rs.rules) * rs.size);
    }
    SPI_freetuptable(SPI_tuptable);


    // Apply rules

    s1 = tokenize(string1, " ");
    pg_qsort(s1.ts, s1.size, sizeof(*s1.ts), cmp_tokens_wrapper);
    s2 = tokenize(string2, " ");
    pg_qsort(s2.ts, s2.size, sizeof(*s2.ts), cmp_tokens_wrapper);

    pkduck = _pkduck(&s1, &s2, &rs);

    if (pkduck - exactness > 0.0f) {
        return true;
    }
    return false;
}


Datum
cmp(PG_FUNCTION_ARGS)
{
    char* string1;
    char* string2;
    Oid tRoid;
    char* tRcol_full;
    char* tRcol_abbr;
    double exactness;

    bool result;

    string1 = get_text_parameter(PG_GETARG_TEXT_P(0));
    string2 = get_text_parameter(PG_GETARG_TEXT_P(1));
    tRoid = PG_GETARG_OID(2);
    tRcol_full = get_text_parameter(PG_GETARG_TEXT_P(3));
    tRcol_abbr = get_text_parameter(PG_GETARG_TEXT_P(4));
    exactness = PG_GETARG_FLOAT4(5);

    SPI_connect();
    result = _do_cmp(string1, string2, exactness, tRoid, tRcol_abbr, tRcol_full);
    SPI_finish();

    return result;
}
