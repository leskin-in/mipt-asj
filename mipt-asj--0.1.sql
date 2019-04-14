\echo Use "CREATE EXTENSION mipt-asj" to load this file. \quit

-- Create schema and set search path
CREATE SCHEMA mipt_asj;


-- Calculate abbreviation dictionary.
-- #1, #2:      Full names table OID and column
-- #3, #4:      Abbreviations table OID and column
-- Return:      Abbreviation dictionary
CREATE OR REPLACE FUNCTION
    mipt_asj.calc_dict(oid, TEXT, oid, TEXT)
    RETURNS TABLE(f VARCHAR, a VARCHAR)
    AS 'MODULE_PATHNAME', 'calc_dict'
    LANGUAGE C
    VOLATILE;


-- Filter out pairs of strings that could be joined
-- #1, #2:      First string set table OID and column
-- #3, #4:      Second string set table OID and column
-- #5, #6, #7:  Abbreviation dictionary table OID, 'full' and 'abbr' column
-- #8:          Exactness parameter
-- Return:      Set of pairs (#1#2, #3#4)
CREATE OR REPLACE FUNCTION
    mipt_asj.calc_pairs(oid, TEXT, oid, TEXT, oid, TEXT, TEXT, REAL)
    RETURNS TABLE(s1 VARCHAR, s2 VARCHAR)
    AS 'MODULE_PATHNAME', 'calc_pairs'
    LANGUAGE C
    VOLATILE;


-- Compare pairs in JOIN
-- #1, #2:      Strings to compare
-- #3, #4, #5:  Abbreviation dictionary table OID, 'full' and 'abbr' column
-- #6:          Exactness parameter
-- Return:      boolean
CREATE OR REPLACE FUNCTION
    mipt_asj.cmp(TEXT, TEXT, oid, TEXT, TEXT, REAL)
    RETURNS BOOLEAN
    AS 'MODULE_PATHNAME', 'cmp'
    LANGUAGE C
    VOLATILE;
