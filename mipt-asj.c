#include "mipt-asj.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


PG_FUNCTION_INFO_V1(select_from_ext_table);
Datum
select_from_ext_table(PG_FUNCTION_ARGS)
{
	int32 arg = PG_GETARG_INT32(0);

    PG_RETURN_INT32(arg * 2);
}
