# Temporary Makefile to be used for initial testing

KERNEL = kernel
IMG = xv6.img
OBJS = \
       setup.o \

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -fno-omit-frame-pointer -ggdb -c
CFLAGS += -O3
# CFLAGS += -MD
# CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

QEMU = qemu-system-x86_64
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 1
endif

all: $(IMG)

QEMUOPTS = -drive file=xv6.img,format=raw -m 512
QEMUOPTS += -curses
QEMUGDB = -S -s

qemu: $(IMG)
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(IMG)
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) $(QEMUGDB)

bootblock: bootasm.S bootmain.c
	$(CC) -m32 -c bootasm.S -o bootasm.o
	$(CC) -fno-builtin -fno-pic -m32 -c -O bootmain.c -o bootmain.o
	$(LD) $(LDFLAGS) -m elf_i386 -N -estart -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -j .text -O binary bootblock.o bootblock
	./sign.pl bootblock

$(KERNEL): entry.S kernel.ld $(OBJS)
	$(CC) -c -o entry.o $<
	$(LD) $(LDFLAGS) -m elf_x86_64 -N -T kernel.ld -o $@ entry.o $(OBJS)

$(IMG): $(KERNEL) bootblock $(OBJS)
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=$(KERNEL) of=xv6.img seek=1 conv=notrunc

clean:
	rm $(KERNEL) *.o *.d bootblock || true

.PHONY: clean, all
