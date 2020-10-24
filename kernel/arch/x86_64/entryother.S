#include "asm.h"
#include "mmu.h"
#include "../../memlayout.h"

.code16           
.globl entryother
entryother:
    cli                         # BIOS enabled interrupts; disable

# Zero data segment registers DS, ES, and SS.
    xorw    %ax,%ax             # Set %ax to zero
    movw    %ax,%ds             # -> Data Segment
    movw    %ax,%es             # -> Extra Segment
    movw    %ax,%ss             # -> Stack Segment

# Switch from real to protected mode.  Use a bootstrap GDT that makes
# virtual addresses map directly to physical addresses so that the
# effective memory map doesn't change during the transition.
    lgdt    gdtdesc
    movl    %cr0, %eax
    orl     $CR0_PE, %eax
    movl    %eax, %cr0


# Complete transition to 32-bit protected mode by using long jmp
# to reload %cs and %eip.  The segment descriptors are set up with no
# translation, so that the mapping is still the identity mapping.
    ljmp    $(SEG_KCODE<<3), $entryother32

.code32  # Tell assembler to generate 32-bit code now.
entryother32:
# Set up the protected-mode data segment registers
    movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
    movw    %ax, %ds                # -> DS: Data Segment
    movw    %ax, %es                # -> ES: Extra Segment
    movw    %ax, %ss                # -> SS: Stack Segment
    movw    $0, %ax                 # Zero segments not ready for use
    movw    %ax, %fs                # -> FS
    movw    %ax, %gs                # -> GS

    movl $0x7000-4, %ecx
    jmp *%ecx
    
    jmp   .


.p2align 2                                # force 4 byte alignment
gdt:
    SEG_NULLASM                             # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
    SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
    
gdtdesc:
    .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
    .long   gdt                             # address gdt