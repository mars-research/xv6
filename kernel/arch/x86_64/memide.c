// Fake IDE disk; stores blocks in memory.
// Useful for running kernel without scratch disk.

#include "../../types.h"
#include "../../defs.h"
#include "../../param.h"
#include "mmu.h"
#include "../../proc.h"
#include "x86_64.h"
#include "traps.h"
#include "../../spinlock.h"
#include "../../sleeplock.h"
#include "../../fs.h"
#include "../../buf.h"

extern uchar _binary_fs_img_start[], _binary_fs_img_size[];

static int disksize;
static uchar *memdisk;

void
ideinit(void)
{
  memdisk = _binary_fs_img_start;
  disksize = (uint)(uint64)_binary_fs_img_size;
  disksize = disksize/BSIZE;
}

// Interrupt handler.
void
ideintr(void)
{
  // no-op
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b, int write)
{
  uchar *p;

  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if(b->valid && !write)
    panic("iderw: nothing to do");
  if(b->dev != 1)
    panic("iderw: ide disk 1 not present");

  p = memdisk + b->blockno*BSIZE;


  if(write){
    // write
    memmove(p, b->data, BSIZE);
  } else {
    // read
    memmove(b->data, p, BSIZE);
  }
}

void
diskinit(void)
{
  ideinit();
}

void
diskrw(struct buf *b, int write)
{
  iderw(b, write);
}
