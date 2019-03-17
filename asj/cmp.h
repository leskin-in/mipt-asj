#ifndef CMP_H
#define CMP_H

/*
 * cmp.h
 *      pkduck comparator. Part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/cmp.h
 */

#include <string.h>

#include "postgres.h"
#include "fmgr.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "funcapi.h"

#include "lib/common.h"


/**
 * @brief ASJ comparator function. Calculates pkduck()
 * for every pair of input received
 *
 * @return true | false, as any equality function
 */
Datum cmp(PG_FUNCTION_ARGS);


#endif /* CMP_H */
