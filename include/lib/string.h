#ifndef _STRING_H
#define _STRING_H

typedef unsigned long size_t;

/* Existing */
void   itoa(unsigned long val, char *buf);
void   strrev(char *s);
void   strcat(char *dst, const char *src);

/* New */
size_t strlen(const char *s);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strtok_r(char *s, const char *delim, char **saveptr);
void  *memset(void *s, int c, size_t n);
char   toupper(char c);
char   tolower(char c);

#endif /*_STRING_H*/
