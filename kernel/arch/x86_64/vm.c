#include "../../types.h"
#include "../../memlayout.h"
#include "../../defs.h"
#include "x86_64.h"
#include "mmu.h"

/*
 * the kernel's page table.
 */
pml4e_t *kernel_pml4;

extern char etext[];  // kernel.ld sets this to end of kernel code.

// Return the address of the PTE in page table pagetable
// that corresponds to virtual address va.  If alloc!=0,
// create any required page-table pages.
//
// 4-level paging, as the name suggests, uses 4 levels of
// page tables (or structures). Each structure contains
// 512 64-bit entries.
// A 64-bit virtual or linear address is split as:
// 63:48 -- same as bit 47
// 47:39 -- 9-bit index into PML4
// 38:30 -- 9-bit index into PDPT
// 29:21 -- 9-bit index into PD
// 20:12 -- 9-bit index into PT
// 11:0  -- 12-bit offset within a 4K page
pte_t *
walk(pml4e_t *pml4, uint64 va, int alloc)
{
  if (va >= MAXVA)
    panic("walk");

  uint level;
  pse_t *pgstr; // paging structure
  pse_t *pse;

  // find level 1 paging structure (Page Table)
  for (level = 4, pgstr = (pse_t*)pml4; level > 1; --level) {
    pse = &pgstr[PX(level, va)];
    if (*pse & PSE_P) {
      pgstr = (pse_t*)PSE2PA(*pse);
    } else {
      if (!alloc || (pgstr = (pse_t*)kalloc()) == 0)
        return 0;
      memset(pgstr, 0, PGSIZE);
      *pse = PA2PSE(pgstr) | PSE_P;
    }
  }

  return (pte_t*) &pgstr[PX(level, va)]; // level == 1
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size need not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pml4e_t *pml4, uint64 va, uint64 size, uint64 pa, uint64 perm)
{
  uint64 a, last;
  pte_t *pte;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for (;;) {
    if ((pte = walk(pml4, a, 1)) == 0)
      return -1;
    if (*pte & PSE_P)
      panic("mmap");
    *pte = PA2PSE(pa) | perm | PSE_P;
    if (a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// add a mapping to the kernel pml4.
// does not flush TLB or enable paging.
void
kvmmap(uint64 va, uint64 pa, uint64 sz, uint64 perm)
{
  if(mappages(kernel_pml4, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

void
vmmap(pml4e_t *pml4, uint64 va, uint64 pa, uint64 sz, uint64 perm)
{
  if (mappages(pml4, va, sz, pa, perm) != 0)
    panic("vmmap");
}

/*
 * create a direct-map page table with kernel addresses
 * mapped. Used to setup up kernel page table and kernel
 * part of process page table.
 * the page allocator is already initialized.
 */
pml4e_t *
kvminit()
{
  pml4e_t *pml4 = (pml4e_t*) kalloc();
  memset(pml4, 0, PGSIZE);

  // TODO: describe first two regions better
  // IO space
  vmmap(pml4, 0, 0, EXTMEM, PSE_W|PSE_XD);

  // more devices
  vmmap(pml4, DEVSPACE, DEVSPACE, DEVSPACETOP-DEVSPACE, PSE_W|PSE_XD);

  // map kernel text executable and read-only.
  vmmap(pml4, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, 0);

  // map kernel data and the physical RAM we'll make use of.
  vmmap(pml4, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PSE_W|PSE_XD);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  // mappages(pml4, TRAMPOLINE, (uint64)trampoline, PGSIZE, PSE_R | PSE_X);

  return pml4;
}

// load kernel_pml4 into the hardware page table register
void
loadkpml4()
{
  lcr3((uint64) kernel_pml4);
}

// Sets up a kernel page table and switches to it.
// Called during boot up.
void
kpaginginit()
{
  kernel_pml4 = kvminit();
  loadkpml4();
}
