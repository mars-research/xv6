#ifndef XV6_MMU_H_
#define XV6_MMU_H_

#include "../../memlayout.h"

// This file contains definitions for the 
// x86 memory management unit (MMU).

// Eflags register
#define FL_CF           0x00000001      // Carry Flag
#define FL_PF           0x00000004      // Parity Flag
#define FL_AF           0x00000010      // Auxiliary carry Flag
#define FL_ZF           0x00000040      // Zero Flag
#define FL_SF           0x00000080      // Sign Flag
#define FL_TF           0x00000100      // Trap Flag
#define FL_IF           0x00000200      // Interrupt Enable
#define FL_DF           0x00000400      // Direction Flag
#define FL_OF           0x00000800      // Overflow Flag
#define FL_IOPL_MASK    0x00003000      // I/O Privilege Level bitmask
#define FL_IOPL_0       0x00000000      //   IOPL == 0
#define FL_IOPL_1       0x00001000      //   IOPL == 1
#define FL_IOPL_2       0x00002000      //   IOPL == 2
#define FL_IOPL_3       0x00003000      //   IOPL == 3
#define FL_NT           0x00004000      // Nested Task
#define FL_RF           0x00010000      // Resume Flag
#define FL_VM           0x00020000      // Virtual 8086 mode
#define FL_AC           0x00040000      // Alignment Check
#define FL_VIF          0x00080000      // Virtual Interrupt Flag
#define FL_VIP          0x00100000      // Virtual Interrupt Pending
#define FL_ID           0x00200000      // ID flag

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_MP          0x00000002      // Monitor coProcessor
#define CR0_EM          0x00000004      // Emulation
#define CR0_TS          0x00000008      // Task Switched
#define CR0_ET          0x00000010      // Extension Type
#define CR0_NE          0x00000020      // Numeric Errror
#define CR0_WP          0x00010000      // Write Protect
#define CR0_AM          0x00040000      // Alignment Mask
#define CR0_NW          0x20000000      // Not Writethrough
#define CR0_CD          0x40000000      // Cache Disable
#define CR0_PG          0x80000000      // Paging

#define CR4_PSE         (1<<4)    // Page size extension
#define CR4_PAE         (1<<5)    // Physical addr. extension

// Register addr of IA32_EFER
#define EFER        0xC0000080      // IA32 MSR
// Flags in IA32_EFER
#define EFER_SCE    (1<<0)          // IA32_EFER.SCE bit
#define EFER_LME    (1<<8)          // IA32_EFER.LME bit
#define EFER_NXE    (1<<11)         // IA32_EFER.NXE bit

// Paging structure entry flags
#define PSE_P       (1<<0)          // P;   1: present
#define PSE_W       (1<<1)          // RW;  1: write enabled region
#define PSE_R       (0<<1)          // RW;  0: read only region
#define PSE_U       (1<<2)          // U/S; 1: user access allowed
#define PSE_PS      (1<<7)          // PS (page size)
/* Disables instruction fetches from memory region mapped by this
 * entry iff IA32_EFER.NXE == 1. MUST be 0 otherwise. */
#define PSE_XD      (1L<<63)         // XD;  1: execution disabled
#define PSE_X       (0L<<63)         // XD;  0: execution not disabled

#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel data+stack
#define SEG_KCPU  3  // kernel per-cpu data
#define SEG_UCODE 4  // user code
#define SEG_UDATA 5  // user data+stack
#define SEG_TSS   6  // this process's task state

// Segment selectors (indexes) in our GDTs.
// Defined by our convention, not the architecture.
#define KCSEG32 (1<<3)  /* kernel 32-bit code segment */
#define KCSEG   (2<<3)  /* kernel code segment */
#define KDSEG   (3<<3)  /* kernel data segment */
#define TSSSEG  (4<<3)  /* tss segment - takes two slots */
#define UDSEG   (6<<3)  /* user data segment */
#define UCSEG   (7<<3)  /* user code segment */

// Number of segment descriptors in out bootstrap GDT
#define NSEGS 8

// Page table/directory entry flags.
#define PSE_P           (1<<0)   // Present
#define PSE_W           (1<<1)   // Writeable
#define PSE_U           (1<<2)   // User
#define PSE_PWT         (1<<3)   // Write-Through
#define PSE_PCD         (1<<4)   // Cache-Disable
#define PSE_A           (1<<5)   // Accessed
#define PSE_D           (1<<6)   // Dirty
#define PSE_PS          (1<<7)   // Page Size
#define PSE_MBZ         0x180   // Bits must be zero

// Page directory and page table constants.
#define NPDENTRIES      512     // # directory entries per page directory
#define NPTENTRIES      512     // # PTEs per page table

#define PGSHIFT         12      // log2(PGSIZE)
#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        21      // offset of PDX in a linear address
#define PX(level, va)   (((uint64)(va)>>(9*(level-1) + 12)) & PXMASK)

#define PXMASK          0x1FF

//PAGEBREAK!
#ifndef __ASSEMBLER__
#include "../../types.h"

// Segment Descriptor
struct segdesc {
	uint16 limit0;
	uint16 base0;
	uint8 base1;
	uint8 bits;
	uint8 bitslimit1;
	uint8 base2;
};

// SEGDESC constructs a segment descriptor literal
// with the given, base, limit, and type bits.
#define SEGDESC(base, limit, bits) (struct segdesc){ \
	(limit)&0xffff, (base)&0xffff, \
	((base)>>16)&0xff, \
	(bits)&0xff, \
	(((bits)>>4)&0xf0) | ((limit>>16)&0xf), \
	((base)>>24)&0xff, \
}
#define SEG16(type, base, lim, dpl) (struct segdesc)  \
{ (lim) & 0xffff, (uintp)(base) & 0xffff,              \
  ((uintp)(base) >> 16) & 0xff, type, 1, dpl, 1,       \
  (uintp)(lim) >> 16, 0, 0, 1, 0, (uintp)(base) >> 24 }

#define DPL_USER    0x3     // User DPL

#define SEG_A      (1<<0)      /* segment accessed bit */
#define SEG_R      (1<<1)      /* readable (code) */
#define SEG_W      (1<<1)      /* writable (data) */
#define SEG_C      (1<<2)      /* conforming segment (code) */
#define SEG_E      (1<<2)      /* expand-down bit (data) */
#define SEG_CODE   (1<<3)      /* code segment (instead of data) */

// User and system segment bits.
#define SEG_S      (1<<4)      /* if 0, system descriptor */
#define SEG_DPL(x) ((x)<<5)    /* descriptor privilege level (2 bits) */
#define SEG_P      (1<<7)      /* segment present */
#define SEG_AVL    (1<<8)      /* available for operating system use */
#define SEG_L      (1<<9)      /* long mode */
#define SEG_D      (1<<10)     /* default operation size 32-bit */
#define SEG_G      (1<<11)     /* granularity */

// IO-mapped device addresses
#define EXTMEM  0x100000            // Start of extended memory
#define DEVSPACE 0xFE000000         // Other devices are top of 32-bit address space
#define DEVSPACETOP 0x100000000

// map the trampoline page to the highest address,
// in both user and kernel space.
#define MAXVA (1L<<(9 + 9 + 9 + 9 + 12))
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)


// TODO: update this to 4-level paging scheme
// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/ 

// page directory index
#define PDX(va)         (((uintp)(va) >> PDXSHIFT) & PXMASK)

// page table index
#define PTX(va)         (((uintp)(va) >> PTXSHIFT) & PXMASK)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uintp)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintp)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uintp)(pte) &  0xFFF)

// Inter-convert paging structure entry and physical addr.
#define PA2PSE(pa)  ((uint64)(pa) & 0x000FFFFFFFFFF000)
#define PSE2PA(pse) ((((int64)(pse)<<16)>>16) & ~0xFFF) // zero out top 16 bits

typedef uint64 pml4e_t;
typedef uint64 pdpte_t;
typedef uint64 pde_t;
typedef uint64 pte_t;
typedef uint64 pse_t;

// Task state segment format
struct taskstate {
  uint link;         // Old ts selector
  uint esp0;         // Stack pointers and segment selectors
  ushort ss0;        //   after an increase in privilege level
  ushort padding1;
  uint *esp1;
  ushort ss1;
  ushort padding2;
  uint *esp2;
  ushort ss2;
  ushort padding3;
  void *cr3;         // Page directory base
  uint *eip;         // Saved state from last task switch
  uint eflags;
  uint eax;          // More saved state (registers)
  uint ecx;
  uint edx;
  uint ebx;
  uint *esp;
  uint *ebp;
  uint esi;
  uint edi;
  ushort es;         // Even more saved state (segment selectors)
  ushort padding4;
  ushort cs;
  ushort padding5;
  ushort ss;
  ushort padding6;
  ushort ds;
  ushort padding7;
  ushort fs;
  ushort padding8;
  ushort gs;
  ushort padding9;
  ushort ldt;
  ushort padding10;
  ushort t;          // Trap on task switch
  ushort iomb;       // I/O map base address
};

// PAGEBREAK: 12
// Gate descriptors for interrupts and traps
struct gatedesc {
  uint off_15_0 : 16;   // low 16 bits of offset in segment
  uint cs : 16;         // code segment selector
  uint args : 5;        // # args, 0 for interrupt/trap gates
  uint rsv1 : 3;        // reserved(should be zero I guess)
  uint type : 4;        // type(STS_{TG,IG32,TG32})
  uint s : 1;           // must be 0 (system)
  uint dpl : 2;         // descriptor(meaning new) privilege level
  uint p : 1;           // Present
  uint off_31_16 : 16;  // high bits of offset in segment
};

// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)                \
{                                                         \
  (gate).off_15_0 = (uint)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? STS_TG32 : STS_IG32;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint)(off) >> 16;                  \
}

#endif // SETGATE
#endif // XV6_MMU_H_
