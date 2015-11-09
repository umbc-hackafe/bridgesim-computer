#!/usr/bin/env python3

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [
    Extension(
        'rdma', ['rdma.pyx'],
        include_dirs=['../motherboard/include'],
    ),
]

setup(
    name='RDMA Module for Bridgesim Computer',
    ext_modules=cythonize(extensions)
)
