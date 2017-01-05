#ifndef MINT_VM_H
#define MINT_VM_H

#include "dict.h"

#include <stdio.h>

#ifdef MINT_FFI_SUPPORT
#include <ffi.h>
#endif

#define MINT_FALSE 0
#define MINT_TRUE 1

struct _VM;
typedef void (*ExternFunction)(struct _VM*);
typedef unsigned char Word;

enum
{	
	OP_GET_RETVAL,
	
	OP_PUSH_NULL,
	OP_PUSH_NUMBER,
	OP_PUSH_STRING,
	OP_CREATE_ARRAY,
	OP_CREATE_ARRAY_BLOCK,
	OP_PUSH_FUNC,
	OP_PUSH_DICT,
	
	// this was removed because dict literals are handled at compile time
	//OP_CREATE_DICT_BLOCK,
	
	// splat operator from python
	OP_EXPAND_ARRAY,
	
	// stores current stack size
	OP_PUSH_STACK,
	// returns to previously stored stack size
	OP_POP_STACK,
	
	OP_LENGTH,
	OP_ARRAY_PUSH,
	OP_ARRAY_POP,
	OP_ARRAY_CLEAR,
	
	// set dict metadict (which should be what has )
	OP_SET_META,
	OP_GET_META,

	OP_DICT_SET,
	OP_DICT_GET,
	
	// these dict ops ignore overloads like GETINDEX and SETINDEX
	OP_DICT_SET_RAW,
	OP_DICT_GET_RAW,
	
	OP_DICT_PAIRS,
	
	// concatenate strings
	OP_CAT,

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
	OP_SHL,
	OP_SHR,
	
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
	
	OP_SETVMDEBUG,

	OP_GETARGS,
	
	OP_FILE, // set current file name
	OP_LINE, // set current line name
	
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
	OBJ_DICT
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
		
		struct { struct _Object* env; int index; Word isExtern; } func;
		 
		// TODO: change this so it doesn't use anon structs
		struct { Dict dict; struct _Object* meta; };
	};
} Object;
 
#define MAX_INDIR						1024
#define MAX_STACK						4096
#define INIT_GC_THRESH					32
#ifdef MINT_FFI_SUPPORT
#define MAX_TRACKED_CALLSTACK_LENGTH 	8
#define MAX_CIF_ARGS					32
#define CIF_STACK_SIZE					4096
#endif
#define NATIVE_STACK_SIZE				4096	

typedef struct
{
	char isActive;
	Object* stack[MAX_STACK];
	int pc, fp, stackSize;
	int numExpandedArgs;
	Object* retVal;
} VMThread;

typedef struct _VM
{
	char isActive;
	
	const char* curFile; // name of the file the produced code is in
	int curLine;		 // line number in the file from where this code was written

	int pc, fp;
	
	Word* program;
	int programLength;
	
	int entryPoint;
	
	int numFunctions;
	char* functionHasEllipsis;
	int* functionPcs;
	Word* functionNumArgs;
	char** functionNames;
	
	const char* lastFunctionName;
	int lastFunctionIndex;

	int numExpandedArgs;
	
	int numNumberConstants;
	double* numberConstants;
	
	int numStringConstants;
	char** stringConstants;
	
	Object* gcHead;
	Object* freeHead;
	
	int numObjects;
	int maxObjectsUntilGc;
	
	Object* retVal;
	
	Object* stack[MAX_STACK];
	int stackSize;
	
	char** globalNames;
	int numGlobals;
	
	int indirStack[MAX_INDIR];
	int indirStackSize;

	char** externNames;
	ExternFunction* externs;
	int numExterns;

#ifdef MINT_FFI_SUPPORT
	ffi_cif cif;
	char cifStack[CIF_STACK_SIZE];
	void* cifValues[MAX_CIF_ARGS];
	size_t cifStackSize;
	unsigned int cifNumArgs;
#endif

	// this stack can be used by native functions to allocate
	// native values with an automatic lifetime (no destructors though)
	size_t nativeStackSize;
	unsigned char nativeStack[NATIVE_STACK_SIZE];

	// if the virtual machine is currently in use by C code then we shouldn't invoke the garbage collector
	// until it exits
	char inExternBody;

	char debug;
} VM;

VM* NewVM(); 

void ErrorExitVM(VM* vm, const char* format, ...);

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
void SetGlobal(VM* vm, int id);	// set global to object on top of stack

void PushObject(VM* vm, Object* obj);
void PushNumber(VM* vm, double value);
void PushString(VM* vm, const char* string);
Object* PushFunc(VM* vm, int id, Word isExtern, Object* env);
Object* PushArray(VM* vm, int length);
Object* PushDict(VM* vm);
void PushNative(VM* vm, void* native, void (*onFree)(void*), void (*onMark)(void*));
void PushNull(VM* vm);

Object* PopObject(VM* vm);
double PopNumber(VM* vm);
const char* PopString(VM* vm);
Object* PopStringObject(VM* vm);
Object* PopFuncObject(VM* vm);
Object** PopArray(VM* vm, int* length);
Object* PopArrayObject(VM* vm);
Object* PopDict(VM* vm);
Object* PopNativeObject(VM* vm);
void* PopNative(VM* vm);
void* PopNativeOrNull(VM* vm);

void* NativeStackAlloc(VM* vm, size_t size);

void ReturnTop(VM* vm);
void ReturnNullObject(VM* vm);

void ExecuteCycle(VM* vm);

void RunVM(VM* vm);

void CollectGarbage(VM* vm);

void DeleteVM(VM* vm);

#endif
