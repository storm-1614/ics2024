/* iringbuf.c
 *
 * Date: 2026-08-25
 * Author: storm1614.top
 *
 * 实现指令环形缓冲区
 */

#include <cpu/cpu.h>
#include <memory/vaddr.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 16
static size_t idx = 0;

typedef struct
{
    vaddr_t pc;
    uint32_t inst; // risv32 指令固定 4 字节
} inst_info;

inst_info buf[BUF_SIZE];

void store_ringbuf(vaddr_t pc)
{
    inst_info *p = &buf[idx % BUF_SIZE];

    p->pc = pc;
    p->inst = vaddr_ifetch(pc, 4);
    idx++;
}

void print_ringbuf()
{
    int i, j;
    printf("print ringbuf :\n");
    for (i = 0; i < BUF_SIZE; i++)
    {
        if (buf[i].inst == 0)
            continue;

        if (i == (idx - 1) % BUF_SIZE)
            printf("---> ");
        else
            printf("     ");

        char logbuf[256];
        char *p = logbuf;
        p += snprintf(p, sizeof(logbuf), FMT_PADDR ":", buf[i].pc);

        uint8_t *inst_u8 = (uint8_t *)&buf[i].inst;
        for (j = 3; j >= 0; j--)
            p += snprintf(p, 4, " %02x", inst_u8[j]);

        void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
        memset(p, ' ', 1);
        p++;
        disassemble(p, 256 - (p - logbuf), buf[i].pc, (uint8_t *)&buf[i].inst, 4);
        printf("%s\n", logbuf);
    }
}
