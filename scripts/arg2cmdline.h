#ifndef _ARG2CMDLINE_H_
#define _ARG2CMDLINE_H_
#include <wchar.h>

// Result require `free`
wchar_t* arg2cmdline(const wchar_t *arg);
wchar_t* arglist2cmdline(int argc, const wchar_t *argv[]);

#endif
