#!/usr/bin/env python3
import struct
import threading
import time

from computer import sodevice, rdmadevice

def main():
    ram_config = struct.pack('@I', 4 * (1<<10))
    ramdev = sodevice.SODevice('ram/libbridgesimram.so', ram_config)
    rdmadev = rdmadevice.RDMADevice('127.0.0.1', 8080)

    cpu_config = struct.pack('@I', 32)
    cpudev = sodevice.SODevice('stack-cpu/libbridgesimstackcpu.so', cpu_config)


    mb_config = struct.pack('@I', 16)
    motherboard = sodevice.SOMotherboard(
        'motherboard/target/debug/libmotherboard.so', mb_config)

    motherboard.add_device(ramdev)
    motherboard.add_device(rdmadev)
    motherboard.add_device(cpudev)

    motherboard_thread = threading.Thread(target=motherboard.boot, daemon=True)

    motherboard_thread.start()

    try:
        while motherboard_thread.is_alive():
            time.sleep(1)
    except KeyboardInterrupt:
        motherboard.halt()
        motherboard_thread.join()

if __name__ == '__main__':
    main()
