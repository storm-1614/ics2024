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

#include "debug.h"
#include <monitor/sdb.h>
#include "utils.h"
#include <string.h>

#define NR_WP 32 // 监视点数量

/* WP 结构体
 * NO：监视点编号
 * next：单链表指针
 *      当 WP 空闲（在 free_ 链），next 指向下一个空闲节点
 *      当被占用，(在 head 链),next 指向下一个在用的节点
 */
typedef struct watchpoint
{
    int NO;
    struct watchpoint *next;
    char *expr;
    word_t old_val;

    /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL; // 初始指向 wp_pool[0]

void init_wp_pool()
{
    int i;
    for (i = 0; i < NR_WP; i++)
    {
        wp_pool[i].NO = i;                                           // 给每个 WP 编号
        wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]); // 把池串在一起，最后一个(31)用 NULL
    }

    head = NULL;
    free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

int add_wp(char *e)
{
    WP *p = NULL;
    bool success;
    word_t val = expr(e, &success);
    if (!success)
        return -2;
    if (free_ != NULL)
    {
        p = free_;
        free_ = p->next;
        p->next = head;
        p->expr = strdup(e);
        p->old_val = val;
        head = p;
        return p->NO;
    }
    Log("No more watchpoint");
    return -1;
}

int free_wp(int idx)
{
    WP *p = NULL, *prev = NULL;
    for (p = head; p != NULL; p = p->next)
    {
        if (p->NO == idx)
        {
            if (prev == NULL)
                head = p->next;
            else
                prev->next = p->next;

            free(p->expr);
            p->expr = NULL;
            p->next = free_;
            free_ = p;
            return p->NO;
        }
        prev = p;
    }
    Log("Not watchpoint %d", idx);
    return -1;
}

static void diff_val_change(WP *wp, bool *changed)
{
    Assert(wp != NULL, "Watchpoint is nullptr");
    bool success;
    word_t new_val = expr(wp->expr, &success);
    if (!success)
    {
        Log("Expression error");
        *changed = false;
        return;
    }
    if (new_val == wp->old_val)
    {
        *changed = false;
        return;
    }
    wp->old_val = new_val;
    *changed = true;
}

void watchpoint_step_diff()
{
    WP *p;
    word_t old_value;
    bool change;
    for (p = head; p != NULL; p = p->next)
    {
        old_value = p->old_val;
        diff_val_change(p, &change);
        if (change)
        {
            printf("watchpoint %d: %s old value=%u, new value=%u\n", p->NO, p->expr, old_value, p->old_val);
            nemu_state.state = NEMU_STOP;
        }
    }
}

void list_watch_point()
{
    WP *p;
    for (p = head; p != NULL; p = p->next)
    {
        printf("watchpoint %d: %s\n", p->NO, p->expr);
    }
}
