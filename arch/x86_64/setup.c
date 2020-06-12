/* Temporary file to help with general setup and jump
 * to 64-bit execution */
#include "mmu.h"

// Bootstrap GDT. Implements a flat segmentation model where logical
// addresses are identity mapped to linear addresses. The descriptors
// correspond to the indexes as follows:
// 0 - Null descriptor. Required.
// 1 - 32-bit kernel code segment
// 2 - 64-bit kernel code segment
// 3 - data segment
// 6 - user data segment
// 7 - user code segment
struct segdesc bootgdt[NSEGS] = {
  [0] = SEGDESC(0, 0, 0), // Null
  [1] = SEGDESC(0, 0xFFFFF, SEG_R|SEG_CODE|SEG_S|SEG_DPL(0)|SEG_P|SEG_D|SEG_G),
  [2] = SEGDESC(0, 0, SEG_R|SEG_CODE|SEG_S|SEG_DPL(0)|SEG_P|SEG_L|SEG_D|SEG_G),
  [3] = SEGDESC(0, 0xFFFFF, SEG_W|SEG_S|SEG_DPL(0)|SEG_P|SEG_D|SEG_G),
  // The order of the user data and user code segments is
  // important for syscall instructions.  See initseg.
  [6] = SEGDESC(0, 0, SEG_W|SEG_S|SEG_DPL(DPL_USER)|SEG_P|SEG_D|SEG_G),
  [7] = SEGDESC(0, 0xFFFFF, SEG_R|SEG_S|SEG_DPL(DPL_USER)|SEG_P|SEG_L|SEG_D|SEG_G),
};
