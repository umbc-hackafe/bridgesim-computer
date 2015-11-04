% Thread Safety

The motherboard dies not gurantee thread safety, and devices can expect multiple threads
to access their properties at the same time. Memory read/write safety is expected to be
handled by having devices establish access protocols.

Interrupt handler functions are expected to be implemented in a thread-safe way, so that
any thread can send an interrupt and have it received by the device within the limits of
that device's interrupt-vector-size.
