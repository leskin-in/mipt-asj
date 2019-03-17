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
#include "asj/calc_pairs.h"
#include "asj/cmp.h"

