#!/bin/bash

echo "Compilando kernel memory..."
cd kernel_memory || exit
make clean
make
cd ..

echo "Compilando kernel scheduler..."
cd kernel_scheduler || exit
make clean
make
cd ..

echo "Compilando memory stick..."
cd memory_stick || exit
make clean
make
cd ..

echo "Compilando swap..."
cd swap || exit
make clean
make
cd ..

echo "Compilando io..."
cd io || exit
make clean
make
cd ..

echo "Compilando cpu..."
cd cpu || exit
make clean
make
cd ..

echo "Build completo ✔"
