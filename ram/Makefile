INCLUDES += ../motherboard/include

CFLAGS += --std=c99 -Wall
CFLAGS += $(patsubst %, -I%, $(INCLUDES))

all: libbridgesimram.so

ram.o: ram.c ram.h ../motherboard/include/motherboard.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libbridgesimram.so: ram.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

.PHONY: clean
clean:
	-rm ram.o libbridgesimram.so
