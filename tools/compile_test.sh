#!/bin/bash -eux
#
# For development purposes, recompile core CVXcanon code and run tests. Intended
# to be run from root of repository.

# Use ccache to speed up compilation
# TODO(mwytock): Specific to OSX, Linux version?
export CC="ccache clang"
export CXX="ccache clang"

# Run swig
python_dir=src/python/cvxcanon
swig_base=cvxcanon_swig
swig -python -c++ -Isrc/cpp $python_dir/${swig_base}.i
mv $python_dir/${swig_base}_wrap.cxx $python_dir/${swig_base}_wrap.cpp

# Build
python setup.py build
python setup.py -q develop --user

# Test
nosetests cvxcanon