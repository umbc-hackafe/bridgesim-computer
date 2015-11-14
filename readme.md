# Bridgesim Ship's Computer

This is a set of dynamically loadable libraries to implement a Ship's Computer for
Bridgesim. The computer is broken into modules which are dynamically loadable so that
different computers can be assembled and run from different instances of the same types of
components. These dynamic loading components use the C function interface to allow new
modules to be created in nearly any language.

C headers will be provided for any publicly accessable Rust code.

To make this accessible for Bridgesim, the system is wrapped in Cython modules for Python,
which should make it easy to import components and start they computer from Python. It is
still completely possible to use the computer core without Python, but the example program
to run a basic computer is written in Python.

## Building

You will need Rust >= 1.4.0, Cargo >= 0.4.0, an implementation of `make`, an
implementation of `cc`, Python 3, setuptools >= 18, and Cython >= 0.22. If you find that
there are other dependencies not listed here, file a bug.

To build and run the test program, first build the shared objects for the Motherboard and
RAM:

```bash
(cd motherboard && cargo build)

make -C ram
```

Next build the Cython extensions:

```bash
./setup.py build_ext --inplace
```

And run!

```bash
./run.py
```
