CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -pedantic -std=c99 -O2
TARGET  = alfdump

.PHONY: all clean

all: $(TARGET)

$(TARGET): alfdump.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
