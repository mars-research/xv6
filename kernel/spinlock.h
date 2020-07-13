#ifndef XV6_SPINLOCK_H_
#define XV6_SPINLOCK_H_

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

#endif // XV6_SPINLOCK_H_
