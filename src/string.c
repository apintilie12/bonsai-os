#include "string.h"

void itoa (unsigned long val, char *buf) {
    char d;
    char *p = buf;
    if(val == 0) {
        *buf = '0';
        *(buf+1) = 0;
        return;
    }
    while (val > 0)
    {
       d = val % 10;
       val /= 10;
       d += '0';
       *p = d;
       p++;
    }
    *p = 0;
    strrev(buf);
}

void strrev(char *source) {
    char *tail = source;
    while (*tail != 0) {
        tail++;
    }
    tail--;
    char *head = source;
    while(head < tail) {
        char aux = *tail;
        *tail = *head;
        *head = aux;
        tail--;
        head++;
    }
}

void strcat(char *destination, const char *source) {
    char *p_dest = destination;
    char *p_source = source;
    while(*p_dest != 0) {
        p_dest++;
    }
    while (*p_source != 0) {
        *p_dest = *p_source;
        p_dest++;
        p_source++;
    }
    *p_dest = 0;
}