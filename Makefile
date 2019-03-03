EXTENSION = mipt-asj

MODULES = mipt-asj
MODULE_big = mipt-asj
DATA = mipt-asj--0.1.sql
OBJS = mipt-asj.o lib/trie.o asj/calc_dict.o

PG_CFLAGS = -std=c99

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
