#pragma once

#define LL_EACH(list, type) for(ListNode* _node = (list).head, type* it = _node ? _node->data; \
        _node != NULL; _node = _node->next, it = _node ? _node->data : NULL)

typedef struct _ListNode
{
    struct _ListNode* prev;
	struct _ListNode* next;

	void* data;
} ListNode;

typedef struct _LinkedList
{
	ListNode* head;
	ListNode* tail;
	int length;
} List;

void InitList(List* list);

int AddNode(List* list, void* data);
void* PopNode(List* list);

void DestroyList(List* list);
