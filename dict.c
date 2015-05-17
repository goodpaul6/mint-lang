#include "dict.h"
#include "vm.h"
#include "hash.h"

#include <stdlib.h>
#include <string.h>

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return mem;
}

static void* ecalloc(size_t size, size_t nmemb)
{
	void* mem = calloc(size, nmemb);
	if(!mem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return mem;
}

static void* erealloc(void* mem, size_t newSize)
{
	void* newMem = realloc(mem, newSize);
	if(!newMem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return newMem;
}

static char* estrdup(const char* string)
{
	char* newString = emalloc(strlen(string) + 1);
	strcpy(newString, string);
	return newString;
}

unsigned long HashFunction(const char* key)
{
	/*
	unsigned long hash = 5381;
	int c;
	
	while ((c = *key++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		
	return SuperFastHash(key, strlen(key));
}

void InitDict(Dict* dict)
{
	dict->buckets = ecalloc(sizeof(DictNode*), INIT_DICT_CAPACITY);
	dict->capacity = INIT_DICT_CAPACITY;
	dict->used = 0;
	
	dict->active.data = ecalloc(sizeof(int), INIT_DICT_CAPACITY);
	dict->active.length = 0;
	dict->active.capacity = INIT_DICT_CAPACITY;
	
	dict->numEntries = 0;
}

void DictResize(Dict* dict, int newCapacity);
void DictPutNode(Dict* dict, DictNode* node)
{
	if(dict->used >= (int)(dict->capacity * (2.0f / 3.0f)))
		DictResize(dict, dict->capacity * 2);
	
	node->activeIndex = dict->active.length;
	
	unsigned long hash = HashFunction(node->key) % dict->capacity;
	if(!dict->buckets[hash])
	{	
		dict->buckets[hash] = node;
		dict->used += 1;
		
		while(dict->active.length + 1 >= dict->active.capacity)
		{
			dict->active.capacity *= 2;
			dict->active.data = erealloc(dict->active.data, sizeof(int) * dict->active.capacity);
		}
		
		dict->active.data[dict->active.length++] = hash;
	}
	else
	{
		node->next = dict->buckets[hash];
		dict->buckets[hash] = node;
	}
	
	++dict->numEntries;
}

void DictResize(Dict* dict, int newCapacity)
{
	int oldCapacity = dict->capacity;
	DictNode** oldBuckets = ecalloc(sizeof(DictNode*), oldCapacity);
	memcpy(oldBuckets, dict->buckets, sizeof(DictNode*) * oldCapacity);
	
	dict->numEntries = 0;
	
	free(dict->active.data);
	dict->active.data = ecalloc(sizeof(int), newCapacity);
	dict->active.length = 0;
	dict->active.capacity = newCapacity;
	
	free(dict->buckets);
	dict->buckets = ecalloc(sizeof(DictNode*), newCapacity);
	dict->used = 0;
	dict->capacity = newCapacity;
	
	for(int i = 0; i < oldCapacity; ++i)
	{
		DictNode* node = oldBuckets[i];
		while(node)
		{
			DictNode* next = node->next;
			DictPutNode(dict, node);
			node = next;
		}
	}
	
	free(oldBuckets);
}

void DictPut(Dict* dict, const char* key, void* value)
{	
	// NOTE: Calculating hash code multiple times in this function call
	// TODO: Don't do that ^ (perhaps pass hashcode as parameter to DictPutNode)
	unsigned long hash = HashFunction(key) % dict->capacity;
	DictNode* node = dict->buckets[hash];
	while(node)
	{
		if(strcmp(node->key, key) == 0)
		{
			node->value = value;
			return;
		}
		node = node->next;
	}
	
	node = emalloc(sizeof(DictNode));
	node->next = NULL;
	node->key = estrdup(key);
	node->value = value;
	
	DictPutNode(dict, node);
}

void* DictRemove(Dict* dict, const char* key)
{
	unsigned long hash = HashFunction(key) % dict->capacity;
		
	DictNode* node = dict->buckets[hash];
	while(node)
	{
		if(strcmp(node->key, key) == 0)
		{
			dict->buckets[hash] = node->next;
			void* value = node->value;
			free(node->key);
			free(node);
			--dict->numEntries;
			return value;
		}
		node = node->next;
	}
	
	return NULL;
}

void* DictGet(Dict* dict, const char* key)
{	
	unsigned long hash = HashFunction(key) % dict->capacity;
	
	DictNode* node = dict->buckets[hash];
	while(node)
	{
		if(strcmp(node->key, key) == 0)
			return node->value;
		node = node->next;
	}
	return NULL;
}

void FreeDict(Dict* dict)
{
	for(int i = 0; i < dict->capacity; ++i)
	{
		DictNode* node = dict->buckets[i];
		DictNode* next;
		
		while(node)
		{
			next = node->next;
			
			if(node->activeIndex + 1 < dict->active.length)
				memmove(&dict->active.data[node->activeIndex], &dict->active.data[node->activeIndex + 1], sizeof(int) * (dict->active.length - node->activeIndex - 1));
			--dict->active.length;
			
			free(node->key);
			free(node);
			node = next;
		}	
	}
	dict->numEntries = 0;
	free(dict->buckets);
}
