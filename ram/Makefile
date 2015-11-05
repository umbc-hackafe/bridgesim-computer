INCLUDES ::= ../include

CFLAGS ::= --std=c99 -Wall
CFLAGS += $(patsubst %,-I%, $(INCLUDES))

all: libbridgesimram.so

ram.o: ram.c ../include/ram/ram.h ../include/motherboard/motherboard.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libbridgesimram.so: ram.o
	$(CC) $(CFLAGS) -shared -Wl,-soname,$@ -o $@ $^

.PHONY: clean
clean:
	-rm ram.o libbridgesimram.so
