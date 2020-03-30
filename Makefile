NASM ?= nasm
OBJCOPY ?= objcopy
OBJS = entry16.o start32.o dataseg.o libstd/vfprintf.o heap.o
CFLAGS = -Wall -std=c11 -ffreestanding -nostdlib -m32 -march=i386 -fno-stack-protector -fno-pic -Iinclude -Iinclude/libstd -Os
LIBGCC = $(shell $(CC) -m32 -print-libgcc-file-name)

all: bios.bin

bios.bin: bootleg.elf32
	$(OBJCOPY) -O binary $< $@

bootleg.elf32: $(OBJS) image.lds
	$(LD) -T image.lds -Map=$@.map --gc-sections -nostdlib -o $@ $(OBJS) $(LIBGCC)

%.o: %.asm
	$(NASM) -iinclude/ -felf32 -o $@ $<

clean:
	@rm -f *.o *.elf32 *.bin *.map

.PHONY: all clean
