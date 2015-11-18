#ifndef bscomp_motherboard_h
#define bscomp_motherboard_h
// This file to be kept in sync with motherboard/src/lib.rs

#include <stdint.h>

struct MotherboardFunctions;

struct MotherboardConfig {
    uint32_t max_devices;
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

    uint64_t device_type;
    uint32_t device_id;

    // Indicates the size of exported memory
    uint32_t export_memory_size;

    // Read and Write need not be thread safe, as multiple devices accessing the same
    // memory can be highly unsafe. It is expected that devices define protocols for safe
    // communication using a combination of shared memory and interrupts.

    // Function to load a byte from the device's memory.
    //
    // Should take four arguments:
    // - The device pointer
    // - The (local) address to read from
    // - The number of bytes to read
    // - A pointer to the memory to store the read bytes in (assumed to be large enough).
    //
    // Reads should simply ignore invalid memory addresses, and fill as much as they can.
    int32_t (*load_bytes)(void*, uint32_t, uint32_t, uint8_t*);
    // Function to save a byte to the device's memory.
    //
    // Should take four arguments:
    // - The device pointer
    // - The (local) address to write to
    // - The number of bytes to write
    // - A pointer to the memory to read the written bytes from.
    //
    // Writes should simply ignore invalid memory addresses, and fill as much as they can.
    int32_t (*write_bytes)(void*, uint32_t, uint32_t, uint8_t*);

    // Device
    // Function to use to initialize the device before booting.
    //
    // Device init order is undefined so don't rely on other devices here.  Called after
    // register motherboard.
    int32_t (*init)(void*);

    // Device
    // Function to reset the device before booting and during rebooting.
    //
    // Don't rely on other device here, as reset order is undefined. Called after init.
    int32_t (*reset)(void*);

    // Device
    // Function to cleanup resources from init.
    //
    // This function is called after all devices have left the boot state. Cleanup order
    // of devices is undefined, so don't rely on device memory here.
    int32_t (*cleanup)(void*);

    // Device
    // Optional function to use to run the device. Called once per boot, after init
    // and reset. Accessing other devices is Ok here.
    //
    // It should loop until the device shuts down. If this method exists, then halt is
    // expeted to be provided. If a device provides boot but not interrupt, it is an error
    // on motheboard boot. If a device provides boot and does not quit when halt is
    // called, the motherboard will hang waiting for it to shutdown.
    //
    // This should not handle device initialization, as the initialization order of these
    // devices with respect to any others is not defined.
    int32_t (*boot)(void*);

    // Device
    // Optional function to use to stop boot from running. Required if boot is provided.
    int32_t (*halt)(void*);

    // Device, Interrupt Code
    // Optional function to use to send an interrupt code to the device.
    //
    // This should be done in a threadsafe way to allow safe inter-device communication.
    int32_t (*interrupt)(void*, uint32_t);

    // Device, Motherboard, Motherboard Functions
    // Function to use to register the motherboard's functions with this device.
    //
    // Can keep the pointer to the motherboard, but need to copy the motherboard
    // functions.
    //
    // This function is called before init, reset, or boot.
    int32_t (*register_motherboard)(void*, void*, struct MotherboardFunctions*);
};

struct MotherboardFunctions {
    // Callback for a device to read a byte of memory.
    //
    // Should take four arguments:
    // - The motherboard
    // - The (global) address to read from
    // - The number of bytes to read
    // - An array of bytes to write to (assumed to be large enough)
    int32_t (*read_bytes)(void*, uint64_t, uint32_t, uint8_t*);

    // Callback for a device to write a byte of memory.
    //
    // Should take four arguments:
    // - The motherboard
    // - The (global) address to write to
    // - The number of bytes to write
    // - An array of bytes to read from (assumed to be large enough)
    int32_t (*write_bytes)(void*, uint64_t, uint32_t, uint8_t*);

    // Motherboard, Target Device, Interrupt Code.
    // Callback for a device to send an interrupt to another device.
    int32_t (*send_interrupt)(void*, uint32_t, uint32_t);
};

// Create a motherboard with a pluggable device capacity of max_devices
//
// To dealocate a motherboard created with this function, always pass the resulting
// pointer to bscomp_motherboard_destroy.
void* bscomp_motherboard_new(struct MotherboardConfig* config);

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

// Boot the motherboard, and hang until shutdown.
//
// If you want to be able to kill from outside teh box, launch this in a separate thread.
int32_t bscomp_motherboard_boot(void* motherboard);

// Halt the motherboard
int32_t bscomp_motherboard_halt(void* motherboard);

// Reboot the motherboard
int32_t bscomp_motherboard_reboot(void* motherboard);

#endif // bscomp_motherboard_h
