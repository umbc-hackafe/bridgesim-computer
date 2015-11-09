INCLUDES += ../motherboard/include

CFLAGS += $(patsubst %, -I%, $(INCLUDES))
CFLAGS += $(shell python3-config --cflags)

LDFLAGS += $(shell python3-config --ldflags)

all: libbridgesimrdma.a

libbridgesimrdma.a: rdma.o
	$(AR) rsc $@ $^

rdma.o: rdma.c rdma.h
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

rdma.c rdma.h: rdma.pyx
	./setup.py build_ext --inplace


.PHONY: clean
clean:
	-rm rdma.h rdma.c *.so *.a *.o
	-./setup.py clean
