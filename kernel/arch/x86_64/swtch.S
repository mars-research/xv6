# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save current registers in old. Load from new.	


.globl swtch
swtch:
  # %rip is saved as return address on the stack
  push %rbp
  push %rbx
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15

  # %rsp now points to what looks like a struct context
  # switch stacks
  mov %rsp, (rdi)
  mov %rsi, %rsp

  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %rbx
  pop %rbp

  ret
