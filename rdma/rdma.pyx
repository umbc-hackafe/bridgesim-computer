from libc.stdint cimport *
from cpython.mem cimport PyMem_Malloc, PyMem_Free

cdef public enum:
    rdma_device_type_id = <uint64_t>2 << <uint64_t>32

cdef uint32_t next_device_id = 0

rdma_devices = set()

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
    cdef Device* device

    def __cinit__(self):
        self.device = <Device*>PyMem_Malloc(sizeof(Device))
        if not self.device:
            raise MemoryError()
        
        self.device.device = <void*>self

        global next_device_id
        self.device.device_type = rdma_device_type_id
        self.device.device_id = next_device_id
        next_device_id += 1

        self.device.boot = boot
        self.device.interrupt = interrupt

    def __dealloc__(self):
        PyMem_Free(self.device)

    def __init__(self, address, port):
        self.address = address
        self.port = port

cdef public Device* bscomp_new_rdma_device(char* bind_address, int port):
    dev = RDMADevice(bind_address.decode('ascii'), port)
    rdma_devices.add(dev)

    return dev.device
    
cdef int32_t boot(void* device):
    pass
    
cdef int32_t interrupt(void* device, uint32_t code):
    pass 
