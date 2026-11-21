#pragma once
// Portable replacement for Borland <alloc.h>.
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *farmalloc(size_t n) { return malloc(n); }
static inline void *farcalloc(size_t n, size_t size) { return calloc(n, size); }
static inline void farfree(void *p) { free(p); }
static inline unsigned long coreleft(void) { return 640UL * 1024UL; }

// Borland heap-debug API used only by TH01's debug mode.
#define _HEAPEMPTY   0
#define _HEAPOK      1
#define _HEAPCORRUPT (-1)
#define _BADNODE     (-2)
#define _FREEENTRY   (-3)
#define _USEDENTRY   (-4)
static inline int heapcheck(void) { return _HEAPOK; }
static inline int heapchecknode(void *p) { (void)p; return _USEDENTRY; }

#ifdef __cplusplus
}
#endif
