/*
 * ftrace.c
 *
 * Date: 2026-08-26
 * Author: storm1614.top
 *
 * 实现调用栈追踪，仅实现 RISC-V 32
 */

#include "debug.h"
#include "utils.h"
#include <elf.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define FTRACE_DEPTH 1024

static const char *ftrace_stack[FTRACE_DEPTH];
static int ftrace_top = -1;

typedef struct
{
    uint32_t addr;    // st_value
    const char *name; // 指向 strtab 里的字符串
} FuncSymbol;

static FuncSymbol *funcs;
static int func_count;

static int cmp_func(const void *a, const void *b)
{
    FuncSymbol *fa = (FuncSymbol *)a;
    FuncSymbol *fb = (FuncSymbol *)b;
    if (fa->addr < fb->addr)
        return -1;
    if (fa->addr > fb->addr)
        return 1;
    return 0;
}
/*
 * 初始化 ftrace，解析 ELF 文件
 */
void init_ftrace(const char *elf_file)
{
    int fd = open(elf_file, O_RDONLY);
    Assert(fd >= 0, "failed to open elf file.");

    struct stat st;
    Assert(fstat(fd, &st) >= 0, "Fstat error.");

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    Assert(map != MAP_FAILED, "mmap error.");

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map;            // elf 头
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) // 验证 ELF 魔数
    {
        munmap(map, st.st_size);
        Assert(0, "Not a vaild ELF file: %s\n", elf_file);
    }
    Log("Elf file loaded, entry point: 0x%x\n", ehdr->e_entry);

    Elf32_Shdr *shdr_table = (Elf32_Shdr *)((char *)map + ehdr->e_shoff); // 节头表
    // Elf32_Shdr *shstrtab_hdr = &shdr_table[ehdr->e_shstrndx];             // 节头字符串表

    // char *shstrtab_data = (char *)map + shstrtab_hdr->sh_offset;

    Elf32_Shdr *symtab_hdr = NULL;
    Elf32_Shdr *strtab_hdr = NULL;

    char *symtab_data = NULL;
    char *strtab_data = NULL;
    int sym_count = 0;

    for (int i = 0; i < ehdr->e_shnum; i++)
    { // 找符号表节
        Elf32_Shdr *sh = &shdr_table[i];
        // char *sec_name = shstrtab_data + sh->sh_name;
        if (sh->sh_type == SHT_SYMTAB)
        {
            symtab_hdr = sh;
            symtab_data = (char *)map + sh->sh_offset;
            sym_count = sh->sh_size / sizeof(Elf32_Sym);

            Elf32_Shdr *link_sec = &shdr_table[sh->sh_link];
            strtab_hdr = link_sec;
            strtab_data = (char *)map + link_sec->sh_offset;
            break;
        }
    }

    if (!symtab_hdr || !strtab_hdr)
    {
        munmap(map, st.st_size);
        Assert(0, "Symbol table or string table not found.\n");
    }

    Log("Found .symtab with %d symbols.\n", sym_count);

    for (int i = 0; i < sym_count; i++)
    {
        Elf32_Sym *sym = &((Elf32_Sym *)symtab_data)[i];

        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
            func_count++;
    }

    // 读取函数表
    funcs = malloc(func_count * sizeof(FuncSymbol));

    int j = 0;
    for (int i = 0; i < sym_count; i++)
    {
        Elf32_Sym *sym = &((Elf32_Sym *)symtab_data)[i];
        if (ELF32_ST_TYPE(sym->st_info) != STT_FUNC)
            continue;
        funcs[j].addr = sym->st_value;
        funcs[j].name = strtab_data + sym->st_name;
        j++;
    }

    qsort(funcs, func_count, sizeof(FuncSymbol), cmp_func);

    for (int i = 0; i < func_count && i < 5; i++)
    {
        Log("func[%d] = %s @ 0x%x", i, funcs[i].name, funcs[i].addr);
    }
}

const char *find_func_name(uint32_t addr)
{
    for (int i = 0; i < func_count; i++)
    {
        if (funcs[i].addr > addr)
        {
            if (i == 0)
                return "<unknown>";
            return funcs[i - 1].name;
        }
    }
    return func_count ? funcs[func_count - 1].name : "<unknown>";
}
void ftrace_call(uint32_t pc, uint32_t target)
{
    const char *name = find_func_name(target);
    printf("0x%x:%*scall [%s @ 0x%x]\n",pc, (ftrace_top + 1) * 2, "", name, target);
    if (ftrace_top + 1 < FTRACE_DEPTH)
        ftrace_stack[++ftrace_top] = name;
}

void ftrace_ret(uint32_t pc)
{
    if (ftrace_top < 0)
    {
        printf("ret [<unknown] @ 0x%x (stack empty!)\n", pc);
        return;
    }
    const char *name = ftrace_stack[ftrace_top];
    ftrace_top--;
    printf("0x%x:%*sret [%s]\n",pc, (ftrace_top + 1) * 2, "", name);
}
