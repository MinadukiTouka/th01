#pragma once
// Wrapper around the NDK's <fcntl.h> that adds Borland's O_BINARY.
#include_next <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif
