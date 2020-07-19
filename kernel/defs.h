// This file will be renamed to defs.h once the port is complete.

// Definitions of platform independent APIs
// (implementation could be platform dependent.)

// forward declarations
struct spinlock;
struct sleeplock;

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);

// printf.c
void            printf(char*, ...);
void            panic(char*) __attribute__((noreturn));
void            printfinit(void);

// console.c
void            consputc(int);

// kalloc.c
void            kinit(void);
void            freerange(void*, void*);
void            kfree(void*);
void*           kalloc(void);

// arch/$ARCH/spinlock.c
void            initlock(struct spinlock*, char*);
void            acquire(struct spinlock*);
void            release(struct spinlock*);
int             holding(struct spinlock*);

// sleeplock.c
void            initsleeplock(struct sleeplock*, char*);
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);

// arch/$ARCH/proc.c
int             cpuid(void);
struct cpu*     mycpu(void);
struct proc*    myproc(void);
void            procinit(void);
void            sched(void);
void            sleep(void*, struct spinlock*);
void            wakeup(void*);

// arch/$ARCH/vm.c
void            kvmmap(uint64, uint64, uint64, uint64);
void            kpaginginit(void);
void            loadkpml4(void);
typedef uint64  pagetablee_t; // this is a hack; TODO: fix it
int             copyout(pagetablee_t*, uint64, char*, uint64);
int             copyin(pagetablee_t*, char*, uint64, uint64);
int             copyinstr(pagetablee_t*, char*, uint64, uint64);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
