\echo Use "CREATE EXTENSION mipt-asj" to load this file. \quit

-- Create schema and set search path
CREATE SCHEMA mipt_asj;

-- Create asj_metadata
CREATE TABLE mipt_asj.asj_metadata(relation_kind VARCHAR, last_id INT);
INSERT INTO mipt_asj.asj_metadata VALUES
('dict', 0),
('pairs', 0);

-- Train functions
CREATE FUNCTION mipt_asj.calc_dict(oid, TEXT, oid, TEXT) RETURNS INT
    AS 'MODULE_PATHNAME', 'asj_calc_dict'
    LANGUAGE C
    VOLATILE;
CREATE FUNCTION mipt_asj.calc_pairs(INT, oid, TEXT, oid, TEXT, REAL) RETURNS INT
    AS 'MODULE_PATHNAME', 'asj_calc_pairs'
    LANGUAGE C
    VOLATILE;

-- asj comparator function
CREATE FUNCTION mipt_asj.cmp(INT, TEXT, TEXT) RETURNS BOOLEAN
    AS 'MODULE_PATHNAME', 'asjcmp'
    LANGUAGE C
    VOLATILE;
