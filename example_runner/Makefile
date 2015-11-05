INCLUDES ::= ../include

CFLAGS ::= --std=c99 -Wall
CFLAGS += $(patsubst %, -I%, $(INCLUDES))

all: example-runner

example-runner: run.o
	$(CC) $(CFLAGS) -L../motherboard/target/debug -lmotherboard -L../ram -lbridgesimram -o $@ $^

run.o: run.c ../include/ram/ram.h ../include/motherboard/motherboard.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean run

run: example-runner
	-LD_LIBRARY_PATH=../motherboard/target/debug:../ram ./example-runner

clean:
	-rm run.o example-runner
