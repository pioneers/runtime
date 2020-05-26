from setuptools import Extension, setup
from Cython.Build import cythonize

sourcefiles = [
    "studentapi.pyx",
    "../logger/logger.c"
]

setup(
    name="Student API",
    ext_modules = cythonize([
        Extension("studentapi", sources=sourcefiles)
    ]),
    zip_safe=False,
)
