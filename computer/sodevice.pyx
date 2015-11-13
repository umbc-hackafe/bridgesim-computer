from libc.stdint cimport *
from computer.base_device cimport Device, MotherboardFunctions
from computer cimport base_device

cdef extern from "dlfcn.h":
    void* dlopen(const char* file, int mode) nogil
    void* dlsym(void* handle, const char* name) nogil
    int dlclose(void* handle) nogil
    char* dlerror() nogil

    unsigned int RTLD_LAZY
    unsigned int RTLD_NOW
    unsigned int RTLD_GLOBAL
    unsigned int RTLD_LOCAL

class DeviceError(Exception):
    pass

class LoadError(DeviceError):
    pass

class StateError(DeviceError):
    pass

cdef str string_check(s):
    if type(s) is str:
        return <str>s
    elif isinstance(s, str):
        return str(s)
    else:
        raise TypeError('Not a string: {}'.format(s))

cdef class SOMotherboard:
    cdef void* shared_object
    cdef void* (*create_func)(void*) nogil
    cdef void (*destroy_func)(void*) nogil
    cdef uint32_t (*num_slots_func)(void*) nogil
    cdef uint32_t (*slots_filled_func)(void*) nogil
    cdef int32_t (*is_full_func)(void*) nogil
    cdef int32_t (*add_device_func)(void*, Device*) nogil except -1
    cdef int32_t (*boot_func)(void*) nogil except -1
    cdef int32_t (*halt_func)(void*) nogil except -1
    cdef int32_t (*reboot_func)(void*) nogil except -1

    cdef readonly str soname

    cdef void* motherboard

    def __cinit__(self):
        self.shared_object = NULL
        self.create_func = NULL
        self.destroy_func = NULL
        self.num_slots_func = NULL
        self.slots_filled_func = NULL
        self.is_full_func = NULL
        self.add_device_func = NULL
        self.boot_func = NULL
        self.halt_func = NULL
        self.reboot_func = NULL
        self.motherboard = NULL

    def __init__(self, soname, constructor_data):
        self.soname = string_check(soname)
        cdef bytes soname_bytes = self.soname.encode('utf-8')

        if constructor_data is None:
            constructor_arg = NULL
        elif isinstance(constructor_data, bytes):
            constructor_arg = constructor_data
        else:
            raise TypeError('Constructor data must be bytes or None')
        
        with nogil:
            self.shared_object = dlopen(soname_bytes, RTLD_NOW | RTLD_GLOBAL)
        if not self.shared_object:
            raise LoadError('Unable to load {}.'.format(self.soname))

        with nogil:
            self.create_func = <void* (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_new')
        if not self.create_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_new".'
                .format(self.soname))      

        with nogil:
            self.destroy_func = <void (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_destroy')
        if not self.destroy_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_destroy".'
                .format(self.soname))

        with nogil:
            self.num_slots_func = <uint32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_num_slots')
        if not self.num_slots_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_num_slots".'

        with nogil:
            self.slots_filled_func = <uint32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_slots_filled')
        if not self.slots_filled_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_slots_filled".'

        with nogil:
            self.is_full_func = <int32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_is_full')
        if not self.is_full_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_is_full".'

        with nogil:
            self.add_device_func = <int32_t (*)(void*, Device*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_add_device')
        if not self.add_device_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_add_device".'

        with nogil:
            self.boot_func = <int32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_boot')
        if not self.boot_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_boot".'

        with nogil:
            self.halt_func = <int32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_halt')
        if not self.halt_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_halt".'

        with nogil:
            self.reboot_func = <int32_t (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_motherboard_reboot')
        if not self.reboot_func:
            raise LoadError(
                '{} does not contain required function "bscomp_motherboard_reboot".'


    def __dealloc__(self):
        if not self.motherboard and not self.shared_object:
            # nothing to deallocate
            return

        if self.motherboard:
            if self.destroy_func:
                with nogil:
                    self.destroy_func(self.motherboard)
                    self.motherboard = NULL
            else:
                raise StateError('Cannot deallocate motherboard! No destroy function!')

        self.create_func = NULL
        self.destroy_func = NULL
        self.num_slots_func = NULL
        self.slots_filled_func = NULL
        self.is_full_func = NULL
        self.add_device_func = NULL
        self.boot_func = NULL
        self.halt_func = NULL
        self.reboot_func = NULL

        if self.shared_object:
            dlclose(self.shared_object)


cdef class SODevice(base_device.BaseDevice):
    cdef void* shared_object
    cdef Device* (*create_func)(void*) nogil
    cdef void (*destroy_func)(Device*) nogil

    cdef readonly str soname

    def __cinit__(self):
        self.device = NULL
        self.shared_object = NULL
        self.create_func = NULL
        self.destroy_func = NULL

    def __init__(self, soname, constructor_data):
        self.soname = str_check(soname)
        cdef bytes soname_bytes = soname.encode('utf-8')

        cdef char* constructor_arg

        if constructor_data is None:
            constructor_arg = NULL
        elif isinstance(constructor_data, bytes):
            constructor_arg = constructor_data
        else:
            raise TypeError('Constructor data must be bytes or None')

        with nogil:
            self.shared_object = dlopen(soname_bytes, RTLD_NOW | RTLD_GLOBAL)
        if not self.shared_object:
            raise LoadError('Unable to load {}.'.format(self.soname))

        with nogil:
            self.create_func = <Device* (*)(void*) nogil>dlsym(
                self.shared_object, 'bscomp_device_new')
        if not self.create_func:
            raise LoadError(
                '{} does not contain required function "bscomp_device_new".'
                .format(self.soname))      

        with nogil:
            self.destroy_func = <void (*)(Device*) nogil>dlsym(
                self.shared_object, 'bscomp_device_destroy')
        if not self.destroy_func:
            raise LoadError(
                '{} does not contain required function "bscomp_device_destroy".'
                .format(self.soname))

        with nogil:
            self.device = self.create_func(<void*>constructor_arg)
        if not self.device:
            raise LoadError('Unable to create a device!')

    def __dealloc__(self):
        if not self.device and not self.shared_object:
            # nothing to deallocate
            return

        if self.device:
            if self.destroy_func:
                with nogil:
                    self.destroy_func(self.device)
                    self.device = NULL
            else:
                raise StateError('Cannot deallocate device! No destroy function!')
        self.create_func = NULL
        self.destroy_func = NULL
        if self.shared_object:
            with nogil:
                dlclose(self.shared_object)
