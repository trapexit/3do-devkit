CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -pedantic -std=c99 -O2
TARGET  = aifinfo

.PHONY: all clean

all: $(TARGET)

$(TARGET): aifinfo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
