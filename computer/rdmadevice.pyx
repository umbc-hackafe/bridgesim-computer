from libc.stdint cimport *
from libc.string cimport memset
from cpython.mem cimport PyMem_Malloc, PyMem_Free

import queue

from computer.base_device cimport Device, MotherboardFunctions
from computer cimport base_device

cdef enum:
    rdma_device_type_id = <uint64_t>2 << <uint64_t>32

cdef uint32_t next_device_id = 0

cdef class RDMADevice(base_device.CallbackDevice):
    cdef address, port, interrupts

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
        self.device.register_motherboard = base_device.register_motherboard

    def __dealloc__(self):
        PyMem_Free(self.device)

    def __init__(self, address, port):
        print('RDMA Init: {}:{}'.format(address, port))
        self.address = address
        self.port = port
        self.interrupts = queue.Queue()

    def boot(self):
        print(self, 'booted!')

    def interrupt(self, code):
        print('{} received interrupt {}'.format(self, code))
        self.interrupts.put(code)

    def __repr__(self):
        return '<' + str(self) + '>'

    def __str__(self):
        if self.device:
            return 'RDMA Device {}'.format(self.device.device_id)
        else:
            return 'RDMA Device NULL'

cdef int32_t boot(void* device) except -1 with gil:
    if not device:
        raise ValueError('Expected an rdma device, got null')
    cdef RDMADevice dev = <RDMADevice?>device
    if dev is None:
        raise ValueError('Expected an rdma device, got None')

    dev.boot()

cdef int32_t interrupt(void* device, uint32_t code) except -1 with gil:
    if not device:
        raise ValueError('Expected an rdma device, got null')
    cdef RDMADevice dev = <RDMADevice?>device
    if dev is None:
        raise ValueError('Expected an rdma device, got None')

    dev.interrupt(code)
