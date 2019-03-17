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


typedef struct {
    /// Applicable side
    TokenSequence a;
    /// Result side
    TokenSequence r;
} RuleSequence;


/**
 * @brief Calculate pkduck() for two sequences given
 *
 * @return true if the given sequences match with given exactness
 */
static bool
_pkduck_cmp(const TokenSequence s1, const TokenSequence s2, const RuleSequence* rules, double exactness)
{
    return false;
}


Datum
cmp(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(false);
}
