from setuptools import Extension, setup
from Cython.Build import cythonize
import sys
import os

sourcefiles = [
    "../runtime_util/runtime_util.c",
    "../shm_wrapper/shm_wrapper.c",
    "../logger/logger.c",
    "../net_handler/net_util.c",
    "client/executor_client.c",
    "client/net_handler_client.c",
    "client/shm_client.c",
    "tester.pyx"
]

pbc_path = "../net_handler/pbc_gen"
pb_generated = os.listdir(pbc_path)
pb_generated = [os.path.join(pbc_path, file) for file in pb_generated if file.endswith('.c')]
print(pb_generated)

sourcefiles.extend(pb_generated)

libraries = ['rt', 'protobuf-c']

setup(
    name="Runtime Tester",
    ext_modules = cythonize([
        Extension("tester", sources=sourcefiles, libraries=libraries)
    ], compiler_directives={'language_level' : '3', 'boundscheck': False}),
    zip_safe=False,
)
