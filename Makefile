CFLAGS ?= -O2
LDFLAGS ?=

TARGET = lo_inp

SOURCES = lo_inp.c

TMPFILE = $(shell mktemp)
USE_MRAA = $(shell $(CC) $(CFLAGS) mraa_tst.c -o $(TMPFILE) -lmraa $(LDFLAGS) > /dev/null 2>&1 && rm $(TMPFILE) && echo "yes")

ifeq ($(USE_MRAA),yes)
	LDFLAGS += -lmraa
	CFLAGS += -DHAVE_MRAA=1
endif

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)
