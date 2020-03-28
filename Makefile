NASM ?= nasm
OBJCOPY ?= objcopy
OBJS = entry16.o start32.o
CFLAGS = -Wall -std=c99 -ffreestanding -m32 -march=i386 -fno-stack-protector -fno-pic

all: bios.bin

bios.bin: bootleg.elf32
	$(OBJCOPY) -O binary $< $@

bootleg.elf32: $(OBJS) image.lds
	$(LD) -T image.lds -Map=$@.map --gc-sections -nostdlib -o $@ $(OBJS)

%.o: %.asm
	$(NASM) -iinclude/ -felf32 -o $@ $<

clean:
	@rm -f *.o *.elf32 *.bin *.map

.PHONY: all clean
