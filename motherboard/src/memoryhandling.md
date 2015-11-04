% Memory

All devices attached to a motherboard communicate in two ways: mapped memory and
interrupts. This document details how memory mapping is done.

All devices are limited to 2^32-1 bytes of mapped memory, Their memory size is stored as a
32-bit unsigned integer. The last byte in any mapped memory cell, at address
`0xXXXXXXXXFFFFFFFF` will always read zero, though future changes may switch to have this
map to some useful value.

Becauses all devices are limited to blocks of 2^32 bytes, memory mapping proceeds in
blocks of 2^32 bytes. That is, each device's mapped memory starts with its first address
on byte `i<<32` where `i` is the device's index in the memory mapping table.

# Reserved Blocks

Of the available spaces for mapping memory, one is reserved.

The last block, starting at byte 0xFFFFFFFF00000000, is used by the motherboard to store
information about available devices on the motherboard. This block is not writable, but
is readable, and can be used by devices to discover information about what other devices
are around them.

# Information Block Layout

The block of motherboard information contains two tables of information: the Ram table and
the Device table. The first few bytes of memory contain information on where to find these
tables:

 Byte Index | Type | Contents
------------|------|-------------------------------------
 0          | u64  | Pointer to the table of Ram Data
 8          | u64  | Pointer to the table of Device Data

## Ram Data Table

The first 4 bytes of the ram data table are a 32 bit unsigned integer indicating the
number of entries in the table. Each entry consists of one 32 bit unsigned integer
indicating how many bytes are mapped in that block. As described above, this number is
actually the index of the last mapped byte.

The index in the ram data table of an entry is equivalent to its block index. So the first
block in the table has index 0 and is mapped starting at 0x0000000000000000, the second
block has index 1 and is mapped starting at 0x0000000100000000.

An example table is shown below.

 Byte Offset | Contents | Table Index | Mapped Bytes
-------------|----------|-------------|-----------------------------------------
 0           | 3        | N/A         | N/A
 4           | 0x10     | 0           | 0x0000000000000000 - 0x000000000000000F
 8           | 0x80     | 1           | 0x0000000100000000 - 0x000000010000007F
 12          | 0x0      | 2           | None

## Device Data Table

The first 4 bytes of the device data table are a 32 bit unsigned integer indicating the
number of entries in the table. Each entry is 16 bytes long and consists of four elements:

 Byte Offset | Type | Contents
-------------|------|--------------------------
 0           | u64  | Device Type Identifier
 8           | u32  | Unique Device Identifier
 12          | u32  | Mapped Memory Index

The layout of the upper 32 bits of the Device Type Identifier are not specified by the
motherboard, but should be unique to that type of device. The lower 32 bits are reserved
for flags to tell the motherboard about the type of device. Note that these flags are
still considered part of the device type identifier, and must still be consistent across
the device type for it to be considered the same type. Their layout is given in the
following table:

 Bit (from least significant) | Meaning/Values
------------------------------|--------------------------------------------------------------------
 0                            | Memory mapping flag: 1 if memory should be mapped, zero otherwise.
 1-31                         | Unused

The mapped memory index is the index of that device's mapped memory information in the ram
data table.

# Safety

The motherboard does not guarantee memory access safety in any way. Because internal
handling uses functions for passing memory, not a bus, there are no bus collisions, but
there are no guarantees about interleaving memory accesses. Devices are expected to
implement their own rules for memory access and use interrupt handlers to indicate
statuses.
