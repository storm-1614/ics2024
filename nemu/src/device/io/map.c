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
 * NEMU 设备 I/O 映射的通用后端。把客户机访问的设备地址，转成宿主机内存缓冲区的读写，
 * 并在需要时调用设备回调来模拟设备行为。
 ***************************************************************************************/

#include <device/map.h>
#include <isa.h>
#include <memory/host.h>
#include <memory/vaddr.h>

#define IO_SPACE_MAX (32 * 1024 * 1024)

static uint8_t *io_space = NULL;
static uint8_t *p_space = NULL;

/*
 * 从 io_space 分出一段缓冲区供设备存放寄存器或其他状态
 * 分配长度会向上调整到页大小的整数倍
 */
uint8_t *new_space(int size)
{
    uint8_t *p = p_space;
    // page aligned;
    size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
    p_space += size;
    assert(p_space - io_space < IO_SPACE_MAX);
    return p;
}

static void check_bound(IOMap *map, paddr_t addr)
{
    if (map == NULL)
    {
        Assert(map != NULL, "address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, addr, cpu.pc);
    }
    else
    {
        Assert(addr <= map->high && addr >= map->low,
               "address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD, addr,
               map->name, map->low, map->high, cpu.pc);
    }
}

static void invoke_callback(io_callback_t c, paddr_t offset, int len, bool is_write)
{
    if (c != NULL)
    {
        c(offset, len, is_write);
    }
}

/*
 * 分配一块 32 MiB 的数组区内存作为设备状态缓冲区的总空间
 * 将分配游标设到起点，设备初始化时会先调用它，再初始化各个设备
 */
void init_map()
{
    io_space = malloc(IO_SPACE_MAX);
    assert(io_space);
    p_space = io_space;
}

// 检查设备地址后，通过回调准备数据，再从映射缓冲区读取并返回指定长度的数据  
word_t map_read(paddr_t addr, int len, IOMap *map)
{
    assert(len >= 1 && len <= 8);
    check_bound(map, addr);
    paddr_t offset = addr - map->low;
    invoke_callback(map->callback, offset, len, false); // prepare data to read
    word_t ret = host_read(map->space + offset, len);
    return ret;
}

/*
 * 检查设备地址后，将指定长度的数据写入映射缓冲区，再通过回调通知设备处理写入
 */
void map_write(paddr_t addr, int len, word_t data, IOMap *map)
{
    assert(len >= 1 && len <= 8);
    check_bound(map, addr);
    paddr_t offset = addr - map->low;
    host_write(map->space + offset, len, data);
    invoke_callback(map->callback, offset, len, true);
}
