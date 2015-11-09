INCLUDES ::= ../include

CFLAGS ::= --std=c99 -Wall
CFLAGS += $(patsubst %, -I%, $(INCLUDES))

RUST_STATIC_REQUIRED ::= -lc -lm -ldl -lpthread -lrt -lgcc_s -lpthread -lc -lm

all: example-runner

example-runner: run.o ../motherboard/target/debug/libmotherboard.a ../ram/libbridgesimram.a
	$(CC) $(CFLAGS) -o $@ $^ ../motherboard/target/debug/libmotherboard.a \
	../ram/libbridgesimram.a $(RUST_STATIC_REQUIRED)

run.o: run.c ../include/ram/ram.h ../include/motherboard/motherboard.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean run

clean:
	-rm run.o example-runner
