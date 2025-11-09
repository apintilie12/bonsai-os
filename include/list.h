#ifndef _LIST_H
#define _LIST_H

#define CONTAINER_OF(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#ifndef __ASSEMBLER__

#include <stddef.h>

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *prev;
    struct _LIST_ENTRY *next;
} LIST_ENTRY;

static inline void _list_add(LIST_ENTRY *new_entry, LIST_ENTRY *prev, LIST_ENTRY *next) {
    prev->next = new_entry;
    new_entry->prev = prev;
    new_entry->next = next;
    next->prev = new_entry;
}

static inline void _list_del(LIST_ENTRY *prev, LIST_ENTRY *next) {
    prev->next = next;
    next->prev = prev;
}

static inline void list_head_init(LIST_ENTRY *list) {
    list->next = list;
    list->prev = list;
}

static inline void list_add(LIST_ENTRY *head, LIST_ENTRY *new_entry) {
    _list_add(new_entry, head, head->next);
}

static inline void list_add_tail(LIST_ENTRY *head, LIST_ENTRY *new_entry) {
    _list_add(new_entry, head->prev, head);
}

static inline void list_del(LIST_ENTRY *entry) {
    _list_del(entry->prev, entry->next);
}
static inline int list_empty(const LIST_ENTRY *head) {
    return head->next == head;
}

#define LIST_FOR_EACH_ENTRY(pos, head, member)				\
	for (pos = CONTAINER_OF((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = CONTAINER_OF(pos->member.next, typeof(*pos), member))

#endif /* __ASSEMBLER__ */

#endif /* _LIST_H */