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
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum
{
    TK_NOTYPE = 256, // 空格串
    TK_EQ,
    TK_NUM,   // 数字类型，十进制数字
    TK_DEREF, // * 解引用
    TK_NEG,   // - 负号
    TK_HEX,
};

static struct rule
{
    const char *regex;
    int token_type;
} rules[] = {

    /*
     * 规则按表从上到下匹配
     */

    {" +", TK_NOTYPE},             // spaces
    {"\\+", '+'},                  // plus
    {"\\-", '-'},                  // minus
    {"\\*", '*'},                  // multiply
    {"\\/", '/'},                  // divison
    {"\\(", '('},                  // left bracket
    {"\\)", ')'},                  // right bracket
    {"0[xX][0-9a-fA-F]+", TK_HEX}, // hex number
    {"[0-9]+", TK_NUM},            // dec number
    {"==", TK_EQ},                 // equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++)
    {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0)
        {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token
{
    int type;
    char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

/*
 * 把一行表达式字符串，切成 token，存进全局 tokens[] 数组
 */
static bool make_token(char *e)
{
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0')
    {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++)
        {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
            {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position,
                    substr_len, substr_len, substr_start);

                position += substr_len; // position 移到下一段

                /*
                 * 记录 Token
                 */

                switch (rules[i].token_type)
                {
                case TK_NOTYPE:
                    break;
                case TK_HEX:
                    Assert(substr_len < 32, "溢出! substr_len=%d", substr_len);
                    strncpy(tokens[nr_token].str, substr_start, substr_len);
                    tokens[nr_token].str[substr_len] = '\0'; // 补结束符
                    tokens[nr_token].type = TK_HEX;
                    nr_token++;
                    break;
                case TK_NUM:
                    Assert(substr_len < 32, "溢出! substr_len=%d", substr_len);
                    strncpy(tokens[nr_token].str, substr_start, substr_len);
                    tokens[nr_token].str[substr_len] = '\0'; // 补结束符
                    tokens[nr_token].type = TK_NUM;
                    nr_token++;
                    break;
                default:
                    tokens[nr_token].type = rules[i].token_type;
                    nr_token++;
                    break;
                }

                break;
            }
        }

        if (i == NR_REGEX)
        {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

// 检查是否被最外层的一对括号完整包裹
// 需要验证这对括号中间有没有提前闭合
bool check_parentheses(int p, int q)
{
    // 最基本的检查，开头结尾必须是闭合括号
    if (tokens[p].type != '(' || tokens[q].type != ')')
        return false;
    int depth = 0;              // 记录括号深度
    for (int i = p; i < q; i++) // 只检查 p ~ q-1 区间，深度有没有提前归零
    {
        if (tokens[i].type == '(')
            depth++;
        else if (tokens[i].type == ')')
            depth--;

        Assert(depth >= 0, "Error depth"); // 右括号先于左括号，括号不配对

        if (depth == 0)
            return false;
    }
    return depth == 1;
}

/* 找主运算符
 */
int find_dominant_operator(int p, int q)
{
    int op = -1;
    int depth = 0;
    int best_prio = 10; // 越高越不优先

    for (int i = p; i <= q; i++)
    {
        int prio;
        switch (tokens[i].type)
        {
        case '(':
            depth++;
            continue;
        case ')':
            depth--;
            continue;
        case '+':
        case '-':
            prio = 1;
            break;
        case '*':
        case '/':
            prio = 2;
            break;
        default:
            continue;
        }

        if (depth == 0 && prio <= best_prio)
        {
            op = i;
            best_prio = prio;
        }
    }
    return op;
}

int eval(int p, int q)
{
    Assert(p <= q, "Bad expression");
    if (p == q) // 递归到只剩下一个词，仅剩下操作数
    {
        Assert(tokens[p].type == TK_NUM || tokens[p].type == TK_HEX, "Is not Number");
        if (tokens[p].type == TK_NUM)
            return (int)strtol(tokens[p].str, NULL, 10);
        else // 16 进制
            return (int)strtol(tokens[p].str, NULL, 16);
    }

    if (check_parentheses(p, q)) // 去除括号
    {
        return eval(p + 1, q - 1);
    }

    int op = find_dominant_operator(p, q); // 找主运算符
    Assert(op != -1, "Not found dominant operator");
    switch (tokens[op].type)
    {
    case '+':
        return eval(p, op - 1) + eval(op + 1, q);
    case '-':
        return eval(p, op - 1) - eval(op + 1, q);
    case '*':
        return eval(p, op - 1) * eval(op + 1, q);
    case '/':
        return eval(p, op - 1) / eval(op + 1, q);
    default:
        Assert(0, "Error operator");
    }
}

/*
 * 计算，递归求值
 */
word_t expr(char *e, bool *success)
{
    int i;
    if (!make_token(e))
    {
        *success = false;
        return 0;
    }

    for (i = 0; i < nr_token; i++)
    {
        if (tokens[i].type == '*')
        {
            bool ismultiply = (i > 0 && (tokens[i - 1].type == ')' || tokens[i - 1].type == TK_NUM || tokens[i-1].type == TK_HEX));
            if (!ismultiply)
                tokens[i].type = TK_DEREF;
        }
    }

    *success = true;
    return eval(0, nr_token - 1);
}
