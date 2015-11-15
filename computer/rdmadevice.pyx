#cython: language_level=3
from libc.stdint cimport *
from libc.string cimport memset
from cpython.mem cimport PyMem_Malloc, PyMem_Free

import flask
import queue
import requests
import threading

from computer.basedevice cimport Device, MotherboardFunctions
from computer cimport basedevice

class APIError(Exception):
    pass

cdef enum:
    rdma_device_type_id = <uint64_t>2 << <uint64_t>32

cdef uint32_t next_device_id = 0

cdef class RDMADevice(basedevice.CallbackDevice):
    cdef address, port
    cdef interrupts

    cdef app

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
        self.device.register_motherboard = basedevice.register_motherboard

    def __dealloc__(self):
        PyMem_Free(self.device)

    def __init__(self, address, port):
        print('RDMA Init: {}:{}'.format(address, port))
        self.address = address
        self.port = port
        self.interrupts = queue.Queue()

        self.app = flask.Flask(__name__)
        self.app.add_url_rule(
            rule='/shutdown', view_func=self.fshutdown, methods=['POST'],
        )

        self.app.add_url_rule(
            rule='/read-bytes', view_func=self.freadbytes, methods=['POST'],
        )

        self.app.add_url_rule(
            rule='/write-bytes', view_func=self.fwritebytes, methods=['POST'],
        )

        self.app.add_url_rule(
            rule='/send-interrupt', view_func=self.fsendinterrupt, methods=['POST'],
        )

        self.app.errorhandler(APIError)(self.ferrorhandler)

    def boot(self):
        print(self, 'booted!')

        run_kwargs = dict(
            host=self.address, port=self.port, use_evalex=False, debug=True,
            use_reloader=False,
        )
        app_run_thread = threading.Thread(target=self.app.run, daemon=True, kwargs=run_kwargs)
        app_run_thread.start()

        for code in iter(self.interrupts.get, 0):
            pass

        requests.post('http://{}:{}/shutdown'.format(self.address, self.port))

        join_tries = 0
        while app_run_thread.is_alive():
            join_tries += 1
            print('Trying to join the app thread. ({} attempts)'.format(join_tries))
            app_run_thread.join(1)
            if app_run_thread.is_alive():
                try:
                    requests.post('http://{}:{}/shutdown'.format(self.address, self.port))
                except:
                    pass

        print(self, 'shutting down.')

    def interrupt(self, code):
        print('{} received interrupt {}'.format(self, code))
        self.interrupts.put(code)

    def ferrorhandler(self, err):
        return flask.jsonify(status='ERROR', message=str(err))

    def fshutdown(self):
        func = flask.request.environ.get('werkzeug.server.shutdown')
        if func is None:
            raise RuntimeError('Not running with the Werkzeug Server')
        func()
        return 'Shutting down flask server.'

    def freadbytes(self):
        data = flask.request.get_json(silent=True)
        if not data:
            raise APIError('No data!')

        address = data.get('address')
        if address is None:
            raise APIError('Must specify an address!')

        try:
            address.to_bytes(8, byteorder='big', signed=False)
        except:
            raise APIError('Address must be a valid uint64_t.')

        read_length = data.get('length')
        if read_length is None:
            raise APIError('Must specify a number of bytes to read.')

        try:
            read_length.to_bytes(4, byteorder='big', signed=False)
        except:
            raise APIError('Read length must be a valid uint32_t.')

        cdef uint64_t addr = address
        cdef uint32_t read_len = read_length

        result = bytearray(read_len)
        cdef uint8_t* res = result

        cdef int32_t ret

        with nogil:
            ret = self.motherboard_funcs.read_bytes(self.motherboard, addr, read_len, res)

        if ret:
            raise APIError('Read failed with code {}.'.format(ret))

        return flask.jsonify(status='OK', data=list(result))

    def fwritebytes(self):
        data = flask.request.get_json(silent=True)
        if not data:
            raise APIError('No data!')

        address = data.get('address')
        if address is None:
            raise APIError('Must specify an address!')

        try:
            address.to_bytes(8, byteorder='big', signed=False)
        except:
            raise APIError('Address must be a valid uint64_t.')

        content = data.get('data')

        if content is None:
            raise APIError('Must provide data to set.')

        try:
            content = bytes(content)
        except:
            raise APIError('Data must be valid bytes.')

        try:
            len(content).to_bytes(4, 'big', signed=False)
        except:
            raise APIError('Data length must be a valid uint32_t.')

        cdef uint64_t addr = address
        cdef uint32_t write_len = len(content)

        cdef int32_t ret

        cdef uint8_t* cont = content

        with nogil:
            ret = self.motherboard_funcs.read_bytes(self.motherboard, addr, write_len, cont)

        if ret:
            raise APIError('Write failed with code {}'.format(ret))

        return flask.jsonify(status='OK')

    def fsendinterrupt(self):
        data = flask.request.get_json(silent=True)
        if not data:
            raise APIError('No data!')

        device = data.get('device')
        if device is None:
            raise APIError('Must specify a device.')

        try:
            device.to_bytes(4, byteorder='big', signed=False)
        except:
            raise APIError('Device number must be a valid uint32_t.')

        code = data.get('code')

        if code is None:
            raise APIError('Must provide an interrupt code.')

        try:
            code.to_bytes(4, byteorder='big', signed=False)
        except:
            raise APIError('Code must be a valide uint32_t.')

        cdef uint32_t dev = device
        cdef uint32_t cd = code

        cdef int32_t ret

        with nogil:
            ret = self.motherboard_funcs.send_interrupt(self.motherboard, dev, cd)

        if ret:
            raise APIError('Interrupt failed with code {}'.format(ret))

        return flask.jsonify(status='OK')

    def __repr__(self):
        return '<' + str(self) + '>'

    def __str__(self):
        if self.device:
            return 'RDMA Device {}'.format(self.device.device_id)
        else:
            return 'RDMA Device NULL'

cdef int32_t boot(void* device) with gil:
    if not device:
        print('Expected an rdma device, got null')
        return -1
    cdef RDMADevice dev = <RDMADevice?>device
    if dev is None:
        print('Expected an rdma device, got None')
        return -1

    try:
        dev.boot()
    except Exception as err:
        print(err)
        return -1
    return 0

cdef int32_t interrupt(void* device, uint32_t code) with gil:
    if not device:
        print('Expected an rdma device, got null')
        return -1
    cdef RDMADevice dev = <RDMADevice?>device
    if dev is None:
        print('Expected an rdma device, got None')
        return -1

    try:
        dev.interrupt(code)
    except Exception as err:
        print(err)
        return -1
    return 0
