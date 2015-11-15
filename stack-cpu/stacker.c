#include <stdlib.h>
#include <stdint.h>

#include "motherboard.h"
#include "stacker.h"

struct StackCPUDevice {
    size_t stack_size;
    uint8_t* stack;
    uint64_t ip;
    uint64_t sp;
};

static uint32_t next_device_id = 0;

struct Device* bscomp_device_new(const struct StackCPUConfig* config) {
    if (!config || !config->stack_size) {
        return 0;
    }

    struct Device* dev = malloc(sizeof(struct Device));
    struct StackCPUDevice* cpudev = malloc(sizeof(struct StackCPUDevice));
    uint8_t* stack = malloc(config->stack_size);

    if (!dev || !cpudev || !stack) {
        free(dev);
        free(cpudev);
        free(stack);
        return 0;
    }

    *dev = (const struct Device){0};

    cpudev->stack_size = config->stack_size;
    cpudev->stack = stack;
    cpudev->ip = 0;
    cpudev->sp = 0;

    dev->device = cpudev;

    dev->device_type = stack_cpu_device_type_id;
    dev->device_id = next_device_id++;

    return dev;
}

void bscomp_device_destroy(struct Device* dev) {
    if (!dev) {
        return;
    }

    struct StackCPUDevice* cpudev = dev->device;
    if (!cpudev || !cpudev->stack) {
        // Already messed up, don't mess up further by trying to do a partial free.
        return;
    }

    free(cpudev->stack);
    cpudev->stack = 0;

    free(cpudev);
    dev->device = 0;

    free(dev);
}
