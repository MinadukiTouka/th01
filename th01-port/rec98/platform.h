// ReC98 portable platform layer for the TH01 Android port.
//
// This replaces the original Borland-only platform.h.  The goal is to keep
// the game code source-compatible: far/near/segment keywords become no-ops,
// and the few real-mode features are provided by port/pc98_runtime.cpp.

#ifndef REC98_PLATFORM_PORTABLE_H
#define REC98_PLATFORM_PORTABLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <alloc.h>

#ifndef CPU
#define CPU 386
#endif

// ---------------------------------------------------------------- keywords
#define far
#define near
#define huge
#define __far
#define __near
#define __huge
#define pascal
#define __pascal
#define cdecl
#define __cdecl
#define __fastcall
#define __ds
#define __cs
#define __ss
#define __es
#define __seg
#define interrupt
#define __interrupt

// Borland's pseudo-register / inline-asm syntax.  The actual sites are
// rewritten in the port; these definitions keep untouched headers parseable.
#define _asm asm
#define __emit__(...) ((void)0)
#define __int__(...)

typedef int16_t bool16;
typedef uint16_t seg_t;

// Generic callback type used by master.lib's vsync hook.
typedef void (*func_t)(void);
typedef void (*func_t_near)(void);
typedef void (*func_t_far)(void);

// Borland <float.h> control-word helpers (no-ops on ARM).
#ifndef MCW_EM
#define MCW_EM 0
#endif
#ifndef _WIN32
static inline unsigned int _control87(unsigned int newcw, unsigned int mask)
{
    (void)newcw; (void)mask;
    return 0;
}
#endif

// Borland <stdlib.h> rotate intrinsics.
static inline uint32_t _lrotl(uint32_t v, int n) { n &= 31; return n ? (v << n) | (v >> (32 - n)) : v; }
static inline uint32_t _lrotr(uint32_t v, int n) { n &= 31; return n ? (v >> n) | (v << (32 - n)) : v; }
static inline uint16_t _rotl(uint16_t v, int n) { n &= 15; return n ? (uint16_t)((v << n) | (v >> (16 - n))) : v; }
static inline uint16_t _rotr(uint16_t v, int n) { n &= 15; return n ? (uint16_t)((v >> n) | (v << (16 - n))) : v; }
#define __rotr__(v, n) _rotr((uint16_t)(v), (n))
#define __rotl__(v, n) _rotl((uint16_t)(v), (n))

#ifndef __cplusplus
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
#endif

// ------------------------------------------------------- real-mode helpers
#ifdef __cplusplus
extern "C" {
#endif

// 1 MiB flat real-mode memory.  Segment S maps to dos_mem + (S << 4).
extern uint8_t *dos_mem;

void *port_mk_fp(uint16_t seg, uint16_t off);
uint16_t port_fp_seg(const void *p);
uint16_t port_fp_off(const void *p);

int16_t port_peek(uint16_t seg, uint16_t off);
int8_t port_peekb(uint16_t seg, uint16_t off);
void port_poke(uint16_t seg, uint16_t off, int16_t v);
void port_pokeb(uint16_t seg, uint16_t off, int8_t v);

uint8_t port_inportb(uint16_t port);
uint16_t port_inportw(uint16_t port);
void port_outportb(uint16_t port, uint8_t v);
void port_outportw(uint16_t port, uint16_t v);

#ifdef __cplusplus
}
#endif

#define MK_FP(seg, off) port_mk_fp((uint16_t)(seg), (uint16_t)(off))
#define FP_SEG(fp)      port_fp_seg((const void *)(fp))
#define FP_OFF(fp)      port_fp_off((const void *)(fp))

#define peek(seg, off)  port_peek((uint16_t)(seg), (uint16_t)(off))
#define peekb(seg, off) port_peekb((uint16_t)(seg), (uint16_t)(off))
#define poke(seg, off, v)  port_poke((uint16_t)(seg), (uint16_t)(off), (int16_t)(v))
#define pokeb(seg, off, v) port_pokeb((uint16_t)(seg), (uint16_t)(off), (int8_t)(v))

#define inportb(p)     port_inportb((uint16_t)(p))
#define inport(p)      port_inportw((uint16_t)(p))
#define outportb(p, v) port_outportb((uint16_t)(p), (uint8_t)(v))
#define outport(p, v)  port_outportw((uint16_t)(p), (uint16_t)(v))

#define disable() ((void)0)
#define enable()  ((void)0)
#define geninterrupt(i) ((void)(i))

// CPU register pseudovariables.  They are only used by legacy I/O/MDRV2
// paths that the port replaces; the values are plain globals for now.
extern uint16_t _AX, _BX, _CX, _DX, _SI, _DI, _ES, _DS;
extern uint8_t _AL, _AH, _BL, _BH, _CL, _CH, _DL, _DH;
extern uint16_t _psp;

struct WORDREGS { uint16_t ax, bx, cx, dx, si, di, cflag, flags; };
struct BYTEREGS { uint8_t al, ah, bl, bh, cl, ch, dl, dh; };
union REGS { struct WORDREGS w; struct BYTEREGS h; };
struct SREGS { uint16_t es, cs, ss, ds; };
typedef void (__cdecl *int_handler_t)(...);

#ifdef __cplusplus
extern "C" {
#endif
int port_int86(int no, union REGS *in, union REGS *out);
int_handler_t port_getvect(int no);
void port_setvect(int no, int_handler_t h);
void port_segread(struct SREGS *s);
#ifdef __cplusplus
}
#endif

#define int86(no, in, out) port_int86((no), (in), (out))
#define getvect(no)        port_getvect((no))
#define setvect(no, h)     port_setvect((no), (int_handler_t)(h))
#define segread(s)         port_segread((s))

#endif // REC98_PLATFORM_PORTABLE_H
