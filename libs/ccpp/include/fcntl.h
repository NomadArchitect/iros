#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/off_t.h>

__CCPP_BEGIN_DECLARATIONS
#define O_RDONLY 0x0001
#define O_WRONLY 0x0002
#define O_RDWR   0x0004
#define O_CREAT  0x0008
#define O_EXCL   0x0010
#define O_TRUNC  0x0020

#define F_SETFD 1

int open(char const* __CCPP_RESTRICT __path, int __flags, ...);
int fcntl(int fd, int cmd, ...);
__CCPP_END_DECLARATIONS
