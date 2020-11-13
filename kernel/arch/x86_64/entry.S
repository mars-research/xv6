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
# We map the 0 - 0 GiB + 1024 MiB of physical memory at 0 - 0 GiB + 1024 MiB (identity mapping)
# With both PSE_PS and CR4_PAE set to 1, we are able to use 3 level paging with each page being 2MiB.
# With PSE_PS set to 0, the CPU switches back to 4kb pages as default. 

initpagetables:
# We need to setup 3-level page tables
# with 2 MiB pages as follows:
# Base addr:   Table
# PML4
# PDPT
# PD_AP -> 0 - 0 GiB + 1 GiB (512 entry) 

# reset PML4 to 0
  xor   %eax,   %eax
  mov   $PGSIZE, %ecx # number of bytes to zero out
  shr   $2,     %ecx                # 4 bytes written at a time
  mov   $PML4,  %edi
  rep   stosl

# reset PDPT to 0
  xor   %eax,   %eax
  mov   $PGSIZE, %ecx # number of bytes to zero out
  shr   $2,     %ecx                # 4 bytes written at a time
  mov   $PDPT,  %edi
  rep   stosl

# reset PD_AP to 0
  xor   %eax,   %eax
  mov   $PGSIZE, %ecx # number of bytes to zero out
  shr   $2,     %ecx                # 4 bytes written at a time
  mov   $PD_AP,  %edi
  rep   stosl

# Setup PD_AP
  mov   $PD_AP,    %ebx
  xor   %ecx,   %ecx                 # counter
  mov   $(2<<20),   %edx             # 2 MiB; to calculate page base addrs
  mov   $0x0,  %eax
  or    $(PSE_P|PSE_W|PSE_PS),  %eax
PD_AP_loop:
  mov   %eax,   (%ebx, %ecx, 8)      # PD_AP[ECX] = EAX
  add   %edx,   %eax                 # create next PD_AP entry
  inc   %ecx
  cmp   $NPDENTRIES,    %ecx                 # 512 entries to map 1024 MiB
  jnz   PD_AP_loop

# Setup PDPT
  mov   $PDPT,  %ebx

# PDPT[0] -> PD_AP -- 0 GiB - 1 GiB entry
  mov   $PD_AP, %eax
  or    $(PSE_P|PSE_W), %eax
  mov   %eax,   (%ebx)

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

.code32
.globl apstart
apstart:
# set a stack for loadgdt to use
  mov $0x6fe0, %esp
  call  loadgdt

# setup paging
  call  enablepaging

# enable 64-bit execution
  ljmp  $KCSEG, $apstart64

.code64
apstart64:
# load VA of stack
  movq $0x7000-12, %rax
  movq (%rax), %rsp

# clear frame pointer for stack walks
  xor   %rbp, %rbp

# boot into C
  call  apmain

# data section
.section .bss
.align PGSIZE
.lcomm stack, STACKSIZE
stacktop:
