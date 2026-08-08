# PA2 实验报告

## 译码宏噩梦

在 /nemu/src/isa/riscv32/inst.c 的 `static int decode_exec(Decode *s)` 开始进行译码。  
主要是一堆宏嵌套，位运算很恶心人。nemu 用的模式匹配来匹配指令。  

关键就在于从 `s->isa->inst` 取到指令。开始：  
``` c
INSTPAT_START();
INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));

INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
INSTPAT_END();
```

我实在是无法抽离开这么多宏的跳转嵌套，就从 INSTPAT 入手。写下 auipc 的匹配。  

``` c
    INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
```
要匹配右边的 0010111 来确定是 auipc 指令。  
INSTPAT 的结构为： `INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);` 可以从这里知道这条指令的作用：把 pc 和 立即数相加存到 rd 寄存器。  

然后 INSTPAT 是这样的：  
``` c
#define INSTPAT(pattern, ...)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        uint64_t key, mask, shift;                                                                                     \
        pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); /* 计算 key mask shift */                       \
        if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key)      /*模式匹配*/                                    \
        {                                                                                                              \
            INSTPAT_MATCH(s, ##__VA_ARGS__);                                                                           \
            goto *(__instpat_end);                                                                                     \
        }                                                                                                              \
    } while (0)
```

啊这又要问了，pattern_decode 是啥，INSTPAT_INST 是啥，INSTPAT_MATCH 又是啥…… 头大！  
在 decode_exec 最开始声明了两个宏：  
``` c
#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)                                                           \
    {                                                                                                                  \
        int rd = 0;                                                                                                    \
        word_t src1 = 0, src2 = 0, imm = 0;                                                                            \
        decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type));                                               \
        __VA_ARGS__;                                                                                                   \
    }
```

INSTPAT_INST 就是把指令抽出来，仅此而已。 pattern_decode 计算 key mask shift。  
pattern_decode 是内联函数，里面有点复杂，不展开，反正 key, mask, shift 进去再出来就算好了。  
- `key` 就是那个指令码
- `mask` 是掩码，用于提取操作码那几位
- `shift` 表示距离最低比特位的数量，用于做偏移方便用 & mask 取出指令码。  

核心就在于 INSTPAT 宏的:  

``` c
((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key)
```

其实这一顿 ` s >>shift & mask` 操作就很好看懂了，与 key 匹配，如果可以就进入 INSTPAT_MATCH 匹配，之后 decode_operand 根据指令类型再做提取。指令类型在 RISC-V 手册写着。  

回到 INSTPAT，我必须再贴一下 `auipc` 的那一行：  
``` c
INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
```

仔细看最后一个参数就是 auipc 所做的对寄存器的操作！  
这可是个了不得的事情，因为 INSTPAT 实际除了第一个字符串之外都是在 ... 的可变参数下。INSTPAT_MATCH 取了点，到最后只剩下 `R(rd) = s-> pc + imm` 没有用到。也就是说：  

``` c
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)                                                           \
    {                                                                                                                  \
        int rd = 0;                                                                                                    \
        word_t src1 = 0, src2 = 0, imm = 0;                                                                            \
        decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type));                                               \
        __VA_ARGS__;                                                                                                   \
    }
```

这里的 __VA_ARGS__ 就是我们指令的命令！我们把执行的操作也做了。  
天哪！终于搞懂了一点点……  
