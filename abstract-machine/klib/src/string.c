#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


size_t strlen(const char *s)
{
    size_t len;
    for (len = 0; *(s + len) != '\0'; len++)
        ;
    return len;
}

char *strcpy(char *dst, const char *src)
{
    char *p = dst;
    while ((*p = *src) != '\0')
    {
        ++p;
        ++src;
    }
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src)
{
    size_t dst_len = strlen(dst), i;
    size_t src_len = strlen(src);
    for (i = 0; i < src_len; i++)
        dst[dst_len + i] = src[i];
    dst[dst_len + src_len] = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        ++s1;
        ++s2;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        if (*s1 != *s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        if (*s1 == '\0')
            return 0;
        ++s1;
        ++s2;
    }
    return 0;
}

void *memset(void *s, int c, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        *((char *)s + i) = (char)c;
    return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
    size_t i;
    if (dst < src)
        for (i = 0; i < n; i++)
            *((char *)dst + i) = *((char *)src + i);
    else
        for (i = n; i > 0; i--)
            *((char *)dst + i - 1) = *((char *)src + i - 1);
    return dst;
}

void *memcpy(void *out, const void *in, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        *((char *)out + i) = *((char *)in + i);
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = (const unsigned char *) s1;
    const unsigned char *b = (const unsigned char *) s2;
    size_t i;
    for (i = 0; i < n; i++)
    {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

#endif
