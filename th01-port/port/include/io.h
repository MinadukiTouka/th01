#pragma once
// Portable replacement for Borland <io.h>.
#if !defined(__ANDROID__)
#include_next <io.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif
