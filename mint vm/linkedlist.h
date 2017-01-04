// linkedlist.h -- linked list utility for the mint lang compiler
typedef struct _LinkedListNode
{
	struct _LinkedListNode* next;
	void* data;
} LinkedListNode;

typedef struct _LinkedList
{
	LinkedListNode* head;
	LinkedListNode* tail;
	int length;
} LinkedList;

void InitList(LinkedList* list);
int AddNode(LinkedList* list, void* data);
void FreeList(LinkedList* list);
