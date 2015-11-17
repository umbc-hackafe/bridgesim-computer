#include <cstdint>
#include <iostream>
#include <mutex>
#include <new>
#include <queue>

extern "C" {
#include "motherboard.h"
}

#include "stacker.h"

using namespace std;

struct StackCPUDevice {
    size_t stack_size;
    // Internal stack pointer
    size_t isp;
    uint32_t* stack;
    // Instruction pointer
    uint64_t ip;
    // Stack pointer
    uint64_t sp;

    queue<uint32_t> interrupts;
    mutex interrupt_lock;

    void* motherboard;
    MotherboardFunctions mbfuncs;

    int32_t init();
    int32_t cleanup();
    int32_t reset();
    int32_t boot();
    int32_t interrupt(uint32_t code);
    int32_t register_motherboard(void* motherboard, MotherboardFunctions* mbfuncs);

    int32_t process_code(uint32_t code);
    int32_t process_instruction();
};

static uint32_t next_device_id = 0;

extern "C" {
    static int32_t init(void*);
    static int32_t cleanup(void*);
    static int32_t reset(void*);
    static int32_t boot(void*);
    static int32_t interrupt(void*, uint32_t);
    static int32_t register_motherboard(void*, void*, MotherboardFunctions*);

    struct Device* bscomp_device_new(const struct StackCPUConfig* config) {
        if (!config || !config->stack_size) {
            return 0;
        }

        Device* dev = 0;
        StackCPUDevice* cpudev = 0;

        try {
            dev = new Device();
            cpudev = new StackCPUDevice();
        } catch (const bad_alloc& ex) {
            if (dev) delete dev;
            if (cpudev) delete cpudev;
            return 0;
        }

        cpudev->stack_size = config->stack_size;

        dev->device = cpudev;
        dev->init = &init;
        dev->cleanup = &cleanup;
        dev->reset = &reset;
        dev->boot = &boot;
        dev->interrupt = &interrupt;
        dev->register_motherboard = &register_motherboard;

        dev->device_type = stack_cpu_device_type_id;
        dev->device_id = next_device_id++;

        cout << "completed create" << endl;
        return dev;
    }

    void bscomp_device_destroy(struct Device* dev) {
        if (!dev) {
            return;
        }

        StackCPUDevice* cpudev = static_cast<StackCPUDevice*>(dev->device);
        if (!cpudev || !cpudev->stack) {
            // Already messed up, don't mess up further by trying to do a partial free.
            return;
        }

        delete[] cpudev->stack;
        cpudev->stack = 0;

        delete cpudev;
        dev->device = 0;

        delete dev;
    }

    static int32_t init(void* cpudev) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->init();
    }

    static int32_t cleanup(void* cpudev) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->cleanup();
    }

    static int32_t reset(void* cpudev) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->reset();
    }

    static int32_t boot(void* cpudev) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->boot();
    }

    static int32_t interrupt(void* cpudev, uint32_t code) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->interrupt(code);
    }

    static int32_t register_motherboard(void* cpudev, void* motherboard, MotherboardFunctions* mbfuncs) {
        if (!cpudev) {
            return -1;
        }
        StackCPUDevice* cd = static_cast<StackCPUDevice*>(cpudev);
        return cd->register_motherboard(motherboard, mbfuncs);
    }
} // end of extern "C"

int32_t StackCPUDevice::init() {
    try {
        stack = new uint32_t[stack_size];
    } catch(const bad_alloc& ex) {
        return -1;
    }
    return 0;
}

int32_t StackCPUDevice::cleanup() {
    if (stack) {
        delete[] stack;
        stack = 0;
    }
    return 0;
}

int32_t StackCPUDevice::reset() {
    for (size_t i = 0; i < stack_size; ++i) {
        stack[i] = 0;
    }

    interrupt_lock.lock();
    queue<uint32_t>().swap(interrupts);
    interrupt_lock.unlock();

    return 0;
}

int32_t StackCPUDevice::boot() {
    cout << "Stack CPU Received BOOT" << endl;
    while (true) {
        uint32_t code = 0;
        bool has_code = false;
        interrupt_lock.lock();
        if (!interrupts.empty()) {
            code = interrupts.front();
            interrupts.pop();
            has_code = true;
        }
        interrupt_lock.unlock();

        if (has_code) {
            if (code == 0) {
                break;
            } else  {
                auto res = process_code(code);
                if (res) {
                    cout << "Simulator error -- Stack CPU Halting." << endl;
                    return res;
                }
            }
        } else {
            auto res = process_instruction();
            if (res) {
                cout << "Simulator error -- Stack CPU Halting." << endl;
                return res;
            }
        }
    }
    cout << "Stack CPU Shutting Down" << endl;
    return 0;
}

int32_t StackCPUDevice::interrupt(uint32_t code) {
    cout << "Stack CPU Received INTERRUPT " << code << endl;
    interrupt_lock.lock();
    interrupts.push(code);
    interrupt_lock.unlock();
    return 0;
}

int32_t StackCPUDevice::register_motherboard(void* motherboard, MotherboardFunctions* mbfuncs) {
    this->motherboard = motherboard;
    this->mbfuncs = *mbfuncs;
    return 0;
}

int32_t StackCPUDevice::process_code(uint32_t code) {
    cout << "CPU: Ignoring Interrupt " << code << endl;
    return 0;
}

int32_t StackCPUDevice::process_instruction() {
    uint8_t instr;
    auto res = mbfuncs.read_bytes(motherboard, ip, 1, &instr);
    if (res) {
        return res;
    }

    return 0;
}
