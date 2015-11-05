#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "motherboard/motherboard.h"
#include "ram/ram.h"

void* mb = 0;

void handle_sigint(int);

int main() {
    // Create a ram device with 256KiB
    struct Device* ram_device = bscomp_make_ram_device(0x40000);
    printf("Created a ram device at %p.\n", ram_device);

    // Create a motherboard with space for four devices
    mb = bscomp_motherboard_new(4);
    printf("Created a motherboard at %p.\n", mb);

    // Add the ram device to the motherboard
    bscomp_motherboard_add_device(mb, ram_device);
    printf("Attached the ram device to the motherboard.\n");

    printf("Booting!\n");
    bscomp_motherboard_boot(mb);

    // Cleanup the motherboard
    bscomp_motherboard_destroy(mb);
    mb = 0;
    printf("Destroyed motherboard.\n");

    // Cleanup the ram device
    bscomp_delete_ram_device(ram_device);
    printf("Destroyed ram device.\n");

    return 0;
}
