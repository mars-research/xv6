#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "defs.h"

volatile static int started = 0;
extern uint64 end[];

// start() jumps here in supervisor mode on all CPUs.
// TODO: remove all references to x86-specific functions here
void
main()
{
  if (0 == cpuid()) {
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 is booting\n");
    printf("\n");
    kinit();
    kpaginginit();
    procinit();
    trapinit();
    seginit();
    lapicinit();
    picinit();
    ioapicinit();
    binit();
    iinit();
    fileinit();
    diskinit();
    userinit();
    printf("done\n");
    while (1);
  }
}
