SET log_min_messages = 'DEBUG1';
SET client_min_messages = 'DEBUG1';

DROP EXTENSION IF EXISTS "mipt-asj";
CREATE EXTENSION "mipt-asj";

DROP TABLE IF EXISTS tdata;
CREATE TABLE tdata(f VARCHAR, a VARCHAR);

INSERT INTO tdata(f, a) VALUES
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

SELECT * FROM mipt_asj.calc_dict(
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'tdata'), 'f',
	(SELECT oid FROM pg_catalog.pg_class WHERE relname = 'tdata'), 'a'
);
