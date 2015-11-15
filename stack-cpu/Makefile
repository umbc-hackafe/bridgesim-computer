INCLUDES += ../motherboard/include

CFLAGS += --std=c99 -Wall
CFLAGS += $(patsubst %, -I%, $(INCLUDES))

all: libbridgesimstackcpu.so

stacker.o: stacker.c stacker.h ../motherboard/include/motherboard.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

libbridgesimstackcpu.so: stacker.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

.PHONY: clean
clean:
	-rm stacker.o libbridgesimstackcpu.so
