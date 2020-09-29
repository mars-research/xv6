# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html

#include "asm.h"
#include "mmu.h"
#include "../../memlayout.h"

# Multiboot2 header -- must be 64-bit aligned
.set MAGIC, 0xE85250D6
.set ARCH,  0         /* 32-bit protected mode */
.set HEADER_LEN, (header_end - multiboot_header)
.set CHECKSUM, (0 - (MAGIC + ARCH + HEADER_LEN))

.code32
.align 8
.section .multiboot_header
.globl multiboot_header
multiboot_header:
  .long MAGIC
  .long ARCH
  .long HEADER_LEN
  .long CHECKSUM

# Address tag
addr_tag:
  MBHEADER_TAG(2, 0, (entry_addr_tag - addr_tag))
  .long multiboot_load             /* header_addr */
  .long multiboot_load             /* load_addr */
  .long multiboot_load_end         /* load_end_addr */
  .long multiboot_bss_end          /* bss_end_addr */

# Entry address tag
entry_addr_tag:
  MBHEADER_TAG(3, 0, (termination_tag - entry_addr_tag))
  .long multiboot_entry

# Tag to indicate end of tags
termination_tag:
  MBHEADER_TAG(0, 0, (header_end - termination_tag))

header_end:

.section .text
# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = entry

# Entering xv6 on boot processor, with paging off.
#define STACKSIZE 2*PGSIZE
.globl entry
entry:
# setup stack
  mov   $stacktop, %esp

# setup bootstrap GDT
  call  loadgdt

# setup paging
  call  initpagetables
  call  enablepaging

# enable 64-bit execution
  ljmp  $KCSEG, $start64

.code64
start64:
# load VA of stack
  movabsq $(stacktop),  %rsp
# clear frame pointer for stack walks
  xor   %rbp, %rbp
# boot into C
  call  main

# should not return
  jmp   .

# Helper procedures/functions

.code32
# Page tables.  See section 4.5 of 253668.pdf.
# We map the first GB of physical memory at 0 and at 1 TB (not GB) before
# the end of virtual memory.  At boot time we are using the mapping at 0
# but during ordinary execution we use the high mapping.
# The intent is that after bootstrap the kernel can expand this mapping
# to cover all the available physical memory.
# This would be easier if we could use the PS bit to create GB-sized entries
# and skip the pdt table, but not all chips support it, and QEMU doesn't.

initpagetables:
# We need to setup 4-level page tables
# with 2 MiB pages as follows:
# Base addr:   Table
# 0x1000: PML4
# 0x2000: PDPT
# 0x3000: PD   -> 2 - 2 GiB + 128 MiB (64 entries)

#define PML4 0x1000
#define PDPT 0x2000
#define PD   0x3000

# reset 0x1000 - 0x4000 to 0
  xor   %eax,   %eax
  mov   $(PD + PGSIZE - PML4), %ecx # number of bytes to zero out
  shr   $2,     %ecx                # 4 bytes written at a time
  mov   $PML4,  %edi
  rep   stosl

# Setup PD
  mov   $PD,    %ebx
  xor   %ecx,   %ecx                 # counter
  mov   $(2<<20),   %edx             # 2 MiB; to calculate page base addrs
  mov   $KERNBASE,  %eax
  or    $(PSE_P|PSE_W|PSE_PS),  %eax
PDE_loop:
  mov   %eax,   (%ebx, %ecx, 8)      # PD[ECX] = EAX
  add   %edx,   %eax                 # create next PD entry
  inc   %ecx
  cmp   $64,    %ecx                 # 64 entries to map 128 MiB
  jnz   PDE_loop

# Setup PDPT
  mov   $PDPT,  %ebx
  mov   $PD,   %eax
  or    $(PSE_P|PSE_W), %eax
# PDPT[2] -> PD -- 2 GiB - 3 GiB entry
  mov   $2,   %edi
  mov   %eax,   (%ebx, %edi, 8)      # PDPT[0] = ADDR(PD)

# Setup PML4
  mov   $PML4,  %ebx
  mov   $PDPT,  %eax
  or    $(PSE_P|PSE_W), %eax
# PML4[0] -> PDPT
  xor   %edi,   %edi
  mov   %eax,   (%ebx, %edi, 8)      # PML4[0] = ADDR(PDPT)

  ret

enablepaging:
# Load PML4 into CR3
  mov   $PML4,  %eax
  mov   %eax,   %cr3

# Set IA32_EFER.LME, .SCE, and .NXE
  mov   $EFER, %ecx
  rdmsr
  or    $(EFER_LME|EFER_NXE|EFER_SCE),  %eax
  wrmsr

# Set CR4.PAE
  mov   %cr4, %eax
  or    $(CR4_PAE), %eax
  mov   %eax, %cr4

# Set CR0.PG -- Enables 4-level paging
  mov   %cr0, %eax
  or    $(CR0_PG), %eax
  mov   %eax, %cr0

  ret

loadgdt:
# setup gdt descriptor on stack
  sub   $8, %esp
  movl  $(8 * NSEGS - 1), 0(%esp)
  movl  $bootgdt, 2(%esp)
  lgdt  0(%esp)

# reclaim stack space
  add   $8, %esp

# load all segment registers except CS
  mov   $KDSEG, %eax # data segment selector
  mov   %eax,   %ds
  mov   %eax,   %es
  mov   %eax,   %ss
  xor   %eax,   %eax # null segment selector
  mov   %eax,   %fs
  mov   %eax,   %gs

# load new CS register
  ljmp  $KCSEG32, $loadgdt32

loadgdt32:
  ret

# data section
.section .bss
.align PGSIZE
.lcomm stack, STACKSIZE
stacktop:
