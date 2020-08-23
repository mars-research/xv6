#include "../../types.h"
#include "../../memlayout.h"
#include "../../defs.h"
#include "x86_64.h"
#include "mmu.h"
#include "msr.h"

pml4e_t *kernel_pml4; // kernel pml4, used by scheduler and bootstrap

extern char etext[];  // kernel.ld sets this to end of kernel code.
extern uint64 trampoline[]; // see trampoline.S
extern uint64 sysentry[];   // see trampoline.S

// Bootstrap GDT. Implements a flat segmentation model where logical
// addresses are identity mapped to linear addresses. The descriptors
// correspond to the indexes as follows:
// 0 - Null descriptor. Required.
// 1 - 32-bit kernel code segment
// 2 - 64-bit kernel code segment
// 3 - data segment
// 6 - user data segment
// 7 - user code segment
struct segdesc bootgdt[NSEGS] = {
  [0] = SEGDESC(0, 0, 0), // Null
  [1] = SEGDESC(0, 0xFFFFF, SEG_R|SEG_CODE|SEG_S|SEG_DPL(0)|SEG_P|SEG_D|SEG_G),
  [2] = SEGDESC(0, 0, SEG_R|SEG_CODE|SEG_S|SEG_DPL(0)|SEG_P|SEG_L|SEG_D|SEG_G),
  [3] = SEGDESC(0, 0xFFFFF, SEG_W|SEG_S|SEG_DPL(0)|SEG_P|SEG_D|SEG_G),
  // The order of the user data and user code segments is
  // important for syscall instructions.  See initseg.
  [6] = SEGDESC(0, 0, SEG_W|SEG_S|SEG_DPL(DPL_USER)|SEG_P|SEG_D|SEG_G),
  [7] = SEGDESC(0, 0xFFFFF, SEG_R|SEG_S|SEG_DPL(DPL_USER)|SEG_P|SEG_L|SEG_D|SEG_G),
};

// Return the address of the PTE in page table pml4 
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

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pml4e_t *pml4, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return 0;

  pte = walk(pml4, va, 0);
  if(pte == 0)
    return 0;
  if((*pte & PSE_P) == 0)
    return 0;
  if((*pte & PSE_U) == 0)
    return 0;
  pa = PSE2PA(*pte);
  return pa;
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
kvmcreate()
{
  pml4e_t *pml4 = (pml4e_t*) kalloc();
  memset(pml4, 0, PGSIZE);

  // TODO: describe first two regions better
  // IO space
  vmmap(pml4, 0, 0, EXTMEM, PSE_W|PSE_XD);

  // more devices
  vmmap(pml4, DEVSPACE, DEVSPACE, DEVSPACETOP-DEVSPACE, PSE_W|PSE_XD);

  // map kernel text executable and read-only.
  vmmap(pml4, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PSE_R|PSE_X);

  // map kernel data and the physical RAM we'll make use of.
  vmmap(pml4, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PSE_W|PSE_XD);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  vmmap(pml4, TRAMPOLINE, (uint64)trampoline, PGSIZE, PSE_R|PSE_X);

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
  kernel_pml4 = kvmcreate();
  loadkpml4();
}

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;
  struct desctr dtr;

  c = mycpu();
  memmove(c->gdt, bootgdt, sizeof bootgdt);
  dtr.limit = sizeof(c->gdt)-1;
  dtr.base = (uint64) c->gdt;
  lgdt((void *)&dtr.limit);

  // When executing a syscall instruction the CPU sets the SS selector
  // to (star >> 32) + 8 and the CS selector to (star >> 32).
  // When executing a sysret instruction the CPU sets the SS selector
  // to (star >> 48) + 8 and the CS selector to (star >> 48) + 16.
  uint64 star = ((((uint64)UCSEG|0x3)- 16)<<48)|((uint64)(KCSEG)<<32);
  writemsr(MSR_STAR, star);
  writemsr(MSR_LSTAR, (uint64)&sysentry);
  writemsr(MSR_SFMASK, FL_TF | FL_IF);

  // Initialize cpu-local storage.
  writegs(KDSEG);
  writemsr(MSR_GS_BASE, (uint64)c);
  writemsr(MSR_GS_KERNBASE, (uint64)c);
}

// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pml4e_t *pml4, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while (len > 0) {
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pml4, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int
copyin(pml4e_t *pml4, char *dst, uint64 srcva, uint64 len)
{
  uint64 n, va0, pa0;

  while (len > 0) {
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pml4, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > len)
      n = len;
    memmove(dst, (void *)(pa0 + (srcva - va0)), n);

    len -= n;
    dst += n;
    srcva = va0 + PGSIZE;
  }
  return 0;
}

// Copy a null-terminated string from user to kernel.
// Copy bytes to dst from virtual address srcva in a given page table,
// until a '\0', or max.
// Return 0 on success, -1 on error.
int
copyinstr(pml4e_t *pml4, char *dst, uint64 srcva, uint64 max)
{
  uint64 n, va0, pa0;
  int got_null = 0;

  while(got_null == 0 && max > 0){
    va0 = PGROUNDDOWN(srcva);
    pa0 = walkaddr(pml4, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (srcva - va0);
    if(n > max)
      n = max;

    char *p = (char *) (pa0 + (srcva - va0));
    while(n > 0){
      if(*p == '\0'){
        *dst = '\0';
        got_null = 1;
        break;
      } else {
        *dst = *p;
      }
      --n;
      --max;
      p++;
      dst++;
    }

    srcva = va0 + PGSIZE;
  }
  if(got_null){
    return 0;
  } else {
    return -1;
  }
}

// Remove mappings from a page table. The mappings in
// the given range must exist. Optionally free the
// physical memory.
void
uvmunmap(pml4e_t *pml4, uint64 va, uint64 size, int do_free)
{
  uint64 a, last;
  pte_t *pte;
  uint64 pa;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);
  for(;;){
    if((pte = walk(pml4, a, 0)) == 0)
      panic("uvmunmap: walk");
    if((*pte & PSE_P) == 0){
      printf("va=%p pte=%p\n", a, *pte);
      panic("uvmunmap: not mapped");
    }
    if(do_free){
      pa = PSE2PA(*pte);
      kfree((void*)pa);
    }
    *pte = 0;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pml4e_t *pml4, uint64 oldsz, uint64 newsz)
{
  if(newsz >= oldsz)
    return oldsz;

  uint64 newup = PGROUNDUP(newsz);
  if(newup < PGROUNDUP(oldsz))
    uvmunmap(pml4, newup, oldsz - newup, 1);

  return newsz;
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pml4e_t *pml4, uint64 oldsz, uint64 newsz)
{
  char *mem;
  uint64 a;

  if(newsz < oldsz)
    return oldsz;

  oldsz = PGROUNDUP(oldsz);
  a = oldsz;
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      uvmdealloc(pml4, a, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pml4, a, PGSIZE, (uint64)mem, PSE_W|PSE_U) != 0){
      kfree(mem);
      uvmdealloc(pml4, a, oldsz);
      return 0;
    }
  }
  return newsz;
}

// walk table at given level, and recursively free
// page-table pages.
static void
freewalk(pte_t *table, int level) {
  if (level < 1 || level > 4)
    panic("freewalktable");

  if (level > 1) { // PML4, PDPT, or PDT
    // every table has 512 entries
    for (int i = 0; i < 512; i++) {
      pte_t pte = table[i];
      if (pte & PSE_P)
        freewalk((pte_t*)PSE2PA(pte), level-1);
    }
  }

  kfree((void*)table);
}

// Free user memory pages,
// then free page-table pages.
void
vmfree(pml4e_t *pml4, uint64 sz)
{
  uvmunmap(pml4, 0, sz, 1);
  freewalk(pml4, /*level*/ 4);
}
