#ifndef __DTRACE_H__
#define __DTRACE_H__

#include <device/map.h>

void dtrace_log(IOMap *map, paddr_t addr, int len, bool is_write, word_t data);

#endif // !__DTRACE_H__
