
#include "linked_list.h"
#include <stdbool.h>

static bool llist_contains_node(llist_t *llist, lnode_t *lnode)
{
    lnode_t *n = NULL;
    LLIST_FOR_EACH(llist, n)
    {
        if (n == lnode)
        {
            return true;
        }
    }
    return false;
}

void llist_init(llist_t *llist)
{
    llist->head = NULL;
    llist->tail = NULL;
    llist->size = 0;
}

void llist_init_node(lnode_t *node, void *data)
{
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
}

bool llist_is_empty(llist_t *llist)
{
    return llist->head == NULL;
}

void llist_append(llist_t *llist, lnode_t *lnode)
{
    llist_push_back(llist, lnode);
}

lnode_t *llist_remove(llist_t *llist, lnode_t *lnode)
{
    if (lnode == NULL || llist == NULL)
    {
        return NULL;
    }

    if (!llist_contains_node(llist, lnode))
    {
        return NULL;
    }

    if (lnode->next != NULL)
    {
        lnode->next->prev = lnode->prev;
    }

    if (llist->head == lnode)
    {
        llist->head = lnode->next;
    }

    if (lnode->prev != NULL)
    {
        lnode->prev->next = lnode->next;
    }

    if (llist->tail == lnode)
    {
        llist->tail = lnode->prev;
    }

    lnode->next = NULL;
    lnode->prev = NULL;

    llist->size--;

    return lnode;
}

void llist_push_front(llist_t *llist, lnode_t *lnode)
{
    if (llist_contains_node(llist, lnode))
    {
        return;
    }

    if (llist->head == NULL)
    {
        llist->head = lnode;
        llist->tail = lnode;
        return;
    }

    lnode_t *head = llist->head;
    head->prev = lnode;
    lnode->next = head;
    llist->head = lnode;
    llist->size++;
}

void llist_push_back(llist_t *llist, lnode_t *lnode)
{
    if (llist->head == NULL)
    {
        llist->head = lnode;
        llist->tail = lnode;
        llist->size = 1;
        return;
    }

    lnode_t *tail = llist->tail;
    tail->next = lnode;
    lnode->prev = tail;
    llist->tail = lnode;
    lnode->next = NULL;
    llist->size++;
}

lnode_t *llist_pop_back(llist_t *llist)
{
    if (llist->tail == NULL)
    {
        return NULL;
    }
    lnode_t *tail = llist->tail;
    llist->tail = tail->prev;
    if (llist->tail != NULL)
    {
        llist->tail->next = NULL;
    }
    llist->size--;
    tail->next = NULL;
    tail->prev = NULL;
    return tail;
}

lnode_t *llist_pop_front(llist_t *llist)
{
    if (llist->head == NULL)
    {
        return NULL;
    }
    lnode_t *head = llist->head;
    llist->head = head->next;
    if (llist->head != NULL)
    {
        llist->head->prev = NULL;
    }
    llist->size--;
    head->next = NULL;
    head->prev = NULL;
    return head;
}
