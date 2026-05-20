#!/bin/bash

echo "Iniciando kernel memory..."
gnome-terminal -- bash -c "cd kernel_memory && ./bin/kernel_memory; exec bash"

echo "Iniciando kernel scheduler..."
gnome-terminal -- bash -c "cd kernel_scheduler && ./bin/kernel_scheduler; exec bash"

echo "Iniciando memory stick..."
gnome-terminal -- bash -c "cd memory_stick && ./bin/memory_stick; exec bash"

echo "Iniciando swap..."
gnome-terminal -- bash -c "cd swap && ./bin/swap; exec bash"

echo "Iniciando io..."
gnome-terminal -- bash -c "cd io && ./bin/io; exec bash"

echo "Iniciando cpu..."
gnome-terminal -- bash -c "cd cpu && ./bin/cpu; exec bash"
