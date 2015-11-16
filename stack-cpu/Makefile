INCLUDES += ../motherboard/include

CXXFLAGS += --std=c++14 -Wall
CXXFLAGS += $(patsubst %, -I%, $(INCLUDES))

all: libbridgesimstackcpu.so

stacker.o: stacker.cpp stacker.h ../motherboard/include/motherboard.h
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

libbridgesimstackcpu.so: stacker.o
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

.PHONY: clean
clean:
	-rm stacker.o libbridgesimstackcpu.so
