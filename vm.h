#ifndef TINY_VM_H
#define TINY_VM_H

#include <stdio.h>

struct _VM;
typedef void (*ExternFunction)(struct _VM*);
typedef unsigned char Word;

enum
{
	OP_PUSH,
	OP_CREATE_ARRAY,
	OP_ARRAY_LENGTH,

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
	
	OP_SETINDEX,
	OP_GETINDEX,

	OP_SET,
	OP_GET,
	
	OP_WRITE,
	OP_READ,
	
	OP_GOTO,
	OP_GOTOZ,

	OP_CALL,
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
	OBJ_NUMBER,
	OBJ_STRING,
	OBJ_ARRAY,
	OBJ_NATIVE
} ObjectType;

typedef struct _Object
{
	char marked;
	
	struct _Object* next;
	ObjectType type;
	
	union
	{
		double number;
		char* string;
		
		struct
		{
			struct _Object** members;
			int length;
		} array;

		struct
		{
			void* value;
			void (*onFree)(void*);
			void (*onMark)(void*);
		} native;
	};
} Object;
 
#define MAX_INDIR		1024
#define MAX_STACK		1024
#define INIT_GC_THRESH	8

typedef struct _VM
{
	int pc, fp;
	
	Word* program;
	int programLength;
	
	int entryPoint;
	
	int numFunctions;
	int* functionPcs;
	
	int numNumberConstants;
	double* numberConstants;
	
	int numStringConstants;
	char** stringConstants;
	
	Object* gcHead;
	
	int numObjects;
	int maxObjectsUntilGc;
	
	Object* stack[MAX_STACK];
	int stackSize;
	
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

void HookExtern(VM* vm, const char* name, ExternFunction func);

void PushNumber(VM* vm, double value);
void PushString(VM* vm, const char* string);
void PushArray(VM* vm, int length);
void PushNative(VM* vm, void* native, void (*onFree)(void*), void (*onMark)(void*));

double PopNumber(VM* vm);
const char* PopString(VM* vm);
Object** PopArray(VM* vm, int* length);
void* PopNative(VM* vm);

void RunVM(VM* vm);

void CollectGarbage(VM* vm);

void DeleteVM(VM* vm);

#endif
