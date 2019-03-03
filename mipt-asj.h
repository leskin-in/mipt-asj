/*
 * mipt-asj.c
 *      Tao-Deng-Stonebraker algorithm of approximate string JOINs with abbreviations
 *
 * IDENTIFICATION
 *	  contrib/mipt-asj/mipt-asj.h
 */

#include "postgres.h"
#include "fmgr.h"

#include "asj/calc_dict.h"

PG_FUNCTION_INFO_V1(calc_dict);
PG_FUNCTION_INFO_V1(asj_calc_pairs);
PG_FUNCTION_INFO_V1(asjcmp);
