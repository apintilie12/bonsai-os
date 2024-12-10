#!/bin/bash

make

if [ $? -eq 0 ]; then
    #qemu-system-aarch64 -m 1G -M raspi3b -serial null -serial mon:stdio -nographic -kernel build/kernel8.elf
    qemu-system-aarch64 -M raspi3b -serial mon:stdio -serial null -kernel kernel8.img
fi
