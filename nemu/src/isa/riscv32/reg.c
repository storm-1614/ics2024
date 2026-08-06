/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "local-include/reg.h"
#include <isa.h>
#include <string.h>

const char *regs[] = {"$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                      "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

void isa_reg_display()
{
    int i;
    int reg_num = sizeof(regs) / sizeof(const char *);
    printf("Print register:\n");
    for (i = 0; i < reg_num; i++)
    {
        printf("%3s -> 0x%.1x\n", regs[i], cpu.gpr[i]);
    }
}

word_t isa_reg_str2val(const char *s, bool *success)
{
    int i;
    int reg_num = sizeof(regs) / sizeof(const char *);
    if (!strcmp(s, regs[0]))
    {
        *success = true;
        return cpu.gpr[0];
    }
    else if (!strcmp(s, "$pc"))
    {
        *success = true;
        return cpu.pc;
    }
    for (i = 1; i < reg_num; i++)
    {
        if (!strcmp(s + 1, regs[i]))
        {
            *success = true;
            return cpu.gpr[i];
        }
    }
    *success = false;
    return 0;
}
