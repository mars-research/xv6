K=kernel
U=user
ULINK=0x100000 # 1 MB
ARCH=x86_64

OBJS = \
  $K/printf.o \
  $K/string.o \
  $K/console.o \
  $K/main.o \
  $K/kalloc.o \
  $K/bio.o \
  $K/log.o \
  $K/file.o \
  $K/fs.o \
  $K/pipe.o \
  $K/sysfile.o \
  $K/sysproc.o \
  $K/arch/$(ARCH)/entry.o \
  $K/arch/$(ARCH)/lapic.o \
  $K/arch/$(ARCH)/uart.o \
  $K/arch/$(ARCH)/spinlock.o \
  $K/arch/$(ARCH)/proc.o \
  $K/arch/$(ARCH)/vm.o \
  $K/arch/$(ARCH)/trampoline.o \
  $K/arch/$(ARCH)/trap.o \
  $K/arch/$(ARCH)/syscall.o \
  $K/arch/$(ARCH)/swtch.o \
  $K/arch/$(ARCH)/sleeplock.o \
  $K/arch/$(ARCH)/mp.o \
  $K/arch/$(ARCH)/ioapic.o \
  $K/arch/$(ARCH)/picirq.o \
  $K/arch/$(ARCH)/ide.o \
  $K/arch/$(ARCH)/exec.o \
  $K/arch/$(ARCH)/kbd.o \

ifeq ($(ARCH),x86_64)
# $(info [INFO]: x86_64; including vectors.o to OBJS)
  OBJS += \
    $K/arch/$(ARCH)/vectors.o
endif

QEMU = qemu-system-x86_64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -fno-omit-frame-pointer -ggdb
# CFLAGS += -O
CFLAGS += -D$(ARCH)
CFLAGS += -MD
CFLAGS += -mcmodel=large
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

$K/kernel: $(OBJS) $K/kernel.ld $K/arch/$(ARCH)/entryother# $U/initcode
	$(LD) $(LDFLAGS) -T $K/arch/$(ARCH)/kernel.ld -o $K/kernel $(OBJS) -b binary entryother
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$K/arch/$(ARCH)/entryother: $K/arch/$(ARCH)/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c $K/arch/$(ARCH)/entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
	$(OBJDUMP) -S bootblockother.o > entryother.asm

 
$K/arch/$(ARCH)/vectors.S: $K/arch/$(ARCH)/vectors.pl
	./$K/arch/$(ARCH)/vectors.pl > $K/arch/$(ARCH)/vectors.S

initcode: $U/arch/$(ARCH)/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -Ikernel -c $U/arch/$(ARCH)/initcode.S -o $U/arch/$(ARCH)/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext $(ULINK) -o $U/arch/$(ARCH)/initcode.out $U/arch/$(ARCH)/initcode.o
	$(OBJCOPY) -S -O binary $U/arch/$(ARCH)/initcode.out initcode
	$(OBJDUMP) -S $U/arch/$(ARCH)/initcode.o > $U/arch/$(ARCH)/initcode.asm


tags: $(OBJS) _init
	etags *.S *.c

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext $(ULINK) -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext $(ULINK) -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

mkfs/mkfs: mkfs/mkfs.c $K/fs.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_wc\
	$U/_zombie\
	#$U/_usertests\
	$U/_wc\
	$U/_zombie\

fs.img: mkfs/mkfs README $(UPROGS)
	mkfs/mkfs fs.img README $(UPROGS)

-include kernel/*.d user/*.d

$K/bootblock: $K/arch/$(ARCH)/bootasm.S $K/arch/$(ARCH)/bootmain.c
	$(CC) -m32 -c $K/arch/$(ARCH)/bootasm.S -o $K/arch/$(ARCH)/bootasm.o
	$(CC) -fno-builtin -fno-pic -m32 -c -O $K/arch/$(ARCH)/bootmain.c -o $K/arch/$(ARCH)/bootmain.o
	$(LD) $(LDFLAGS) -m elf_i386 -N -estart -Ttext 0x7C00 -o $K/bootblock.o $K/arch/$(ARCH)/bootasm.o $K/arch/$(ARCH)/bootmain.o
	$(OBJDUMP) -S $K/bootblock.o > $K/bootblock.asm
	$(OBJCOPY) -j .text -O binary $K/bootblock.o $K/bootblock
	$K/arch/$(ARCH)/sign.pl $K/bootblock

xv6.img: $K/kernel $K/bootblock
	dd if=/dev/zero of=xv6.img count=10000
	dd if=$K/bootblock of=xv6.img conv=notrunc
	dd if=$< of=xv6.img seek=1 conv=notrunc

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/arch/$(ARCH)/initcode $U/arch/$(ARCH)/initcode.out $K/kernel fs.img \
	$U/arch/$(ARCH)/initcode.asm \
	mkfs/mkfs \
        $U/usys.S \
	$(UPROGS) \
	xv6.img serial.log \
	*/*/*/*.o */*/*/*.d \
	*/*/*/vectors.S \
	$K/bootblock

# try to generate a unique GDB port
# GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
# QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
#	then echo "-gdb tcp::$(GDBPORT)"; \
#	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 2
endif

QEMUGDB = -S -s
QEMUEXTRA = -drive file=fs1.img,if=none,format=raw,id=x1 -device virtio-blk-device,drive=x1,bus=virtio-mmio-bus.1
QEMUOPTS = -drive file=xv6.img,format=raw -m 3G -smp $(CPUS) -nographic
QEMUOPTS += -drive file=fs.img,index=1,media=disk,format=raw
QEMUOPTS += -no-shutdown -no-reboot
# QEMUOPTS += -d int
# QEMUOPTS += -serial file:serial.log
# QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: xv6.img fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: xv6.img .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

# CUT HERE
# prepare dist for students
# after running make dist, probably want to
# rename it to rev0 or rev1 or so on and then
# check in that version.

EXTRA=\
	mkfs.c ulib.c user.h cat.c echo.c forktest.c grep.c kill.c\
	ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c\
	printf.c umalloc.c\
	README dot-bochsrc *.pl \
	.gdbinit.tmpl gdbutil\

dist:
	rm -rf dist
	mkdir dist
	for i in $(FILES); \
	do \
		grep -v PAGEBREAK $$i >dist/$$i; \
	done
	sed '/CUT HERE/,$$d' Makefile >dist/Makefile
	echo >dist/runoff.spec
	cp $(EXTRA) dist

dist-test:
	rm -rf dist
	make dist
	rm -rf dist-test
	mkdir dist-test
	cp dist/* dist-test
	cd dist-test; $(MAKE) print
	cd dist-test; $(MAKE) bochs || true
	cd dist-test; $(MAKE) qemu

# update this rule (change rev#) when it is time to
# make a new revision.
tar:
	rm -rf /tmp/xv6
	mkdir -p /tmp/xv6
	cp dist/* dist/.gdbinit.tmpl /tmp/xv6
	(cd /tmp; tar cf - xv6) | gzip >xv6-rev10.tar.gz  # the next one will be 10 (9/17)

.PHONY: dist-test dist
