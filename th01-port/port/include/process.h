#pragma once
// Portable replacement for Borland <process.h>.
#include <unistd.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int _spawn(const char *path, const char *args, const char *env)
{
    (void)path; (void)args; (void)env;
    return -1;
}
static inline int spawnl(int mode, const char *path, const char *arg0, ...)
{
    (void)mode; (void)path; (void)arg0;
    return -1;
}

#ifdef __cplusplus
}
#endif
