CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -pedantic -std=c99 -O2
TARGET  = armlib

.PHONY: all clean

all: $(TARGET)

$(TARGET): armlib.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
