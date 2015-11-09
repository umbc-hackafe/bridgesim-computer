#include <Python.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "motherboard.h"
#include "ram.h"
#include "rdma.h"

int initialize_interpreter() {
    Py_Initialize();
    PyEval_InitThreads();

    PyObject* pathadd = PyUnicode_FromString("../rdma");
    if (!pathadd) {
        PyErr_Print();
        Py_Finalize();
        return -1;
    }

    PyObject* path = PySys_GetObject("path");
    if (!path) {
        printf("Failed to get sys.path");
        Py_DECREF(pathadd);
        Py_Finalize();
        return -1;
    }

    if (!PyList_Check(path)) {
        printf("sys.path failed PyList_Check");
        Py_DECREF(pathadd);
        Py_Finalize();
        return -1;
    }

    if (PyList_Append(path, pathadd)) {
        PyErr_Print();
        Py_DECREF(pathadd);
        Py_Finalize();
        return -1;
    }

    Py_DECREF(pathadd);
    pathadd = 0;

    path = 0;

    PyInit_rdma();

    return 0;
}

void finalize_interpreter() {
    Py_Finalize();
}

int main() {
    int err = initialize_interpreter();
    if (err) {
        printf("Failed to initialize Python.\n");
        return -1;
    }
    printf("Initialized Python.\n");

    struct Device* rdma_device = bscomp_rdma_device_new("127.0.0.1", 9000);
    if (!rdma_device) {
        printf("Failed to create rdma device.\n");
        finalize_interpreter();
        return -1;
    }
    printf("Created an rdma device at %p.\n", rdma_device->device);

    // Create a ram device with 256KiB
    struct Device* ram_device = bscomp_ram_device_new(0x40000);
    if (!ram_device) {
        printf("Failed to create a ram device.\n");
        bscomp_rdma_device_destroy(rdma_device);
        finalize_interpreter();
        return -1;
    }
    printf("Created a ram device at %p.\n", ram_device->device);

    // Create a motherboard with space for four devices
    void* mb = bscomp_motherboard_new(4);
    if (!mb) {
        printf("Failed to create a motherboard.\n");
        bscomp_ram_device_destroy(ram_device);
        bscomp_rdma_device_destroy(rdma_device);
        finalize_interpreter();
        return -1;
    }
    printf("Created a motherboard at %p.\n", mb);

    // Add the ram device to the motherboard
    err = bscomp_motherboard_add_device(mb, ram_device);
    if (err) {
        printf("Failed to attach ram to the motherboard.\n");
        bscomp_motherboard_destroy(mb);
        bscomp_ram_device_destroy(ram_device);
        bscomp_rdma_device_destroy(rdma_device);
        finalize_interpreter();
        return -1;
    }
    printf("Attached the ram device to the motherboard.\n");

    // Attach the rdma device.
    err = bscomp_motherboard_add_device(mb, rdma_device);
    if (err) {
        printf("Failed to attach rdma to the motherboard.\n");
        bscomp_motherboard_destroy(mb);
        bscomp_ram_device_destroy(ram_device);
        bscomp_rdma_device_destroy(rdma_device);
        finalize_interpreter();
        return -1;
    }
    printf("Attached rdma device to motherboard.\n");

    printf("Booting!\n");
    err = bscomp_motherboard_boot(mb);
    if (err) {
        printf("Boot exited with error %d.\n", err);
    } else {
        printf("System shutdown ok.\n");
    }

    // Cleanup the motherboard
    bscomp_motherboard_destroy(mb);
    mb = 0;
    printf("Destroyed motherboard.\n");

    // Cleanup the ram device
    bscomp_ram_device_destroy(ram_device);
    printf("Destroyed ram device.\n");

    bscomp_rdma_device_destroy(rdma_device);
    printf("Destroyed rdma device.\n");

    finalize_interpreter();
    printf("Finalized Python.\n");

    return 0;
}
