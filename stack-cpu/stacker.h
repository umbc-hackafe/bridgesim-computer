#ifndef bscomp_stacker_h
#define bscomp_stacker_h

#ifdef __cplusplus
#include <cstdint>

using namespace std;

extern "C" {
#else
#include <stdint.h>
#endif

#include "motherboard.h"

static const uint64_t stack_cpu_device_type_id = 2l;

struct StackCPUConfig {
    uint32_t stack_size;
};

struct Device* bscomp_device_new(const struct StackCPUConfig* config);
void bscomp_device_destroy(struct Device* dev);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // bscomp_stacker_h
