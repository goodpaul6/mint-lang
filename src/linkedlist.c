#include <assert.h>

#include "utils.h"
#include "linkedlist.h"

void InitList(List* list)
{
    assert(list);

	list->head = list->tail = NULL;
	list->length = 0;
}

int AddNode(List* list, void* data)
{
    assert(list);

	ListNode* newNode = emalloc(sizeof(ListNode));

    newNode->prev = NULL;
	newNode->next = NULL;

	newNode->data = data;
	
	if(!list->length) {
		list->head = list->tail = newNode;
    } else {
		list->tail->next = newNode;
        newNode->prev = list->tail;

		list->tail = newNode;
	}
	
	++list->length;
	
	return list->length - 1;
}

void* PopNode(List* list)
{
    assert(list && list->length > 0);

    ListNode* removed = list->tail; 

    list->tail = removed->prev;
    if(removed == list->head) {
        list->head = NULL;
    }
    
    void* data = removed->data;
    free(removed);

    return data;
}

void DestroyList(List* list)
{ 
    assert(list);

    while(list->head) {
        ListNode* next = list->head->next;

        free(list->head);
        list->head = next;
    }
}

