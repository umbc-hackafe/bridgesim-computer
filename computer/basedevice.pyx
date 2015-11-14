#cython: language_level=3
cdef class CallbackDevice(BaseDevice):
    cdef register_motherboard(self, void* motherboard, MotherboardFunctions* mbfuncs):
        print(self, 'registering motherboard')
        self.motherboard_funcs = mbfuncs[0]
        self.motherboard = motherboard

cdef int32_t register_motherboard(
        void* device, void* motherboard, MotherboardFunctions* mbfuncs) with gil:
    if not device:
        raise ValueError('Expected a callback device, got null')
    cdef CallbackDevice dev = <CallbackDevice?>device
    if dev is None:
        raise ValueError('Expected a callback device, got None')

    dev.register_motherboard(motherboard, mbfuncs)
