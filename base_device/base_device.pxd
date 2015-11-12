from libc.stdint cimport *

cdef extern from "motherboard.h":
    struct Device:
        void* device
        uint64_t device_type
        uint32_t device_id
        uint32_t export_memory_size
        int32_t (*load_bytes)(void*, uint32_t, uint32_t, uint8_t*) except -1
        int32_t (*write_bytes)(void*, uint32_t, uint32_t, uint8_t*) except -1
        int32_t (*init)(void*) except -1
        int32_t (*reset)(void*) except -1
        int32_t (*cleanup)(void*) except -1
        int32_t (*boot)(void*) except -1
        int32_t (*interrupt)(void*, uint32_t) except -1
        int32_t (*register_motherboard)(void*, void*, MotherboardFunctions*) except -1

    struct MotherboardFunctions:
        int32_t (*read_bytes)(void*, uint64_t, uint32_t, uint8_t*) except -1
        int32_t (*write_bytes)(void*, uint64_t, uint32_t, uint8_t*) except -1
        int32_t (*send_interrupt)(void*, uint32_t, uint32_t) except -1

cdef class BaseDevice:
    cdef Device* device

cdef class CallbackDevice(BaseDevice):
    cdef MotherboardFunctions motherboard_funcs
    cdef void* motherboard

    cdef register_motherboard(self, void* motherboard, MotherboardFunctions* mbfuncs)

cdef int32_t register_motherboard(
    void* device, void* motherboard, MotherboardFunctions* mbfuncs) except -1 with gil
