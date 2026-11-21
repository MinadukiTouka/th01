// ReC98 portable subset of Turbo C++'s <dos.h> / real-mode helpers.

#ifndef REC98_X86REAL_PORTABLE_H
#define REC98_X86REAL_PORTABLE_H

#include "platform.h"

#define _PORT_DEFS

// Opcodes / interrupts are no-ops in the port; the call sites that matter
// are replaced by port/pc98_runtime.cpp.
#define _outportb_(port, val) port_outportb((uint16_t)(port), (uint8_t)(val))
#define _inportb_(port)       port_inportb((uint16_t)(port))

// Optimally inlined peek/poke variants.
#define _peek_(a, b)    port_peek((uint16_t)(a), (uint16_t)(b))
#define _peekb_(a, b)   port_peekb((uint16_t)(a), (uint16_t)(b))
#define _poke_(a, b, c) port_poke((uint16_t)(a), (uint16_t)(b), (int16_t)(c))
#define _pokeb_(a, b, c) port_pokeb((uint16_t)(a), (uint16_t)(b), (int8_t)(c))

inline uint16_t intvector_segment(uint8_t i)
{
    (void)i;
    // The port has no real interrupt vectors.  Callers that actually need
    // the MDRV2 vector are replaced by the audio backend.
    return 0;
}

#endif // REC98_X86REAL_PORTABLE_H
