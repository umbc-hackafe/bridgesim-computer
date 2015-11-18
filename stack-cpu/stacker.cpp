#include <cstdint>
#include <iostream>
#include <mutex>
#include <new>
#include <queue>
#include <type_traits>

extern "C" {
#include "motherboard.h"
}

#include "stacker.h"

using namespace std;

struct StackCPUDevice {
    uint32_t stack_size;
    // Internal stack pointer
    uint32_t isp;
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

    template<typename T>
    int32_t pop(T& dest);

    template<typename T>
    int32_t push(T source);

    template<typename T>
    int32_t add();
    template<typename T>
    int32_t subtract();
    template<typename T>
    int32_t multiply();
    template<typename T>
    int32_t divide();
    template<typename T>
    int32_t and_();
    template<typename T>
    int32_t or_();
    template<typename T>
    int32_t xor_();
    template<typename T>
    int32_t not_();
    template<typename T>
    int32_t negate();

    template<typename T>
    int32_t ge();
    template<typename T>
    int32_t gt();
    template<typename T>
    int32_t eq();
    template<typename T>
    int32_t neq();
    template<typename T>
    int32_t lt();
    template<typename T>
    int32_t le();

    template<typename T>
    int32_t copy();
    template<typename T>
    int32_t discard();

    template<typename T>
    int32_t read();
    template<typename T>
    int32_t read_immediate();
    template<typename T>
    int32_t write();
    template<typename T>
    int32_t shift();
    template<typename T>
    int32_t unshift();
    int32_t read_sp();
    int32_t write_sp();
    template<typename T, typename U>
    int32_t resize();
    template<typename T>
    int32_t swap();
    int32_t jump();
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
    for (uint32_t i = 0; i < stack_size; ++i) {
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
            try {
                auto res = process_instruction();
                if (res) {
                    cout << "Bad instruction; error: " << res << endl;
                }
            } catch (const int32_t& simerror) {
                cout << "Simultator error code " << simerror << " -- CPU Shutting Down."
                     << endl;
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

#define SIZE_SWITCH(OP, size) switch (size) {   \
    case 2:                                     \
        return OP<float>();                     \
        break;                                  \
    case 3:                                     \
        return OP<uint8_t>();                   \
        break;                                  \
    case 4:                                     \
        return OP<uint16_t>();                  \
        break;                                  \
    case 5:                                     \
        return OP<uint32_t>();                  \
        break;                                  \
    case 6:                                     \
        return OP<uint64_t>();                  \
        break;                                  \
    case 7:                                     \
        return OP<double>();                    \
        break;                                  \
    default:                                    \
        return 4;                               \
    }

#define SIZE_SWITCH_NOFLOAT(OP, size) switch (size) {   \
    case 3:                                             \
        return OP<uint8_t>();                           \
        break;                                          \
    case 4:                                             \
        return OP<uint16_t>();                          \
        break;                                          \
    case 2:                                             \
    case 5:                                             \
        return OP<uint32_t>();                          \
        break;                                          \
    case 6:                                             \
    case 7:                                             \
        return OP<uint64_t>();                          \
        break;                                          \
    default:                                            \
        return 4;                                       \
    }

#define RESIZE_SWITCH_INNER(fromsize, from)                 \
    case fromsize | (2 << 3):                               \
        return resize<from, float>();                       \
        break;                                              \
    case fromsize | (3 << 3):                               \
        return resize<from, uint8_t>();                     \
        break;                                              \
    case fromsize | (4 << 3):                               \
        return resize<from, uint16_t>();                    \
        break;                                              \
    case fromsize | (5 << 3):                               \
        return resize<from, uint32_t>();                    \
        break;                                              \
    case fromsize | (6 << 3):                               \
        return resize<from, uint64_t>();                    \
        break;                                              \
    case fromsize | (7 << 3):                               \
        return resize<from, double>();                      \
        break;                                              \


int32_t StackCPUDevice::process_instruction() {
    uint8_t instruction[2];
    auto res = mbfuncs.read_bytes(motherboard, ip, 2, instruction);
    if (res) {
        throw res;
    }
    ip += 2;

    uint8_t& instr = instruction[0];
    uint8_t& size = instruction[1];

    switch (instr) {
    case '+': // add
        SIZE_SWITCH(add, size)
        break;
    case '-': // subtract
        SIZE_SWITCH(subtract, size)
        break;
    case '*': // multiply
        SIZE_SWITCH(multiply, size)
        break;
    case '/': // divide
        SIZE_SWITCH(divide, size)
        break;
    case '&': // and
        SIZE_SWITCH_NOFLOAT(and_, size)
        break;
    case '|': // or
        SIZE_SWITCH_NOFLOAT(or_, size)
        break;
    case '^': // xor
        SIZE_SWITCH_NOFLOAT(xor_, size)
        break;
    case '~': // not
        SIZE_SWITCH_NOFLOAT(not_, size)
        break;
    case '_': // negate
        SIZE_SWITCH(negate, size)
        break;
    case '<': // less than
        SIZE_SWITCH(lt, size)
        break;
    case '>': // greater than
        SIZE_SWITCH(gt, size)
        break;
    case 'g': // greater than or equal to
        SIZE_SWITCH(ge, size)
        break;
    case 'l': // less than or equal to
        SIZE_SWITCH(le, size)
        break;
    case '=': // equal
        SIZE_SWITCH(eq, size)
        break;
    case '!': // not equal
        SIZE_SWITCH(neq, size)
        break;
    case 'C': // Copy
        SIZE_SWITCH(copy, size)
        break;
    case 'D': // Discard
        SIZE_SWITCH(discard, size)
        break;
    case 'R': // Read
        SIZE_SWITCH(read, size)
        break;
    case 'r': // Read Immediate
        SIZE_SWITCH(read_immediate, size)
        break;
    case 'W': // Write
        SIZE_SWITCH(write, size)
        break;
    case 'S': // Shift
        SIZE_SWITCH(shift, size)
        break;
    case 'U': // Unshift
        SIZE_SWITCH(unshift, size)
    case 'P': // Read Stack Pointer
        return read_sp();
        break;
    case 'p': // Write Stack Pointer
        return write_sp();
        break;
    case 'z': // Resize
        // OLDSIZE = size & 0b111, NEWSIZE = (size & 0b111000) >> 3
        switch (size) {
            RESIZE_SWITCH_INNER(2, float)
            RESIZE_SWITCH_INNER(3, uint8_t)
            RESIZE_SWITCH_INNER(4, uint16_t)
            RESIZE_SWITCH_INNER(5, uint32_t)
            RESIZE_SWITCH_INNER(6, uint64_t)
            RESIZE_SWITCH_INNER(7, double)
        default:
            return 4;
        }
        break;
    case 's': // Swap
        SIZE_SWITCH(swap, size)
        break;
    case 'j': // Jump
        return jump();
        break;
    }

    return 16;
}

template<typename T>
int32_t StackCPUDevice::pop(T& dest) {
    static_assert(sizeof(T) <= sizeof(uint32_t), "Dest must be no more than 4 bytes.");
    if (sp < 1) return 1;
    dest = *((T*)(&stack[--sp]));
    return 0;
}

template<>
int32_t StackCPUDevice::pop<uint64_t>(uint64_t& dest) {
    if (sp < 2) return 1;
    uint32_t* blocks = (uint32_t*)(&dest);
    blocks[1] = stack[--sp];
    blocks[0] = stack[--sp];
    return 0;
}

template<>
int32_t StackCPUDevice::pop<double>(double& dest) {
    if (sp < 2) return 1;
    uint32_t* blocks = (uint32_t*)(&dest);
    blocks[1] = stack[--sp];
    blocks[0] = stack[--sp];
    return 0;
}

template<typename T>
int32_t StackCPUDevice::push(T source) {
    static_assert(sizeof(T) <= sizeof(uint32_t), "Source must be no more than 4 bytes.");
    if (sp >= stack_size) return 2;
    *((T*)(&stack[sp++])) = source;
    return 0;
}

template<>
int32_t StackCPUDevice::push<uint64_t>(uint64_t source) {
    if (sp >= stack_size - 1) return 2;
    uint32_t* blocks = (uint32_t*)(&source);
    stack[sp++] = blocks[0];
    stack[sp++] = blocks[1];
    return 0;
}

template<>
int32_t StackCPUDevice::push<double>(double source) {
    if (sp >= stack_size - 1) return 2;
    uint32_t* blocks = (uint32_t*)(&source);
    stack[sp++] = blocks[0];
    stack[sp++] = blocks[1];
    return 0;
}

#define BINARY_OPERATOR(opname, OP) template<typename T>   \
    int32_t StackCPUDevice::opname() {                     \
        T a, b;                                            \
        auto res1 = pop<T>(a);                             \
        auto res2 = pop<T>(b);                             \
        auto res3 = push<T>(a OP b);                       \
        return res1 | res2 | res3;                         \
    }

BINARY_OPERATOR(add, +)
BINARY_OPERATOR(subtract, -)
BINARY_OPERATOR(multiply, *)
BINARY_OPERATOR(divide, /)
BINARY_OPERATOR(and_, &)
BINARY_OPERATOR(or_, |)
BINARY_OPERATOR(xor_, ^)

#define BINARY_COMPARISON(opname, OP) template<typename T> \
    int32_t StackCPUDevice::opname() {                     \
        T a, b;                                            \
        auto res1 = pop<T>(a);                             \
        auto res2 = pop<T>(b);                             \
        auto res3 = push<int32_t>(a OP b);                 \
        return res1 | res2 | res3;                         \
    }

BINARY_COMPARISON(le, <=)
BINARY_COMPARISON(lt, <)
BINARY_COMPARISON(eq, ==)
BINARY_COMPARISON(neq, !=)
BINARY_COMPARISON(gt, >)
BINARY_COMPARISON(ge, >=)

template<typename T>
int32_t StackCPUDevice::not_() {
    T a;
    auto res1 = pop<T>(a);
    auto res2 = push<T>(~a);
    return res1 | res2;
}

template<typename T>
int32_t StackCPUDevice::negate() {
    T a;
    auto res1 = pop<T>(a);
    auto res2 = push<T>(-a);
    return res1 | res2;
}

#define B

template<typename T>
int32_t StackCPUDevice::copy() {
    T a;
    auto res1 = pop<T>(a);
    auto res2 = push<T>(a);
    auto res3 = push<T>(a);
    return res1 | res2 | res3;
}

template<typename T>
int32_t StackCPUDevice::discard() {
    T a;
    return pop<T>(a);
}

template<typename T>
int32_t StackCPUDevice::read() {
    uint64_t addr;
    auto res1 = pop<uint64_t>(addr);
    T val;
    auto res2 = mbfuncs.read_bytes(motherboard, addr, sizeof(val), (uint8_t*)(&val));
    if (res2) {
        throw res2;
    }
    auto res3 = push<T>(val);
    return res1 | res3;
}

template<typename T>
int32_t StackCPUDevice::read_immediate() {
    T val;
    auto res1 = mbfuncs.read_bytes(motherboard, ip, sizeof(val), (uint8_t*)(&val));
    ip += sizeof(val);
    if (res1) {
        throw res1;
    }
    auto res2 = push<T>(val);
    return res2;
}

template<typename T>
int32_t StackCPUDevice::write() {
    uint64_t addr;
    auto res1 = pop<uint64_t>(addr);
    T val;
    auto res2 = pop<T>(val);
    auto res3 = mbfuncs.write_bytes(motherboard, addr, sizeof(val), (uint8_t*)(&val));
    if (res3) {
        throw res3;
    }
    return res1 | res2;
}

template<typename T>
int32_t StackCPUDevice::shift() {
    T val;
    auto res1 = pop<T>(val);
    sp -= sizeof(val);
    auto res2 = mbfuncs.write_bytes(motherboard, sp, sizeof(val), (uint8_t*)(&val));
    if (res2) {
        throw res2;
    }
    return res1;
}

template<typename T>
int32_t StackCPUDevice::unshift() {
    T val;
    auto res1 = mbfuncs.read_bytes(motherboard, sp, sizeof(val), (uint8_t*)(&val));
    if (res1) {
        throw res1;
    }
    auto res2 = push<T>(val);
    return res2;
}

int32_t StackCPUDevice::read_sp() {
    return push<uint64_t>(sp);
}

int32_t StackCPUDevice::write_sp() {
    return pop<uint64_t>(sp);
}

template<typename T, typename U>
int32_t StackCPUDevice::resize() {
    if (sizeof(T) == sizeof(U)) return 0;
    T original;
    U replacement;
    auto res1 = pop<T>(original);
    replacement = static_cast<U>(original);
    auto res2 = push<U>(replacement);
    return res1 | res2;
}

template<typename T>
int32_t StackCPUDevice::swap() {
    T a, b;
    auto res1 = pop<T>(a);
    auto res2 = pop<T>(b);
    auto res3 = push<T>(a);
    auto res4 = push<T>(b);
    return res1 | res2 | res3 | res4;
}

int32_t StackCPUDevice::jump() {
    uint64_t addr;
    int32_t condition;
    auto res1 = pop(addr);
    auto res2 = pop(condition);
    if (condition) {
        ip = addr;
    }
    return res1 | res2;
}
