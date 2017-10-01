#include <stdlib.h>
#include <stdio.h>

#include "linkedlist.h"

void InitList(LinkedList* list)
{
	list->head = list->tail = NULL;
	list->length = 0;
}

int AddNode(LinkedList* list, void* data)
{
	LinkedListNode* newNode = malloc(sizeof(LinkedListNode));
	if(!newNode)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	
	newNode->next = NULL;
	newNode->data = data;
	
	if(!list->length)
		list->head = list->tail = newNode;
	else
	{
		list->tail->next = newNode;
		list->tail = newNode;
	}
	
	++list->length;
	
	return list->length - 1;
}

void FreeList(LinkedList* list)
{
	if(!list) return;
	
	if(list->length)
	{
		LinkedListNode* curr;
		LinkedListNode* next;
		
		curr = list->head;
		while(curr)
		{
			next = curr->next;
			
			if(curr->data)
				free(curr->data);
			
			free(curr);
			
			curr = next;
		}
	}

	free(list);
}

