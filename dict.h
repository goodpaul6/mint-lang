// dict.h -- dictionary implementation for mint vm
#ifndef MINT_DICT_H
#define MINT_DICT_H

#define INIT_DICT_CAPACITY 8

typedef struct _DictNode
{
	struct _DictNode* next;
	char* key;
	void* value;
	int activeIndex;
} DictNode;

typedef struct _Dict
{
	struct
	{
		int* data;
		int length;
		int capacity;
	} active;
	
	DictNode** buckets;
	int capacity;
	int used;
	
	int numEntries;
} Dict;

void InitDict(Dict* dict);
void DictPut(Dict* dict, const char* key, void* value);
void* DictRemove(Dict* dict, const char* key);
void* DictGet(Dict* dict, const char* key);
void FreeDict(Dict* dict);

#endif
