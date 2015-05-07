// dict.h -- dictionary implementation for mint vm
#ifndef MINT_DICT_H
#define MINT_DICT_H

#define INIT_DICT_CAPACITY 8

struct _Object;
typedef struct _DictNode
{
	struct _DictNode* next;
	char* key;
	struct _Object* value;
} DictNode;

typedef struct _Dict
{
	DictNode** buckets;
	int capacity;
	int used;
} Dict;

void InitDict(Dict* dict);
void DictPut(Dict* dict, const char* key, struct _Object* value);
struct _Object* DictRemove(Dict* dict, const char* key);
struct _Object* DictGet(Dict* dict, const char* key);
void FreeDict(Dict* dict);

#endif
