/* iringbuf.c
 *
 * Date: 2026-08-25
 * Author: storm1614.top
 *
 * 实现指令环形缓冲区
 */

#include <common.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 16
static size_t idx = 0;

typedef struct
{
    vaddr_t pc;
    char buflog[256];
} inst_info;

inst_info buf[BUF_SIZE];

void store_ringbuf(vaddr_t pc, const char *buflog)
{
    inst_info *p = &buf[idx % BUF_SIZE];

    p->pc = pc;
    strncpy(p->buflog, buflog, 255);
    p->buflog[255] = '\0';
    idx++;
}

void print_ringbuf()
{
    printf("print ringbuf :\n");
    int i;
    for (i = 0; i < BUF_SIZE; i++)
    {
        if (buf[i].buflog[0] == '\0')
            continue;
        if (i == (idx - 1) % BUF_SIZE)
            printf("---->");
        else
            printf("     ");
        printf("%s\n", buf[i].buflog);
    }
}
