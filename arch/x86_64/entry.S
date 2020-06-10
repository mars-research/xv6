# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html

#include "asm.h"

# Multiboot2 header -- must be 64-bit aligned
.code32
.align 8
.set MAGIC, 0xE85250D6
.set ARCH,  0         /* 32-bit protected mode */
.set HEADER_LEN, (header_end - multiboot_header)
.set CHECKSUM, (0 - (MAGIC + ARCH + HEADER_LEN))

.section .multiboot_header
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

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
# .globl _start
# _start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  jmp entry
