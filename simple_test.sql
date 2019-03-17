-- Set
SET log_min_messages = 'DEBUG1';
SET client_min_messages = 'INFO';


-- Install extension
DROP EXTENSION IF EXISTS "mipt-asj";
CREATE EXTENSION "mipt-asj";


-- Data for calc_dict
DROP TABLE IF EXISTS ddata;
CREATE TABLE ddata(f VARCHAR, a VARCHAR);

INSERT INTO ddata(f, a) VALUES
('moscow institute of physics and technology', 'mipt'),
('moscow metro', 'mosmetro'),
('moscow metro', 'mm'),
('moscow institute of physics and technology', 'phystech'),
(NULL, 'mosmet'),
(NULL, 'metro'),
(NULL, 'volosy'),
(NULL, 'mgu'),
(NULL, 'msu'),
('kolobok production', NULL);

DROP TABLE IF EXISTS rules;
CREATE TABLE rules(f VARCHAR, a VARCHAR);


-- calc_dict

INSERT INTO rules(f, a) (
	SELECT * FROM mipt_asj.calc_dict(
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'ddata'), 'f',
		(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'ddata'), 'a'
	)
);
SELECT * FROM rules;


-- Data for calc_pairs
DROP TABLE IF EXISTS pdata;
CREATE TABLE pdata(c1 VARCHAR, c2 VARCHAR);

INSERT INTO pdata(c1, c2) VALUES
('mipt mosmetro', NULL),
('moscow metro hall', NULL),
(NULL, 'moscow institute of physics and technology moscow metro'),
(NULL, 'mm');


-- calc_pairs

SELECT * FROM mipt_asj.calc_pairs(
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'pdata'), 'c1',
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'pdata'), 'c2',
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'rules'), 'f', 'a',
	0.7
);
