\echo Use "CREATE EXTENSION mipt-asj" to load this file. \quit


CREATE SCHEMA mipt_asj;

CREATE TABLE mipt_asj.dictionary (id INT);

CREATE FUNCTION select_from_ext_table(INT) RETURNS INT
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT;
