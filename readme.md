# Bridgesim Ship's Computer

This is a set of dynamically loadable libraries to implement a Ship's Computer for
Bridgesim. The computer is broken into modules which are dynamically loadable so that
different computers can be assembled and run from different instances of the same types of
components. These dynamic loading components use the C function interface to allow new
modules to be created in nearly any language.

An include directory is provided with header files describing the contents of the
different Rust modules. This should make it easier to create non-rust modules.

# Building

I'm currently focused on development, not deployment, so I haven't bothered to properly
set up something like automake or cmake for this project. It's just some (rather brittle)
makefiles for the C code, and Cargo for the Rust code.

You will need Rust, Cargo, Make, and CC. If there are other, more specific dependencies,
they're things I happened to have installed already and don't know what they are (good
luck with that).

I'm only officially supporting rustc 1.3.0+, cargo 0.4.0+, since those are the minimum
versions that I have.

To build and run the test program:

```bash
cd motherboard
cargo build

cd ../ram
make

cd ../example_runner
make # or make run to build and run in one step
make run
```
