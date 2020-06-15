// Boot loader.
// 
// Part of the boot sector, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.
// bootmain() loads a multiboot2 kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.

#include "../../types.h"
#include "x86_64.h"

#define SECTSIZE  512
#define SCRATCH_SPACE 0x10000
// Multiboot header is reqd. to be in the first 32768 bytes.
// We only check the first page since we're deliberately placing
// the header right behind the ELF header.
#define MBHEADER_LIMIT 0x2000
#define MBHEADER_MAGIC 0xE85250D6
#define MBHEADER_TAG_ADDR 2
#define MBHEADER_TAG_ENTRY_ADDR 3
#define CHECKSUM_VALID(h) (0 == (h->checksum\
                                 + h->magic + h->arch\
                                 + h->header_len))
#define END_TAG(h, off) (0 == ((struct mbheader_tag*)(h->tags + off))->type)

struct mbheader_tag {
  uint16 type;
  uint16 flags;
  uint32 size;
};

struct mbheader {
  uint32 magic;
  uint32 arch;
  uint32 header_len;
  uint32 checksum;
  uchar  tags[0];
};

struct mbheader_tag_addr {
  struct mbheader_tag tag;
  uint32 header_addr;
  uint32 load_addr;
  uint32 load_end_addr;
  uint32 bss_end_addr;
};

struct mbheader_tag_entry {
  struct mbheader_tag tag;
  uint32 entry_addr;
};

static void readseg(uchar*, uint, uint);
static inline void loados(struct mbheader_tag_addr*, uint);
static inline int endtag(struct mbheader_tag*);
static struct mbheader_tag* next(struct mbheader_tag*);

void
bootmain(void)
{
  void (*entry)(void); /* entry point of OS */
  struct mbheader     *hdr;
  struct mbheader_tag *tag;
  uchar *scratch; /* scratch space */
  uint offset;    /* offset into scratch space */

  // read first MBHEADER_LIMIT bytes into scratch space
  scratch = (uchar *) SCRATCH_SPACE;
  readseg(scratch, MBHEADER_LIMIT, 0);

  // look for valid multiboot2 header.
  // mbheader is reqd. to be 64-bit aligned;
  // keep offset 64-bit aligned by incrementing by 8
  for (offset = 0; offset < MBHEADER_LIMIT; offset += 8) {
    hdr = (struct mbheader *) (scratch + offset);
    if (MBHEADER_MAGIC == hdr->magic && CHECKSUM_VALID(hdr)) {
      break;
    }
  }

  if (MBHEADER_LIMIT <= offset) {
    // failure
    return;
  }

  // look for Address and Entry point tags in the multiboot header;
  // load the OS, and set the entry point, accordingly
  entry = 0;
  tag = (struct mbheader_tag *) hdr->tags;
  for ( ; !endtag(tag); tag = next(tag)) {
    if (MBHEADER_TAG_ADDR == tag->type) {
      loados((struct mbheader_tag_addr *) tag, offset);
    }
    else if (MBHEADER_TAG_ENTRY_ADDR == tag->type) {
      entry =
        (void (*)(void))(((struct mbheader_tag_entry*)tag)->entry_addr);
    }
  }

  if (0 == entry) {
    // entry point not defined
    return;
  }
  // Call the entry point from the multiboot header.
  // Does not return!
  entry();
}

inline void
loados(struct mbheader_tag_addr *tag, uint hdroff)
{
  // see multiboot2 header specification to understand
  // what is being done below
  readseg((uchar*) tag->load_addr,
    (tag->load_end_addr - tag->load_addr),
    hdroff - (tag->header_addr - tag->load_addr));

  if (tag->bss_end_addr > tag->load_end_addr)
    stosb((void*) tag->load_end_addr, 0,
      tag->bss_end_addr - tag->load_end_addr);
}

static inline int
endtag(struct mbheader_tag *tag)
{
  return (0 == tag->type) && (8 == tag->size);
}

static inline struct mbheader_tag*
next(struct mbheader_tag *tag)
{
  return (struct mbheader_tag *)((uchar*)tag + tag->size);
}


static void
waitdisk(void)
{
  // Wait for disk ready.
  while((inb(0x1F7) & 0xC0) != 0x40)
    ;
}

// Read a single sector at offset into dst.
static void
readsect(void *dst, uint offset)
{
  // Issue command.
  waitdisk();
  outb(0x1F2, 1);   // count = 1
  outb(0x1F3, offset);
  outb(0x1F4, offset >> 8);
  outb(0x1F5, offset >> 16);
  outb(0x1F6, (offset >> 24) | 0xE0);
  outb(0x1F7, 0x20);  // cmd 0x20 - read sectors

  // Read data.
  waitdisk();
  insl(0x1F0, dst, SECTSIZE/4);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked.
static void
readseg(uchar* pa, uint count, uint offset)
{
  uchar* epa;

  epa = pa + count;

  // Round down to sector boundary.
  pa -= offset % SECTSIZE;

  // Translate from bytes to sectors; kernel starts at sector 1.
  offset = (offset / SECTSIZE) + 1;

  // If this is too slow, we could read lots of sectors at a time.
  // We'd write more to memory than asked, but it doesn't matter --
  // we load in increasing order.
  for(; pa < epa; pa += SECTSIZE, offset++)
    readsect(pa, offset);
}
