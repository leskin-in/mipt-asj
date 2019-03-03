#include "mipt-asj.h"

#include "postgres.h"
#include "fmgr.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/**
 *
 */
Datum
asj_calc_pairs(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(-1);
}

/**
 *
 */
Datum
asjcmp(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(false);
}
