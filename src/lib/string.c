#include "lib/string.h"

void itoa(unsigned long val, char *buf) {
    char d;
    char *p = buf;
    if (val == 0) {
        *buf = '0';
        *(buf + 1) = 0;
        return;
    }
    while (val > 0) {
        d = val % 10;
        val /= 10;
        d += '0';
        *p = d;
        p++;
    }
    *p = 0;
    strrev(buf);
}

void strrev(char *s) {
    char *tail = s;
    while (*tail) tail++;
    tail--;
    char *head = s;
    while (head < tail) {
        char tmp = *tail;
        *tail-- = *head;
        *head++ = tmp;
    }
}

void strcat(char *dst, const char *src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *ret = dst;
    while (n && (*dst++ = *src++)) n--;
    while (n--) *dst++ = '\0';
    return ret;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (!n) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strchr(const char *s, int c) {
    for (; *s; s++)
        if (*s == (char)c) return (char *)s;
    return (c == '\0') ? (char *)s : 0;
}

char *strrchr(const char *s, int c) {
    const char *last = 0;
    for (; *s; s++)
        if (*s == (char)c) last = s;
    if (c == '\0') return (char *)s;
    return (char *)last;
}

char *strtok_r(char *s, const char *delim, char **saveptr) {
    if (!s) s = *saveptr;

    // skip leading delimiters
    while (*s) {
        const char *d = delim;
        int is_delim = 0;
        while (*d) { if (*s == *d++) { is_delim = 1; break; } }
        if (!is_delim) break;
        s++;
    }
    if (!*s) { *saveptr = s; return 0; }

    char *token = s;

    // find end of token
    while (*s) {
        const char *d = delim;
        while (*d) {
            if (*s == *d) {
                *s = '\0';
                *saveptr = s + 1;
                return token;
            }
            d++;
        }
        s++;
    }
    *saveptr = s;
    return token;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

char toupper(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

char tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}
