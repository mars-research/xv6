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

To review
---------

-- switchkvm()/kinithart() has been renamed to loadkpml4(). The function
   (re)loads CR3 with the kernel's PML4.
