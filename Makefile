EXTENSION = redis
EXTVERSION = $(shell grep default_version $(EXTENSION).control | sed -e "s/default_version[[:space:]]*=[[:space:]]*'\([^']*\)'/\1/")

MODULE_big = $(EXTENSION)
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
SHLIB_LINK += -lhiredis

all: $(EXTENSION)--$(EXTVERSION).sql
$(EXTENSION)--$(EXTVERSION).sql: $(EXTENSION).sql
	cp $< $@

DATA_built = $(EXTENSION)--$(EXTVERSION).sql
DATA = $(filter-out $(EXTENSION)--$(EXTVERSION).sql, $(wildcard *--*.sql))
EXTRA_CLEAN = $(EXTENSION)--$(EXTVERSION).sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)