#!/bin/bash

echo "Iniciando kernel memory..."
gnome-terminal -- bash -c "cd kernel_memory && ./bin/kernel_memory kernel_memory.config; exec bash"

echo "Iniciando kernel scheduler..."
gnome-terminal -- bash -c "cd kernel_scheduler && ./bin/kernel_scheduler kernel_scheduler.config PROCESO_INICIAL.prc; exec bash"


