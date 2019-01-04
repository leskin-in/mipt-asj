EXTENSION = mipt-asj
DATA = mipt-asj--0.1.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
