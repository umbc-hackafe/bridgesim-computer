from base_device cimport Device, MotherboardFunctions
cimport base_device

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

    def __init__(self, soname):
        self.soname = str(soname)

        self.shared_object = dlopen(self.soname, RTLD_NOW | RTLD_GLOBAL)
        if not self.shared_object:
            raise LoadError('Unable to load {}.'.format(self.soname))

        self.create_func = <Device* (*)(void*) nogil>dlsym(
            self.shared_object, 'bscomp_device_new')

        if not self.create_func:
            raise LoadError(
                '{} does not contain required function "bscomp_device_new".'
                .format(self.soname))      

        self.destroy_func = <void (*)(Device*) nogil>dlsym(
            self.shared_object, 'bscomp_device_destroy')
        if not self.destroy_func:
            raise LoadError(
                '{} does not contain required function "bscomp_device_destroy".'
                .format(self.soname))

    def __dealloc__(self):
        if not self.device and not self.shared_object:
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
            # Todo cleanup
            pass
