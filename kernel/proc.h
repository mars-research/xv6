#ifndef XV6_PROC_H_
#define XV6_PROC_H_

#include "types.h"
#include "param.h"
#include "spinlock.h"


enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  // kstack needs to be the first entry; see trampoline.S:sysentry
  uint64 kstack;               // Bottom of kernel stack for this process, must be first entry
  struct spinlock lock;

  // p->lock must be held whenusing these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int pid;                     // Process ID
  int xstate;                  // Exit status to be returned to parent's wait

  // these are private to the process, so lock need not be held
  // uint64 kstack;            // Bottom of kernel stack for this process, must be first entry
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t *pagetable;      // Page table
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif // XV6_PROC_H_
