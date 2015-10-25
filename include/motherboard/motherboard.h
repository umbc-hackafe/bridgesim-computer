#ifndef bscomp_motherboard_h
#define bscomp_motherboard_h
// This file to be kept in sync with motherboard/src/lib.rs

#include <stdint.h>

struct MotherboardFunctions;

// Represents how memory is mapped for a Device
//
// Different devices present different types of memory; devices intended just to be used
// as RAM will be mapped to memory differently than devices which provide I/O.
enum MappedMemoryType {
    // No memory will be mapped for the device.
    MMT_None = 0,
    // Memory will be mapped for the device, and it will be mapped in the RAM section of
    // system memory.
    MMT_Ram = 1,
    // Memory will be mapped for the device, and it will be mapped in the I/O Devices
    // section of system memory.
    MMT_IODevice = 2,
};

// Represents a device handled by a motherboard
//
// All of the device functions supplied should return an error code if they fail. However
// this should only be used for signaling the motherboard of invalid *simulator*
// behavior. Invalid behavior of a *simulated* part should be handled through in-sim
// methods like interrupts.
//
// # Safety
//
// The device operation functions provided in a `Device` must be made thread-safe by the
// implementor. The motherboard and other devices may call them freely in a
// multi-threaded environment.
//
// The `load_*` functions take a pointer to the memory to place the loaded value in. The
// actual implementation must not hold on to this pointer past the lifetime of the
// function.
struct Device {
    // Pointer to the actual underlying device being operated on.
    void* device;

    // Indicated whether memory mapping should be done for this device, and what type of
    // mapping should be used.
    enum MappedMemoryType export_memory;
    // Indicates the size of exported memory
    uint32_t export_memory_size;
    // Device, Address, Destination/Source
    // Function to load a byte from the device's memory.
    int32_t (*load_byte)(void*, uint32_t, uint8_t*);
    // Function to load a word from the device's memory.
    int32_t (*load_byte)(void*, uint32_t, uint16_t*);
    // Function to load a dword from the device's memory.
    int32_t (*load_byte)(void*, uint32_t, uint32_t*);
    // Function to load a qword from the device's memory.
    int32_t (*load_byte)(void*, uint32_t, uint64_t*);
    // Function to save a byte to the device's memory.
    int32_t (*write_byte)(void*, uint32_t, uint8_t*);
    // Function to save a word to the device's memory.
    int32_t (*write_byte)(void*, uint32_t, uint16_t*);
    // Function to save a dword to the device's memory.
    int32_t (*write_byte)(void*, uint32_t, uint32_t*);
    // Function to save a qword to the device's memory.
    int32_t (*write_byte)(void*, uint32_t, uint64_t*);

    // Device
    // Function to use to initialize the device before booting.
    int32_t (*init)(void*);

    // Device
    // Optional function to use to boot the device.
    int32_t (*boot)(void*);

    // Device, Interrupt Code
    // Optional function to use to send an interrupt code to the device.
    int32_t (*interrupt)(void*, uint32_t);

    // Device, Motherboard, Motherboard Functions
    // Function to use to register the motherboard's functions with this device.
    int32_t (*register_motherboard)(void*, void*, struct MotherboardFunctions*);
};

struct MotherboardFunctions {
    // Motherboard, Address, Destination.
    // Callback for a device to read a byte of memory.
    int32_t (*read_byte)(void*, uint64_t, uint8_t*);
    // Callback for a device to read a word of memory.
    int32_t (*read_byte)(void*, uint64_t, uint16_t*);
    // Callback for a device to read a dword of memory.
    int32_t (*read_byte)(void*, uint64_t, uint32_t*);
    // Callback for a device to read a qword of memory.
    int32_t (*read_byte)(void*, uint64_t, uint64_t*);

    // Motherboard, Address, Source.
    // Callback for a device to write a byte of memory.
    int32_t (*write_byte)(void*, uint64_t, uint8_t);
    // Callback for a device to write a word of memory.
    int32_t (*write_byte)(void*, uint64_t, uint16_t);
    // Callback for a device to write a dword of memory.
    int32_t (*write_byte)(void*, uint64_t, uint32_t);
    // Callback for a device to write a qword of memory.
    int32_t (*write_byte)(void*, uint64_t, uint64_t);

    // Motherboard, Target Device, Interrupt Code.
    // Callback for a device to send an interrupt to another device.
    int32_t (*send_interrupt)(void*, uint32_t, uint32_t);
};

// Create a motherboard with a pluggable device capacity of max_devices
//
// To dealocate a motherboard created with this function, always pass the resulting
// pointer to bscomp_motherboard_destroy.
void* bscomp_motherboard_new(uint32_t max_devices);

// Free a motherboard created by bscomp_motherboard_new.
//
// It is undefined behavior to call this function with an pointer not allocated by
// bscomp_motherboard_new.
void bscomp_motherboard_destroy(void* motherboard);

// Get the number of available slots
uint32_t bscomp_motherboard_num_slots(void* motherboard);
// Get the number of slotss that are filled
uint32_t bscomp_motherboard_slots_filled(void* motherboard);
// Get the whether the board is full
int32_t bscomp_motherboard_is_full(void* motherboard);

// Add a device to the given mother board.
//
// Returns an error code indicating if the addition is successful. It fails if either of
// the pointers is null or the motherboard is already full.
//
// # Safety
//
// The motherboard must be a motherboard allocated with `bscomp_motherboard_new`.
//
// The device passed in will be copied, so the caller does not need to keep a pointer to
// the device. However, it is the responsibility of the caller to enure that any function
// pointers included in the device live longer than the motherboard.
int32_t bscomp_motherboard_add_device(void* motherboard, struct Device* device);

#endif // bscomp_motherboard_h
