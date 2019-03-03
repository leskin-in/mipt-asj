\echo Use "CREATE EXTENSION mipt-asj" to load this file. \quit

-- Create schema and set search path
CREATE SCHEMA mipt_asj;


-- calc_dict
CREATE OR REPLACE FUNCTION
    mipt_asj.calc_dict(oid, TEXT, oid, TEXT)
    RETURNS TABLE(full VARCHAR, abbr VARCHAR)
    AS 'MODULE_PATHNAME', 'calc_dict'
    LANGUAGE C
    VOLATILE;


-- calc_pairs
CREATE OR REPLACE FUNCTION
    mipt_asj.calc_pairs(INT, oid, TEXT, oid, TEXT, REAL)
    RETURNS INT
    AS 'MODULE_PATHNAME', 'asj_calc_pairs'
    LANGUAGE C
    VOLATILE;


-- cmp
CREATE OR REPLACE FUNCTION
    mipt_asj.cmp(INT, TEXT, TEXT)
    RETURNS BOOLEAN
    AS 'MODULE_PATHNAME', 'asjcmp'
    LANGUAGE C
    VOLATILE;
