#ifndef CALC_PAIRS_H
#define CALC_PAIRS_H

/*
 * calc_pairs.h
 *      String filtering-out module, part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/calc_pairs.h
 */

#include <string.h>

#include "postgres.h"
#include "fmgr.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "funcapi.h"

#include "lib/common.h"


/**
 * @brief Filter out strings that could be joined using TDS algorithm
 *
 * @param 0: 1st string set table OID
 * @param 1: 1st string set table column
 *
 * @param 2: 2nd string set table OID
 * @param 3: 2nd string set table column
 *
 * @param 4: Abbreviation dictionary OID
 * @param 5: Abbreviation dictionary column 'full'
 * @param 6: Abbreviation dictionary column 'abbr'
 *
 * @param 7: Exactness
 *
 * Returns table (see SQL definition)
 */
Datum calc_pairs(PG_FUNCTION_ARGS);


#endif /* CALC_PAIRS_H */
