#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <limits.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*
 * 向缓冲区写入一个字节
 */
void writec(char *buf, int *len, int cap, char c)
{
    if (*len < cap - 1)
        buf[*len] = c;
    (*len)++;
}

/*
 * 封装写入整形到缓冲区，返回写入数
 */
void write_int(char *buf, int *len, int cap, int val)
{
    unsigned int u;
    if (val < 0)
    {
        writec(buf, len, cap, '-');
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

    while (i > 0)
    {
        writec(buf, len, cap, tmp[--i]);
    }
}

void write_str(char *buf, int *len, int cap, const char *s)
{
    for (; *s != '\0'; s++)
        writec(buf, len, cap, *s);
}

int printf(const char *fmt, ...)
{
    panic("Not implemented");
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

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
    int len = 0;
    for (; *fmt != '\0'; fmt++)
    {
        if (*fmt != '%')
        {
            writec(out, &len, n, *fmt);
            continue;
        }
        fmt++;
        if (*fmt == 'd')
            write_int(out, &len, n, va_arg(ap, int));
        else if (*fmt == 's')
            write_str(out, &len, n, va_arg(ap, char *));
        else if (*fmt == '%')
            writec(out, &len, n, '%');
    }
    // 考虑被截断可能的写结束符
    int end = len < (int)n ? len : (int)n - 1;
    if (n > 0)
        out[end] = '\0';

    return len;
}

#endif
