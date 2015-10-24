#ifndef bscomp_motherboard_h
#define bscomp_motherboard_h

#include <stddef.h>

// Create a motherboard with a pluggable device capacity of max_devices
//
// To dealocate a motherboard created with this function, always pass the resulting
// pointer to bscomp_motherboard_destroy.
void* bscomp_motherboard_new(size_t max_devices);

// Free a motherboard created by bscomp_motherboard_new.
//
// It is undefined behavior to call this function with an pointer not allocated by
// bscomp_motherboard_new.
void bscomp_motherboard_destroy(void* motherboard);

#endif // bscomp_motherboard_h
