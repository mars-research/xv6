#ifndef __XV6_TYPES_H_
#define __XV6_TYPES_H_
// Use fixed width integers from stdint.h, and more importantly,
// uintptr_t which has the correct width depending on whether
// we are compiling 32- or 64-bit code.
#include "stdint.h"

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef int64_t  int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uintptr_t uintp;

typedef 
#if defined(__x86_64__)
	uint64 
#elif defined(riscv)
	uint64 
#endif
	pagetable_t;
#endif // __XV6_TYPES_H_
