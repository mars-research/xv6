#include "types.h"
#include "defs.h"

volatile static int started = 0;

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
  userinit();
  printf("done\n");

  scheduler();
}
