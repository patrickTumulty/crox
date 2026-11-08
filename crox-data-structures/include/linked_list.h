
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct lnode_s;

typedef struct lnode_s
{
    void *data;
    struct lnode_s *next;
    struct lnode_s *prev;
} lnode_t;

typedef struct
{
    lnode_t *head;
    lnode_t *tail;
    int size;
} llist_t;

#define LLIST_FOR_EACH(LL, IT) for ((IT) = (LL)->head; (IT) != NULL; (IT) = (IT)->next)
#define LLIST_GET_ENTRY(NODE, T) ((T *)(NODE)->data)
#define LLIST_STATIC_INIT() ((LList){.head = nullptr, .tail = nullptr, .size = 0})

void llist_init(llist_t *llist);
void llist_append(llist_t *llist, lnode_t *lnode);
void llist_insert(llist_t *llist, uint32_t index, lnode_t *lnode);
void llist_push_front(llist_t *llist, lnode_t *lnode);
void llist_push_back(llist_t *llist, lnode_t *lnode);
lnode_t *llist_pop_back(llist_t *llist);
lnode_t *llist_pop_front(llist_t *llist);
lnode_t *llist_remove(llist_t *llist, lnode_t *lnode);
void llist_init_node(lnode_t *lnode, void *data);
bool llist_is_empty(llist_t *llist);

#endif // LINKED_LIST_H
