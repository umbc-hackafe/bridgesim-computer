INCLUDES += ../motherboard/include ../ram ../rdma

CFLAGS += --std=c99 -Wall
CFLAGS += $(patsubst %, -I%, $(INCLUDES))
CFLAGS += $(shell python3-config --cflags)

LDFLAGS += $(shell python3-config --ldflags)

RUST_STATIC_REQUIRED ::= -lc -lm -ldl -lpthread -lrt -lgcc_s -lpthread -lc -lm
LDFLAGS += $(RUST_STATIC_REQUIRED)

all: example-runner

example-runner: run.o ../motherboard/target/debug/libmotherboard.a ../ram/libbridgesimram.a \
	../rdma/libbridgesimrdma.a
	$(CC) $(LDFLAGS) -o $@ $^

run.o: run.c ../ram/ram.h ../motherboard/include/motherboard.h ../rdma/rdma.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean run

clean:
	-rm run.o example-runner
