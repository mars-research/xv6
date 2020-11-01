#include "../../types.h"
#include "../../defs.h"
#include "defs.h"
#include "x86_64.h"

volatile static int started = 0;

static void startothers(void);
// start() jumps here in supervisor mode on all CPUs.
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
// Other CPUs jump here from apstart.
void
apmain(void)
{
  switchkvm();
  seginit();
  lapicinit();
  idtinit();
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  printf("AP%d: starting %d\n", cpuid(), cpuid());
  scheduler();     // start running processes
}

void apstart(void);
static void startothers(void)
{
  struct cpu *c;
  char *stack;
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  memmove((void*)0x7000, _binary_entryother_start, (uint64)_binary_entryother_size); //load entryother binary to 0x7000

  for(c = cpus; c < cpus+ncpu; c++){
    printf("booting cpu%d: starting AP %d\n", cpuid(), c->apicid);
    if(c == mycpu())  // We've started already.
      continue;

    //set a pgentry mapping 0-2MB vm to 0-2MB physcial memory for ap booting
    *(uint64*) 0x4000 = (uint64)0xe3;
    *(uint64*) 0x2000 = (uint64)0x4023;

    *(uint32*)(0x7000-4) = (uint32)((uint64)apstart & 0xffffffff); // store the address of apstart so aps can jmp to it.
    
    //start aps and set up 1 page of stack for them
    stack = kalloc();
    *(uint64*)(0x7000-12) = ((uint64) (stack) + 4096);
    lapicstartap(c->apicid, (uint64)0x7000);

    // wait for cpu to finish mpmain()
    while(c->started == 0){;
    }

  }

}
