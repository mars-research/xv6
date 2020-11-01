// Definitions of platform independent APIs
// (implementation could be platform dependent.)

// forward declarations
struct spinlock;
struct sleeplock;
struct buf;
struct superblock;
struct inode;
struct stat;
struct pipe;

// extern variables
extern struct spinlock tickslock;
extern uint ticks;

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
void            consoleinit(void);
void            consputc(int);
int             consolewrite(int, uint64, int);
int             consoleread(int, uint64, int);
void            consoleintr(int);

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

// arch/$ARCH/sleeplock.c
void            initsleeplock(struct sleeplock*, char*);
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);

// bio.c
void            binit(void);
struct buf*     bread(uint, uint);
void            bwrite(struct buf*);
void            brelse(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op();
void            end_op();

// file.c
void            fileinit(void);
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// fs.c
void            fsinit(int);
void            iinit();
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);

// syscall.c
int             argint(int, int*);
int             argstr(int, char*, int);
int             argaddr(int, uint64 *);
int             fetchstr(uint64, char*, int);
int             fetchaddr(uint64, uint64*);
void            syscall();

// arch/$ARCH/proc.c
void            procinit(void);
void            userinit(void);
int             cpuid(void);
struct cpu*     mycpu(void);
struct proc*    myproc(void);
void            scheduler(void);
void            sched(void);
void            sleep(void*, struct spinlock*);
void            wakeup(void*);
int             fork(void);
int             either_copyin(void*, int, uint64, uint64);
int             either_copyout(int, uint64, void*, uint64);
pagetable_t*   proc_pagetable(struct proc*);
void            proc_freepagetable(pagetable_t*, uint64);
void            procdump(void);
void            exit(int);
int             wait(uint64);
int             kill(int);
int             growproc(int);
void            yield(void);

// arch/$ARCH/vm.c
void            seginit(void);
void            kvmmap(uint64, uint64, uint64, uint64);
void            kpaginginit(void);
void            switchuvm(struct proc*);
void            switchkvm(void);
void            loadpml4(pagetable_t*);
uint64          walkaddr(pagetable_t*, uint64);
int             copyout(pagetable_t*, uint64, char*, uint64);
int             copyin(pagetable_t*, char*, uint64, uint64);
int             copyinstr(pagetable_t*, char*, uint64, uint64);
pagetable_t*   kvmcreate(void);
void            uvminit(pagetable_t*, uchar*, uint);
uint64          uvmalloc(pagetable_t*, uint64, uint64);
uint64          uvmdealloc(pagetable_t*, uint64, uint64);
void            vmmap(pagetable_t*, uint64, uint64, uint64, uint64);
void            uvmunmap(pagetable_t*, uint64, uint64, int);
void            uvmclear(pagetable_t*, uint64);
int             uvmcopy(pagetable_t*, pagetable_t*, uint64);
void            vmfree(pagetable_t*, uint64);

// arch/$ARCH/exec.c
int             exec(char*, char**);

// arch/$ARCH/ioapic.c
void            ioapicenable(int, int);
void            ioapicinit(void);

// arch/$ARCH/lapic.c
void            lapicinit(void);
void            lapiceoi(void);
void             lapicstartap(uchar apicid, uint addr);

// arch/$ARCH/picirq.c
void            picinit(void);

// arch/$ARCH/kbd.c
void            kbdintr(void);

// arch/$ARCH/ide.c
void            diskinit(void);
void            diskintr(void);
void            diskrw(struct buf *, int);
void            ideintr(void);

// arch/$ARCH/pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, uint64, int);
int             pipewrite(struct pipe*, uint64, int);

// arch/$ARCH/trap.c
void            idtinit(void);
void            trapinit(void);

// arch/$ARCH/mp.c
void            mpinit(void);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
