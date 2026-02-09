CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -pedantic -std=c99 -O2
TARGET  = aofdump

.PHONY: all clean

all: $(TARGET)

$(TARGET): aofdump.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
