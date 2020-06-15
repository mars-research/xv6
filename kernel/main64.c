#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "defs64.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if (0 == cpuid()) {
    uartinit();
    printfinit();
    printf("\n");
    printf("xv6 is booting\n");
    printf("\n");
    while (1);
  }
}
