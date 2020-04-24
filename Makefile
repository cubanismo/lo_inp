CC = gcc
CFLAGS ?= -O2

TARGET = lo_inp

SOURCES = lo_inp.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET)
