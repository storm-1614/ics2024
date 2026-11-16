/*
 * dtrace.c
 *
 * Author: storm1614.top
 * Date: 2026-11-16
 */

#include <debug.h>
#include <isa.h>
#include <utils/dtrace.h>

void dtrace_log(IOMap *map, paddr_t addr, int len, bool is_write, word_t data)
{
#ifdef CONFIG_DTRACE_DEV
    if (CONFIG_DTRACE_DEV[0] != '\0' && strstr(map->name, CONFIG_DTRACE_DEV) == NULL)
    {
        return;
    }
#endif
    paddr_t offset = addr - map->low;

    Log("[dtrace] %c %s @ " FMT_PADDR " +" FMT_PADDR " len=%d data=" FMT_PADDR " pc=" FMT_WORD, is_write ? 'W' : 'R',
        map->name, addr, offset, len, data, cpu.pc);
}
