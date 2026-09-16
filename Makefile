CC = gcc
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector
AS = nasm
ASFLAGS = -f elf32
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJS = boot.o kernel.o

all: amiluna.bin

boot.o: boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

amiluna.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

amiluna.iso: amiluna.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp amiluna.bin isodir/boot/amiluna.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o amiluna.iso isodir
	rm -rf isodir

clean:
	rm -rf $(OBJS) amiluna.bin amiluna.iso isodir
