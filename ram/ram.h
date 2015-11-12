#ifndef bscomp_ram_h
#define bscomp_ram_h

#include <stdint.h>

#include "motherboard.h"

static const uint64_t ram_device_type_id = (1l << 32) | 1l;

struct RAMConfig {
    uint32_t memory_size;
};

struct Device* bscomp_device_new(struct RAMConfig* config);
void bscomp_device_destroy(struct Device* dev);

#endif // bscomp_ram_h
