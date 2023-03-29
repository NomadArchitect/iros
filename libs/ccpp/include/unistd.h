#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/intptr_t.h>
#include <ccpp/bits/useconds_t.h>

__CCPP_BEGIN_DECLARATIONS

unsigned alarm(unsigned __seconds);

int usleep(useconds_t __useconds);

extern char* optarg;

extern int optind;
extern int opterr;
extern int optopt;

int getopt(int __argc, char* const __argv[], const char* __envp);

__CCPP_END_DECLARATIONS
