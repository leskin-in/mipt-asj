#include "mipt-asj.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


/**
 * Calculate abbreviation dictionary and place it into 'dict_X'
 */
Datum
asj_calc_dict(PG_FUNCTION_ARGS)
{
    // Constants
    static const LOCKMODE lockMode = AccessShareLock;

    // Function arguments
    Oid left = PG_GETARG_OID(0);
    const char *leftCol = VARDATA(PG_GETARG_TEXT_P(1));
    Oid right = PG_GETARG_OID(2);
    const char *rightCol = VARDATA(PG_GETARG_TEXT_P(3));

    // Local variables

    Relation left = heap_open(left, lockMode);
    if (!HeapScanIsValid(left)) {
        elog(ERROR, "Could not open the table with provided OID");
    }
    HeapScanDesc leftDesc = heap_beginscan(left, SnapshotSelf, 0, (ScanKey)NULL);
    for (;;) {
        HeapTuple tuple;
    }



    PG_RETURN_INT32(-1);
}

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
