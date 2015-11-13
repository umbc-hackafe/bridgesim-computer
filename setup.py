#!/usr/bin/env python3

from setuptools import setup
from setuptools.extension import Extension

extensions = [
    Extension(
        'computer.base_device', ['computer/base_device.pyx'],
        include_dirs=[
            'motherboard/include',
        ],
    ),
    Extension(
        'computer.sodevice', ['computer/sodevice.pyx'],
        include_dirs=[
            'motherboard/include',
        ],
    ),
    Extension(
        'computer.rdmadevice', ['computer/rdmadevice.pyx'],
        include_dirs=[
            'motherboard/include',
        ],
    ),
]

setup(
    name='Bridgesim Computer Python Base-Device Module',
    setup_requires=['cython>=0.22'],
    ext_modules=extensions,
)
