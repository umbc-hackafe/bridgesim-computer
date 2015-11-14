#!/usr/bin/env python3
import struct
import threading
import time

from computer import sodevice, rdmadevice

def main():
    ram_config = struct.pack('@I', 4 * (1<<10))
    ramdevice = sodevice.SODevice('ram/libbridgesimram.so', ram_config)
    rdmadev = rdmadevice.RDMADevice('127.0.0.1', 8080)

    mb_config = struct.pack('@I', 16)
    motherboard = sodevice.SOMotherboard(
        'motherboard/target/debug/libmotherboard.so', mb_config)

    motherboard.add_device(ramdevice)
    motherboard.add_device(rdmadev)

    motherboard_thread = threading.Thread(target=motherboard.boot, daemon=True)

    motherboard_thread.start()

    try:
        while True:
            time.sleep(10000000)
    except KeyboardInterrupt:
        motherboard.halt()
        motherboard_thread.join()

if __name__ == '__main__':
    main()
