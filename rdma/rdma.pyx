from libc.stdint cimport *

cdef extern from "Python.h":
    void PyEval_InitThreads()

cdef extern from "motherboard/motherboard.h":
    struct Device:
        void* device
        uint64_t device_type
        uint32_t device_id
        uint32_t export_memory_size
        int32_t (*load_bytes)(void*, uint32_t, uint32_t, uint8_t*)
        int32_t (*write_bytes)(void*, uint32_t, uint32_t, uint8_t*)
        int32_t (*init)(void*)
        int32_t (*reset)(void*)
        int32_t (*cleanup)(void*)
        int32_t (*boot)(void*)
        int32_t (*interrupt)(void*, uint32_t)
        int32_t (*register_motherboard)(void*, void*, MotherboardFunctions*);

    struct MotherboardFunctions:
        int32_t (*read_bytes)(void*, uint64_t, uint32_t, uint8_t*)
        int32_t (*write_bytes)(void*, uint64_t, uint32_t, uint8_t*)
        int32_t (*send_interrupt)(void*, uint32_t, uint32_t)

cdef class RDMADevice:
    pass

#cdef public 
