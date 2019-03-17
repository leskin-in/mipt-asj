#include "mipt-asj.h"

#include "postgres.h"
#include "fmgr.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


PG_FUNCTION_INFO_V1(calc_dict);
PG_FUNCTION_INFO_V1(calc_pairs);
PG_FUNCTION_INFO_V1(cmp);

