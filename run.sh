#!/bin/bash

set -ex

QEMU=$(realpath ${QEMU:-../qemu/x86_64-softmmu/qemu-system-x86_64})
BIOS=$(realpath ${BIOS:-./bios.bin})
DEBUGCON=$(realpath ${DEBUGCON:-./debugcon.log})

$QEMU \
    -machine pc,accel=kvm \
    -cpu host \
    -bios $BIOS \
    -L . \
    -m 512 \
    -smp 2 \
    -display none \
    -chardev file,path=$DEBUGCON,id=debugcon \
    -device isa-debugcon,iobase=0x402,chardev=debugcon \
    -qmp unix:./qmp.sock,server,nowait \

