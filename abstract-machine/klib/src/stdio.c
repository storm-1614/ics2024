#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <limits.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// 写一个字符的回调函数
typedef void (*emit_fn)(void *context, char c);

typedef struct
{
    char *buf;
    int len;
    int cap;
} stringContext;

// 串口的字符输出
void emit_serial(void *context, char c)
{
    int *cnt = context;
    putch(c);
    (*cnt)++;
}

// 缓冲区的字符输出
void emit_memory(void *context, char c)
{
    stringContext *s = context;
    if (s->len < s->cap - 1)
        s->buf[s->len] = c;
    s->len++;
}
/*
 * 封装写入整形到缓冲区，返回写入数
 */
static void write_int(emit_fn emit, void *context, int val, int width, int zero_pad)
{
    unsigned int u;
    bool neg = false;
    if (val < 0)
    {
        neg = true;
        u = (unsigned int)(-(val + 1)) + 1;
    }
    else
        u = (unsigned int)(val);

    char tmp[12] = {'\0'};
    int i = 0;
    do
    {
        tmp[i++] = '0' + (u % 10);
        u /= 10;
    } while (u != 0);

    int ndigits = i;
    int pad = width - ndigits;
    if (pad > 0 && zero_pad)
    {
        if (neg)
            emit(context, '-');
        while (pad-- > 0)
            emit(context, '0');
        while (i > 0)
            emit(context, tmp[--i]);
        return;
    }
    if (neg)
        emit(context, '-');
    while (i > 0)
        emit(context, tmp[--i]);
}

static void write_str(emit_fn emit, void *context, const char *s)
{
    for (; *s != '\0'; s++)
        emit(context, *s);
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
    return vsnprintf(out, INT_MAX, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...)
{
    int len;
    va_list ap;
    va_start(ap, fmt);
    len = vsprintf(out, fmt, ap);
    va_end(ap);
    return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
    int len;
    va_list ap;
    va_start(ap, fmt);
    len = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return len;
}

static void format_parsing(emit_fn emit, void *context, const char *fmt, va_list ap)
{
    for (; *fmt != '\0'; fmt++)
    {
        if (*fmt != '%')
        {
            emit(context, *fmt);
            continue;
        }
        fmt++;
        int zero_pad = 0, width = 0;
        if (*fmt == '0')
        {
            zero_pad = 1;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9')
        {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (*fmt == 'd')
            write_int(emit, context, va_arg(ap, int), width, zero_pad);
        else if (*fmt == 's')
            write_str(emit, context, va_arg(ap, char *));
        else if (*fmt == '%')
            emit(context, '%');
    }
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
    stringContext s = {.buf = out, .len = 0, .cap = n};
    format_parsing(emit_memory, &s, fmt, ap);
    // 考虑被截断可能的写结束符
    int end = s.len < (int)n ? s.len : (int)n - 1;
    if (n > 0)
        out[end] = '\0';

    return s.len;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int len = 0;
    format_parsing(emit_serial, &len, fmt, ap);
    va_end(ap);
    return len;
}

#endif
