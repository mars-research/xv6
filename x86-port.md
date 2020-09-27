MIT's x86_64 bit port 
----------------------

Is in 0f90388c893d1924e89e2e4d2187eda0004e9d73 (maybe not finished)

Layout
------

We add an arch/x86_64 folder for architecture depart

OBJS = \ 
  $K/arch/x86_64/swtch.S \ 
  $K/arch/x86_64/lapic.c \ 



Boot
----

-- Create a boot folder next to kernel and user

-- Copy the boot loader from our x86 port 
   https://github.com/mars-research/xv6-64-bm/tree/dev

-- This way we have a separate boot loader in the boot folder and don't 
   really depend on GRUB. 

-- Maybe we can support GRUB as well


Things of note
--------------

-- Boot loader is currently placed in kernel/arch/x86_64/; assuming it cannot
   be architecture independent.

-- Leave architecture independent definitions in kernel/ and move all
   architecture dependent implementations to kernel/arch/x86_64

-- Instead of using one kernel page table and per-process user-space page tables,
   we map kernel space virtual addresses in every process' page tables.

-- The first instruction executed after `syscall` is `swapgs`. Hence, it is important that `syscall`
   is never executed in ring-0 i.e., nesting syscalls will lead to crashes or undefined behaviour.

To review
---------

-- switchkvm()/kinithart() has been renamed to loadkpml4(). The function
   (re)loads CR3 with the kernel's PML4.

-- walkaddr(...) takes a userspace virtual address and retrieves the physical address
   by walking the pml4 table. It only accepts userspace virtual addresses. Will walkuaddr(...)
   be a better name for the function?

-- in argraw(), the fourth arg is retrieved from RCX; however, Kashooek's implementation did so
   from R10. The internet says RCX is right register. Why was R10 used?

-- the new strategy for context switch in swtch.S might be simpler and easier to understand.
   `struct cpu` has a `struct context` member as opposed to a pointer. `swtch` simply saves
   registers into this `struct` instead of pushing registers in reverse order and storing
   the stack pointer as `struct context*`
   Should we adopt this method as well? Do we not do this because we have to save the
   instruction pointer and the only (easy) way to do it is as a return address?

-- `PTE_R`, `PTE_W`, etc. have been renamed to `PSE_R`, `PSE_W`, etc. It
   stands for Paging Structure Entry. Should we retain this?

To do
-----

-- kernel stack setup for new processes is VERY messy
-- get rid of `kernel_pml4`. It is irrelevant with ever process owning
   its own pagetable/pml4.
-- consider renaming `pml4e_t` to something generic like `pte_t` OR replace 
   all `pml4e_t *`s with `pagetable_t`
-- using PSE and pse (paging structure entry) instead of PTE and pte was a
   bad idea. Change it back.
-- `kvminit` must be renamed. It creates and returns a page table with kernel
   memory mapped. `kvmcreate` is likely a better suited name.
-- vm.c:`mappages` can and probably should be static
