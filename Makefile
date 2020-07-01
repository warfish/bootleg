NASM ?= nasm
OBJCOPY ?= objcopy
CC = gcc
OBJS = entry16.o start64.o dataseg.o libstd/vfprintf.o heap.o apic.o
CFLAGS = -Wall -std=c11 -ffreestanding -nostdlib -m64 -mcmodel=large -mno-red-zone -fno-stack-protector -fno-pic -Iinclude -Iinclude/libstd -Os
LIBGCC = $(shell $(CC) -m64 -print-libgcc-file-name)

all: bios.bin

bios.bin: bootleg.elf64
	$(OBJCOPY) -O binary $< $@

bootleg.elf64: $(OBJS) image.lds
	$(LD) -T image.lds -Map=$@.map --gc-sections -nostdlib -o $@ $(OBJS) $(LIBGCC)

%.o: %.asm
	$(NASM) -iinclude/ -felf64 -o $@ $<

clean:
	@rm -f $(OBJS) *.elf64 *.bin *.map

.PHONY: all clean
