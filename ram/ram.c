#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "motherboard.h"
#include "ram.h"

struct RamDevice {
    uint32_t memory_size;
    uint8_t* memory;
};

static uint32_t next_device_id = 0;

static int32_t load_bytes(void*, uint32_t, uint32_t, uint8_t*);
static int32_t write_bytes(void*, uint32_t, uint32_t, uint8_t*);
static int32_t reset(void*);

struct Device* bscomp_device_new(const struct RAMConfig* config) {
    if (!config || !config->memory_size) {
        return 0;
    }

    struct Device* dev = malloc(sizeof(struct Device));
    struct RamDevice* ramdev = malloc(sizeof(struct RamDevice));
    uint8_t* mem = malloc(config->memory_size);

    if (!dev || !ramdev || !mem) {
        free(dev);
        free(ramdev);
        free(mem);
        return 0;
    }

    *dev = (const struct Device){0};

    ramdev->memory_size = config->memory_size;
    ramdev->memory = mem;

    dev->device = ramdev;
    dev->export_memory_size = config->memory_size;

    dev->load_bytes = &load_bytes;
    dev->write_bytes = &write_bytes;
    dev->reset = &reset;

    dev->device_type = ram_device_type_id;
    dev->device_id = next_device_id++;

    return dev;
}

void bscomp_device_destroy(struct Device* dev) {
    if (!dev) {
        return;
    }

    struct RamDevice* ramdev = dev->device;
    if (!ramdev || !ramdev->memory) {
        // Things are borked already, we're not even going to try to fix it by cleaning
        // stuff up. That might just make things worse if we have the wrong kind of
        // pointer.
        return;
    }

    // Clear pointers after free, even thoug we know we're freeing the object that
    // contains them too.
    free(ramdev->memory);
    ramdev->memory = 0;

    free(ramdev);
    dev->device = 0;

    free(dev);
}

static int32_t load_bytes(void* ramdev, uint32_t src, uint32_t len, uint8_t* dest) {
    if (!ramdev) {
        return -1;
    }

    struct RamDevice* rd = ramdev;

    for (uint32_t i = 0; i < len && src + i < rd->memory_size; i++) {
        dest[i] = rd->memory[i];
    }

    return 0;
}

static int32_t write_bytes(void* ramdev, uint32_t dest, uint32_t len, uint8_t* src) {
    if (!ramdev) {
        return -1;
    }

    struct RamDevice* rd = ramdev;

    for (uint32_t i = 0; i < len && dest + i < rd->memory_size; i++) {
        rd->memory[i] = src[i];
    }

    return 0;
}

static int32_t reset(void* ramdev) {
    if (!ramdev) {
        return -1;
    }

    struct RamDevice* rd = ramdev;
    memset(rd->memory, 0, rd->memory_size);

    return 0;
}
