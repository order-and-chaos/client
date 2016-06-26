#include <sys/cdefs.h>

#ifndef __printflike
#define __printflike(format,start) __attribute__((format (printf,format,start)))
#endif
