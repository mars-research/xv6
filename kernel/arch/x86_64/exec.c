#include "x86_64.h"
#include "../../types.h"
#include "../../param.h"
#include "../../memlayout.h"
#include "../../spinlock.h"
#include "../../proc.h"
#include "../../defs.h"
#include "../../elf.h"

pte_t* walk(pml4e_t *pagetable, uint64 va, int alloc); 
static int loadseg(pagetablee_t *pagetable, uint64 va, struct inode *ip, uint offset, uint sz);

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint64 argc, sz, oldsz, sp, ustack[MAXARG+1+1], stackbase, kstackpa;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetablee_t *pagetable = 0, *oldpagetable;
  struct proc *p;

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  p = myproc();
  if((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if(ph.vaddr < SZ2UVA(0))
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if((sz = uvmalloc(pagetable, sz, UVA2SZ(ph.vaddr) + ph.memsz)) == 0)
      goto bad;
    if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  uvmclear(pagetable, sz-2*PGSIZE);
  sp = SZ2UVA(sz);
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // data should be 16-byte aligned for fast access
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[1+argc] = sp;
  }
  ustack[1+argc] = 0; // null terminator for argv
  ustack[0] = (0L-1); // fake return address for user main

  // push the array of argv[] pointers.
  sp -= (argc+1+1) * sizeof(uint64);
  // we want ustack[1] (argv) to be 16-byte aligned. Therefore,
  // sp needs to be 8 bytes off
  if ((sp % 16) == 0) // 16-byte aligned?
    sp -= 8;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1+1)*sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  p->tf->rdi = argc;
  p->tf->rsi = sp + 8; // argv

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
    
  // map kernel stack in the new pagetable
  kstackpa = PSE2PA(*walk(p->pagetable, p->kstack, 0));
  vmmap(pagetable, p->kstack, kstackpa, PGSIZE, PSE_R|PSE_W);

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->tf->rip = elf.entry;  // initial program counter = main
  p->tf->rcx = elf.entry;  // sysexit loads rip from rcx
  p->tf->rsp = sp;         // initial stack pointer
  switchuvm(p);

  proc_freepagetable(oldpagetable, oldsz);
  return argc;

 bad:
  if(pagetable)
    proc_freepagetable(pagetable, sz);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetablee_t *pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  if((va % PGSIZE) != 0)
    panic("loadseg: va must be page aligned");

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}
