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
  printf("%d CPUS are available \n",ncpu);
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
  printf("AP NO.%d is up\n",cpuid());
  scheduler();     // start running processes
}

void apstart(void);
static void startothers(void)
{
  struct cpu *c;
  char *stack;
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  void *code = (void*)0x7000;

  memmove(code, _binary_entryother_start, (uint64)_binary_entryother_size); //load entryother binary to 0x7000

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    printf("Main CPU starting AP %d\n", c->apicid);
    *(uint32*)(code-4) = (uint64)apstart; // store the address of apstart so aps can jmp to it.
    
    //start aps and set up 1 page of stack for them
    stack = kalloc();
    *(uint64*)(code-12) = ((uint64)(stack) + PGSIZE);
    lapicstartap(c->apicid, (uint64)code);

    // wait for cpu to finish mpmain()
    while(c->started == 0){;
    }

  }

}

__attribute__((__aligned__(PGSIZE)))
pdpte_t PML4[NPDENTRIES];

__attribute__((__aligned__(PGSIZE)))
pde_t   PDPT[NPDENTRIES];

__attribute__((__aligned__(PGSIZE)))
pte_t   PD_AP[NPDENTRIES];