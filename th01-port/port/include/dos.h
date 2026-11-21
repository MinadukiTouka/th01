#pragma once
// Portable replacement for Borland <dos.h>.
#include "platform.h"
#include <io.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32
static inline long filelength(int fd)
{
    struct stat st;
    if (fstat(fd, &st) != 0) return -1;
    return (long)st.st_size;
}
#endif

static inline void delay(unsigned int ms)
{
    usleep((useconds_t)ms * 1000u);
}

#ifdef __cplusplus
}
#endif
