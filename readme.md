# Bridgesim Ship's Computer

This is a set of dynamically loadable libraries to implement a Ship's Computer for
Bridgesim. The computer is broken into modules which are dynamically loadable so that
different computers can be assembled and run from different instances of the same types of
components. These dynamic loading components use the C function interface to allow new
modules to be created in nearly any language.

An include directory is provided with header files describing the contents of the
different Rust modules. This should make it easier to create non-rust modules.
