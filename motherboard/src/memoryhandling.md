% Memory

There are two types of memory mapping for devices: RAM and I/O. Ram devices are assumed to
always be read/write-able at any point within their size, while I/O devices can have
memory addresses which serve I/O data and may not retain data written to them or provide
the same data for every read of the same address. Because of this, these two types of
memory are mapped to different locations by the motherboard.

All devices, I/O or Ram are limited to 2^32 bytes of mapped memory. Their memory size is
stored as a 32-bit unsigned integer; since a mapping of zero bytes would be the same as
not mapping any memory, the number of mapped bytes is taken to be the memory size
value + 1. Equivalently, the value stored in `export_memory_size` is the *index* of the
last byte of that device's mapped ram.

Becauses all devices are limited to blocks of 2^32 bytes, memory mapping proceeds in
blocks of 2^32 bytes. That is, each device's mapped memory starts on a 2^32 byte boundary.

# Reserved Blocks

Of the available spaces for mapping memory, two are reserved.

The first block, starting at byte zero, is used by the motherboard to store information
about available devices on the mother board. This block is not writable, but is readable,
and can be used by devices to discover information about what other devices are around
them.

The last block, starting at byte 0xFFFFFFFF00000000, is reserved for devices wishing to
perform internal memory mapping. Neither reads nor writes to it are serviced, so devices
are free to treat addresses in this range however they like. One example use is for the
CPU to memory map its own registers without making them available externally.

# Block Zero Layout

The first block of motherboard information contains three major tables of information: the
Ram table, the I/O table, and the Device table. The first few bytes of memory contain
information on where to find these tables:

 Byte Index | Type | Contents
------------|------|-------------------------------------
 0          | u64  | Pointer to the table of Ram Data
 8          | u64  | Pointer to the table of I/O Data
 16         | u64  | Pointer to the table of Device Data

## Ram Data Table

The first 4 bytes of the ram data table are a 32 bit unsigned integer indicating the
number of entries in the table. Each entry consists of one 32 bit unsigned integer
indicating how many bytes are mapped in that block. As described above, this number is
actually the index of the last mapped byte.

The table does not contain a entry for the first block, (i.e. the motherboard reserved
block). The index in the ram data table of an entry is equivalent to its block index in
memory. So the first block in the table has index 1 and is mapped starting at
0x0000000100000000.

Example:

 Byte Offset | Contents | Table Index | Mapped Bytes
-------------|----------|-------------|-----------------------------------------
 0           | 3        | 0           | N/A
 4           | 0x10     | 1           | 0x0000000100000000 - 0x0000000100000010
 8           | 0x80     | 2           | 0x0000000200000000 - 0x0000000200000080
 12          | 0x0      | 3           | 0x0000000300000000 - 0x0000000300000000

Notice that the thrid entry, which has contents zero still maps exactly one byte.

## I/O Data Table

The first 4 bytes of the io data table are a 32 bit unsigned integer indicating the number
of entries in the table. Each entry consists of one 32 bit unsigned integer indicating how
may bytes are mapped in that block. As described above, this number is actually the index
of the last mapped byte.

The index in the io table of an entry is equivalent to the index of its block in memory
starting from the end of memory. An example table is given below.

 Byte Offset | Contents | Table Index | Mapped Bytes
-------------|----------|-------------|-----------------------------------------
 0           | 3        | 0           | N/A
 4           | 0x10     | 1           | 0xfffffffe00000000 - 0xfffffffe00000010
 8           | 0x80     | 2           | 0xfffffffd00000000 - 0xfffffffd00000080
 12          | 0x0      | 3           | 0xfffffffc00000000 - 0xfffffffc00000000

## Device Data Table

The first 4 bytes of the device data table are a 32 bit unsigned integer indicating the
number of entries in the table. Each entry is 16 bytes long and consists of four elements:

 Byte Offset | Type | Contents
-------------|------|--------------------------
 0           | u32  | Device Type Identifier
 4           | u32  | Unique Device Identifier
 8           | u32  | Mapped Memory Type
 12          | u32  | Mapped Memory Index

The mapped memory type is a value from the enumeration `MappedMemoryType`, and the mapped
memory index is the table index of the mapped memory size in whichever memory table it
would be in.
