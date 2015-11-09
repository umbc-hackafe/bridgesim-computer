#ifndef bscomp_ram_h
#define bscomp_ram_h

#include <stdint.h>

#include "motherboard/motherboard.h"

static const uint64_t ram_device_id = (1l << 32) | 1l;

struct Device* bscomp_make_ram_device(uint32_t memory_size);
void bscomp_delete_ram_device(struct Device* dev);

#endif // bscomp_ram_h
