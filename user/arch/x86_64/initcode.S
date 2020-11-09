# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"

# exec(init, argv)
.globl start
start:
  movabs $init,      %rdi
  movabs $argv,      %rsi
  mov $SYS_exec,  %rax
  syscall

# for(;;) exit(-1);
exit:
  mov $(-1),     %edi
  mov $SYS_exit, %rax
  syscall

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .quad init # pointers are 64 bits long
  .quad 0
