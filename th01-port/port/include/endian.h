#pragma once
// Minimal endian.h for building ReC98's host-side pipeline tools with MinGW.
#ifndef le16toh
#define le16toh(x) (x)
#endif
#ifndef le32toh
#define le32toh(x) (x)
#endif
#ifndef htole16
#define htole16(x) (x)
#endif
#ifndef htole32
#define htole32(x) (x)
#endif
