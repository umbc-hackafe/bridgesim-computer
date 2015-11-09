from libc.stdint cimport *
from libc.string cimport memset
from cpython.mem cimport PyMem_Malloc, PyMem_Free

cdef public enum:
    rdma_device_type_id = <uint64_t>2 << <uint64_t>32

cdef uint32_t next_device_id = 0

rdma_devices = set()

cdef extern from "motherboard.h":
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
        int32_t (*boot)(void*) except -1
        int32_t (*interrupt)(void*, uint32_t) except -1
        int32_t (*register_motherboard)(void*, void*, MotherboardFunctions*);

    struct MotherboardFunctions:
        int32_t (*read_bytes)(void*, uint64_t, uint32_t, uint8_t*)
        int32_t (*write_bytes)(void*, uint64_t, uint32_t, uint8_t*)
        int32_t (*send_interrupt)(void*, uint32_t, uint32_t)

cdef class RDMADevice:
    cdef Device* device
    cdef address, port

    def __cinit__(self):
        self.device = <Device*>PyMem_Malloc(sizeof(Device))
        if not self.device:
            raise MemoryError()

        memset(self.device, 0, sizeof(Device)) 
        
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
        print('RDMA Init: {}:{}'.format(address, port))
        self.address = address
        self.port = port

    def boot(self):
        print('RDMA Device booted!')

cdef public Device* bscomp_rdma_device_new(char* bind_address, int port) with gil:
    dev = RDMADevice(bind_address.decode('ascii'), port)
    rdma_devices.add(dev)

    return dev.device

cdef public void bscomp_rdma_device_destroy(Device* device) with gil:
    if not device:
        return

    cdef RDMADevice dev = <RDMADevice?>device.device
    if dev is None:
        return
    device.device = NULL
    rdma_devices.discard(dev)
    
cdef int32_t boot(void* device) except -1 with gil:
    if not device:
        raise ValueError('Expected an rdma device, got null')
    cdef RDMADevice dev = <RDMADevice?>device 
    if dev is None:
        raise ValueError('Expected an rdma device, got None')

    dev.boot()
    
cdef int32_t interrupt(void* device, uint32_t code) except -1 with gil:
    pass
