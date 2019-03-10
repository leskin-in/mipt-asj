#ifndef CALC_DICT_H
#define CALC_DICT_H

/*
 * calc_dict.h
 *      Abbreviation dictionary calculation, part of
 *      Tao-Deng-Stonebraker algorithm for
 *      approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	    contrib/mipt-asj/asj/calc_dict.h
 */

#include "postgres.h"
#include "fmgr.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "funcapi.h"

#include "lib/common.h"
#include "lib/trie.h"


/**
 * @brief Calculate abbreviation dictionary from two columns in PostgreSQL
 *
 * @param 0: OID of full names table
 * @param 1: column of full names table
 * @param 2: OID of abbreviations table
 * @param 3: column of abbreviations table
 *
 * Returns table (see SQL definition)
 */
Datum calc_dict(PG_FUNCTION_ARGS);


#endif /* CALC_DICT_H */
