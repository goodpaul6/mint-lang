#ifndef MINT_VM_H
#define MINT_VM_H

#include "dict.h"

#include <stdio.h>

struct _VM;
typedef void (*ExternFunction)(struct _VM*);
typedef unsigned char Word;

enum
{
	OP_PUSH_NULL,
	OP_PUSH_NUMBER,
	OP_PUSH_STRING,
	OP_CREATE_ARRAY,
	OP_CREATE_ARRAY_BLOCK,
	OP_PUSH_FUNC,
	OP_PUSH_DICT,
	OP_CREATE_DICT_BLOCK,
	
	OP_LENGTH,
	OP_ARRAY_PUSH,
	OP_ARRAY_POP,
	OP_ARRAY_CLEAR,
	
	// fast dictionary operations
	OP_DICT_SET,
	OP_DICT_GET,
	OP_DICT_PAIRS,
	
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_OR,
	OP_AND,
	OP_LT,
	OP_LTE,
	OP_GT,
	OP_GTE,
	OP_EQU,
	OP_NEQU,
	OP_NEG,
	OP_LOGICAL_NOT,
	OP_LOGICAL_AND,
	OP_LOGICAL_OR,
	
	OP_CADD,
	OP_CSUB,
	OP_CMUL,
	OP_CDIV,
	
	OP_SETINDEX,
	OP_GETINDEX,

	OP_SET,
	OP_GET,
	
	OP_WRITE,
	OP_READ,
	
	OP_GOTO,
	OP_GOTOZ,

	OP_CALL,
	OP_CALLP,
	OP_RETURN,
	OP_RETURN_VALUE,

	OP_CALLF,

	OP_GETLOCAL,
	OP_SETLOCAL,
	
	OP_HALT,

	NUM_OPCODES
};

typedef enum 
{
	OBJ_NULL,
	OBJ_NUMBER,
	OBJ_STRING,
	OBJ_ARRAY,
	OBJ_NATIVE,
	OBJ_FUNC,
	OBJ_DICT,
} ObjectType;

typedef struct _Object
{
	char marked;
	
	struct _Object* next;
	ObjectType type;
	
	union
	{
		double number;
		struct { char* raw; } string;
		
		struct
		{
			struct _Object** members;
			int length;
			int capacity;
		} array;

		struct
		{
			void* value;
			void (*onFree)(void*);
			void (*onMark)(void*);
		} native;
		
		struct { int index; Word hasEllipsis; Word isExtern; Word numArgs; } func;
		
		Dict dict;
	};
} Object;
 
#define MAX_INDIR		1024
#define MAX_STACK		1024
#define INIT_GC_THRESH	128

typedef struct _VM
{
	int pc, fp;
	
	Word* program;
	int programLength;
	
	int entryPoint;
	
	int numFunctions;
	int* functionPcs;
	char** functionNames;
	
	int numNumberConstants;
	double* numberConstants;
	
	int numStringConstants;
	char** stringConstants;
	
	Object* gcHead;
	Object* freeHead;
	
	int numObjects;
	int maxObjectsUntilGc;
	
	Object* stack[MAX_STACK];
	int stackSize;
	
	char** globalNames;
	int numGlobals;
	
	int indirStack[MAX_INDIR];
	int indirStackSize;

	char** externNames;
	ExternFunction* externs;
	int numExterns;

	char debug;
} VM;

VM* NewVM(); 

void ResetVM(VM* vm);

void LoadBinaryFile(VM* vm, FILE* in);

void HookStandardLibrary(VM* vm);
void HookExtern(VM* vm, const char* name, ExternFunction func);
void HookExternNoWarn(VM* vm, const char* name, ExternFunction func);

void CheckExterns(VM* vm);

int GetFunctionId(VM* vm, const char* name);
void CallFunction(VM* vm, int id, Word numArgs);

int GetGlobalId(VM* vm, const char* name);
Object* GetGlobal(VM* vm, int id);

void PushObject(VM* vm, Object* obj);
void PushNumber(VM* vm, double value);
void PushString(VM* vm, const char* string);
Object* PushFunc(VM* vm, int id, Word hasEllipsis, Word isExtern, Word numArgs);
Object* PushArray(VM* vm, int length);
Object* PushDict(VM* vm);
void PushNative(VM* vm, void* native, void (*onFree)(void*), void (*onMark)(void*));

Object* PopObject(VM* vm);
double PopNumber(VM* vm);
const char* PopString(VM* vm);
int PopFunc(VM* vm, Word* hasEllipsis, Word* isExtern, Word* numArgs);
Object** PopArray(VM* vm, int* length);
Object* PopArrayObject(VM* vm);
Object* PopDict(VM* vm);
void* PopNative(VM* vm);

void RunVM(VM* vm);

void CollectGarbage(VM* vm);

void DeleteVM(VM* vm);

#endif
