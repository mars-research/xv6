#ifndef XV6_PROC_H_
#define XV6_PROC_H_

#include "mmu.h"
#include "../../spinlock.h"

// Per-CPU state
struct cpu {
  uint64 syscallno;            // Temporary used by sysentry
  uint64 usp;                  // Temporary used by sysentry
  struct proc *proc;           // The process running on this cpu or null
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint64 r15;
  uint64 r14;
  uint64 r13;
  uint64 r12;
  uint64 r11;
  uint64 rbx;
  uint64 rbp;
  uint64 rip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held whenusing these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int pid;                     // Process ID
  int xstate;                  // Exit status to be returned to parent's wait

  // these are private to the process, so lock need not be held
  uint64 kstack;               // Bottom of kernel stack for this process, must be first entry
  uint64 sz;                   // Size of process memory (bytes)
  pml4e_t* pml4;               // Page table
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
