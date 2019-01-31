/*
 * mipt-asj.c
 *      Tao-Deng-Stonebraker algorithm of approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	  contrib/mipt-asj/mipt-asj.c
 */

#include "postgres.h"
#include "fmgr.h"

#include "executor/spi.h"
#include "utils/builtins.h"

#include "lib/trie.h"


PG_FUNCTION_INFO_V1(asj_calc_dict);
PG_FUNCTION_INFO_V1(asj_calc_pairs);
PG_FUNCTION_INFO_V1(asjcmp);
