#include "types.h"
#include "defs.h"
#include "kernel/arch/x86_64/x86_64.h"
volatile static int started = 0;

static void startothers(void);
// start() jumps here in supervisor mode on all CPUs.
// TODO: remove all references to x86-specific functions here
void
main()
{
  kinit();
  kpaginginit();
  procinit();
  trapinit();
  seginit();
  mpinit();
  lapicinit();
  picinit();
  ioapicinit();
  consoleinit();
  printfinit();
  printf("\n");
  printf("xv6 is booting\n");
  printf("\n");
  binit();
  iinit();
  fileinit();
  diskinit();
  startothers();
  userinit();
  printf("done\n");
  scheduler();
}
// Other CPUs jump here from entryother.S.
void
apmain(void)
{
  printf("cpu%d: starting %d\n", cpuid(), cpuid());
  seginit();
  lapicinit();
  printf("cpu%d: starting %d\n", cpuid(), cpuid());
  // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  scheduler();     // start running processes
}

void apstart(void);

static void startothers(void)
{
  struct cpu *c;
  char *stack;
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  memmove((void*)0x7000, _binary_entryother_start, (uint64)_binary_entryother_size);

  for(c = cpus; c < cpus+ncpu; c++){
    printf("cpu%d: searching %d\n", cpuid(), c->apicid);
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    printf("entryother addr 0x%p", _binary_entryother_start);
    stack = kalloc();
    *(uint32*)(0x7000-4) = (uint32)((uint64)apstart & 0xffffffff);
    *(uint64*)(0x7000-12) = (uint64) (stack);
    lapicstartap(c->apicid, (uint64)0x7000);

    // wait for cpu to finish mpmain()
    while(c->started == 0){
      //printf("cpu%d: waiting %d\n", cpuid(), c->apicid);
      ;
    }
  }
}
