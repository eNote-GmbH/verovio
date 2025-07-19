#!/bin/sh

# Requires python
# Requires svgpathtools python library (install with 'pip3 install svgpathtools')

PYTHON="python3"

echo "Generating C++ header file ..."
$PYTHON generate.py smufl

echo "Generating Bravura files ..."
$PYTHON generate.py extract Bravura
$PYTHON generate.py css Bravura $@

echo "Generating Leipzig files ..."
$PYTHON generate.py check Leipzig
$PYTHON generate.py extract Leipzig
$PYTHON generate.py css Leipzig $@

echo "Generating Cantata files ..."
$PYTHON generate.py extract Cantata
$PYTHON generate.py css Cantata $@

echo "Generating Leland files ..."
$PYTHON generate.py extract Leland
$PYTHON generate.py css Leland $@

echo "Done!"
