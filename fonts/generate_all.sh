#!/bin/sh

# Requires python
# Requires svgpathtools python library (install with 'pip3 install svgpathtools')

PYTHON="python3.9"

echo "Generating C++ header file ..."
$PYTHON generate.py smufl

echo "Generating Bravura files ..."
$PYTHON generate.py extract Bravura
$PYTHON generate.py css Bravura

echo "Generating Leipzig files ..."
$PYTHON generate.py check Leipzig
$PYTHON generate.py extract Leipzig
$PYTHON generate.py css Leipzig

echo "Generating Gootville files ..."
$PYTHON generate.py extract Gootville
$PYTHON generate.py css Gootville

echo "Generating Petaluma files ..."
$PYTHON generate.py extract Petaluma
$PYTHON generate.py css Petaluma

echo "Generating Leland files ..."
$PYTHON generate.py extract Leland
$PYTHON generate.py css Leland

echo "Generating Legato files ..."
$PYTHON extract-bounding-boxes.py Legato.svg json/legato_metadata.json ../data/Legato.xml

echo "Generating Maestro files ..."
$PYTHON extract-bounding-boxes.py Maestro.svg json/maestro_metadata.json ../data/Maestro.xml

echo "Done!"
