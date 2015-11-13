#!/usr/bin/env python3

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

extensions = [
    Extension(
        'base_device', ['base_device.pyx'],
        include_dirs=[
            '../motherboard/include',
        ],
    ),
    Extension(
        'sodevice', ['sodevice.pyx'],
        include_dirs=[
            '../motherboard/include',
        ],
    ),
]

setup(
    name='Bridgesim Computer Python Base-Device Module',
    ext_modules=cythonize(extensions),
)
