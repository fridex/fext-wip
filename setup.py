from setuptools import setup
from setuptools import Extension

eheapq_module = Extension("eheapq", sources=["fext/eheapq.cpp"])
edict_module = Extension("edict", sources=["fext/edict.cpp"])

setup(
    name="fext",
    version="0.0.0",
    description="An extension to standard Python's heapq",
    long_description="An extension to standard Python's heapq for performance applications.",
    author="Fridolin Pokorny",
    author_email="fridex.devel@gmail.com",
    license="GPLv3+",
    ext_modules=[
        eheapq_module,
        edict_module,
    ],
)
