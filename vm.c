#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* ObjectTypeNames[] =
{
	"number",
	"string",
	"array",
	"native pointer"
};

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

void InitVM(VM* vm)
{
	vm->program = NULL;
	vm->programLength = 0;
	
	vm->entryPoint = 0;
	
	vm->numFunctions = 0;
	vm->functionPcs = NULL;
	
	vm->numNumberConstants = 0;
	vm->numberConstants = NULL;
	
	vm->numStringConstants = 0;
	vm->stringConstants = NULL;
	
	vm->gcHead = NULL;
	
	vm->numObjects = 0;
	vm->maxObjectsUntilGc = INIT_GC_THRESH;
	
	vm->numGlobals = 0;
	vm->stackSize = 0;
	
	vm->indirStackSize = 0;
	
	vm->pc = -1;
	vm->fp = 0;

	vm->numExterns = 0;
	vm->externs = NULL;

	vm->debug = 0;
}

VM* NewVM()
{
	VM* vm = emalloc(sizeof(VM));
	InitVM(vm);
	return vm;
}

void ResetVM(VM* vm)
{
	if(vm->pc != -1) { fprintf(stderr, "Attempted to reset a running virtual machine\n"); exit(1); }
	
	if(vm->program)
		free(vm->program);
	vm->program = NULL;
	
	if(vm->functionPcs)
		free(vm->functionPcs);
	
	if(vm->numberConstants)
		free(vm->numberConstants);
	
	if(vm->stringConstants)
	{
		for(int i = 0; i < vm->numStringConstants; ++i)
			free(vm->stringConstants[i]);
		free(vm->stringConstants);
	}
	
	vm->stackSize = 0;
	vm->numGlobals = 0;
	vm->numExterns = 0;

	if(vm->externNames)
	{
		for(int i = 0; i < vm->numExterns; ++i)
			free(vm->externNames[i]);
		free(vm->externNames);
	}
	
	if(vm->externs)
		free(vm->externs);
	
	CollectGarbage(vm);
	
	InitVM(vm);
}

/* BINARY FORMAT:
entry point as integer

program length (in words) as integer
program code (must be no longer than program length specified previously)

number of global variables as integer

number of functions as integer
function entry points as integers

number of external functions referenced as integer
string length followed by string as chars (names of external functions as referenced in the code)

number of number constants as integer
number constants as doubles

number of string constants
string length followed by string as chars
*/

void LoadBinaryFile(VM* vm, FILE* in)
{
	int entryPoint;
	fread(&entryPoint, sizeof(int), 1, in);
	
	vm->entryPoint = entryPoint;
	
	int programLength;
	fread(&programLength, sizeof(int), 1, in);
	
	vm->program = emalloc(sizeof(Word) * programLength);
	vm->programLength = programLength;
	
	fread(vm->program, sizeof(Word), programLength, in);
	
	int numGlobals;
	fread(&numGlobals, sizeof(int), 1, in);
	
	vm->numGlobals = numGlobals;
	vm->stackSize = numGlobals;
	vm->maxObjectsUntilGc += numGlobals;

	for(int i = 0; i < vm->numGlobals; ++i)
		vm->stack[i] = NULL;
		
	int numFunctions, numNumberConstants, numStringConstants;
	
	fread(&numFunctions, sizeof(int), 1, in);
	
	vm->functionPcs = emalloc(sizeof(int) * numFunctions);
	vm->numFunctions = numFunctions;
	
	fread(vm->functionPcs, sizeof(int), numFunctions, in);
	
	int numExterns;
	fread(&numExterns, sizeof(int), 1, in);
	
	vm->numExterns = numExterns;
	vm->externNames = emalloc(sizeof(char*) * numExterns);
	vm->externs = emalloc(sizeof(ExternFunction) * numExterns);
	
	for(int i = 0; i < vm->numExterns; ++i)
	{
		int nameLength;
		fread(&nameLength, sizeof(int), 1, in);
		
		char* name = emalloc(sizeof(char) * (nameLength + 1));
		fread(name, sizeof(char), nameLength, in);
		
		name[nameLength] = '\0';
		vm->externNames[i] = name;
		vm->externs[i] = NULL;
	}
	
	fread(&numNumberConstants, sizeof(int), 1, in);
	
	vm->numberConstants = emalloc(sizeof(double) * numNumberConstants);
	vm->numNumberConstants = numNumberConstants;
	
	fread(vm->numberConstants, sizeof(double), numNumberConstants, in);
	
	fread(&numStringConstants, sizeof(int), 1, in);
	
	vm->stringConstants = emalloc(sizeof(char*) * numStringConstants);
	vm->numStringConstants = numStringConstants;
	
	for(int i = 0; i < numStringConstants; ++i)
	{
		int stringLength;
		fread(&stringLength, sizeof(int), 1, in);
		
		char* string = emalloc(sizeof(char) * (stringLength + 1));
		fread(string, sizeof(char), stringLength, in);
		
		string[stringLength] = '\0';
		
		vm->stringConstants[i] = string;
	}
}

void HookExtern(VM* vm, const char* name, ExternFunction func)
{
	int index = -1;
	for(int i = 0; i < vm->numExterns; ++i)
	{
		if(strcmp(vm->externNames[i], name) == 0)
		{
			index = i;
			break;
		}
	}
	
	if(index < 0 || index >= vm->numExterns)
		printf("Warning: supplied invalid extern hook name '%s'; code does not declare this function anywhere!\n", name);
	else
		vm->externs[index] = func;
}

void MarkAll(VM* vm)
{
	for(int i = 0; i < vm->stackSize; ++i)
	{
		Object* reachable = vm->stack[i];
		if(reachable)
		{
			if(reachable->type == OBJ_NATIVE)
			{
				if(reachable->native.onMark)
					reachable->native.onMark(reachable->native.value);
			}
			if(reachable->type == OBJ_ARRAY)
			{
				for(int i = 0; i < reachable->array.length; ++i)
				{
					Object* obj = reachable->array.members[i];
					if(obj)
						obj->marked = 1;
				}
			}

			reachable->marked = 1;
		}
	}
}

void DeleteObject(Object* obj)
{
	if(obj->type == OBJ_STRING)
		free(obj->string);
	if(obj->type == OBJ_NATIVE)
	{
		if(obj->native.onFree)
			obj->native.onFree(obj->native.value);
	}
	if(obj->type == OBJ_ARRAY)
	{
		free(obj->array.members);
		obj->array.length = 0;
	}
	
	free(obj);
}

void Sweep(VM* vm)
{
	Object** obj = &vm->gcHead;
	while(*obj)
	{
		if(!(*obj)->marked)
		{
			Object* unreached = *obj;
			*(obj) = unreached->next;
			DeleteObject(unreached);
			--vm->numObjects;
		}
		else 
		{
			(*obj)->marked = 0;
			obj = &(*obj)->next;
		}
	}
}

void CollectGarbage(VM* vm)
{
	if(vm->debug)
		printf("collecting garbage...\n");
	int numObjects = vm->numObjects;
	MarkAll(vm);
	if(vm->debug)
		printf("marked all objects\n");
	Sweep(vm);
	if(vm->debug)
		printf("cleaned objects\n");
	vm->maxObjectsUntilGc = numObjects * 2 + vm->numGlobals;

	if(vm->debug)
	{
		printf("objects before collection: %i\n"
			   "objects after collection: %i\n", numObjects, vm->numObjects);
	}
}

Object* NewObject(VM* vm, ObjectType type)
{
	if(vm->numObjects == vm->maxObjectsUntilGc) CollectGarbage(vm);
	if(vm->debug)
		printf("creating object: %s\n", ObjectTypeNames[type]);
	
	Object* obj = emalloc(sizeof(Object));
	obj->type = type;
	obj->marked = 0;
	
	obj->next = vm->gcHead;
	vm->gcHead = obj;
	
	++vm->numObjects;
	
	return obj;
}

void PushObject(VM* vm, Object* obj)
{
	if(vm->stackSize == MAX_STACK) { fprintf(stderr, "Stack overflow!\n"); exit(1); }
	vm->stack[vm->stackSize++] = obj;
}

Object* PopObject(VM* vm)
{
	if(vm->stackSize == vm->numGlobals) { fprintf(stderr, "Stack underflow!\n"); exit(1); }
	return vm->stack[--vm->stackSize];
}

void PushNumber(VM* vm, double value)
{
	Object* obj = NewObject(vm, OBJ_NUMBER);
	obj->number = value;
	PushObject(vm, obj);
}

void PushString(VM* vm, const char* string)
{
	Object* obj = NewObject(vm, OBJ_STRING);
	obj->string = estrdup(string);
	PushObject(vm, obj);
}

void PushArray(VM* vm, int length)
{
	Object* obj = NewObject(vm, OBJ_ARRAY);
	obj->array.members = ecalloc(sizeof(Object*), length);
	obj->array.length = length;
	PushObject(vm, obj);
}

void PushNative(VM* vm, void* value, void (*onFree)(void*), void (*onMark)())
{
	Object* obj = NewObject(vm, OBJ_NATIVE);
	obj->native.value = value;
	obj->native.onFree = onFree;
	obj->native.onMark = onMark;
	PushObject(vm, obj);
}

double PopNumber(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_NUMBER) { fprintf(stderr, "Expected number but recieved %s\n", ObjectTypeNames[obj->type]); exit(1); }
	return obj->number;
}

const char* PopString(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_STRING) { fprintf(stderr, "Expected string but recieved %s\n", ObjectTypeNames[obj->type]); exit(1); }
	return obj->string;
}

Object** PopArray(VM* vm, int* length)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_ARRAY) { fprintf(stderr, "Expected array but recieved %s\n", ObjectTypeNames[obj->type]); exit(1); }
	if(length)
		*length = obj->array.length;
	return obj->array.members;
}

void* PopNative(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_NATIVE) { fprintf(stderr, "Expected native pointer but recieved %s\n", ObjectTypeNames[obj->type]); exit(1); }
	return obj->native.value;
}

int ReadInteger(VM* vm)
{
	int value;
	Word* vp = (Word*)(&value);
	
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		vp[i] = vm->program[vm->pc++];
	
	return value;
}

void SetLocal(VM* vm, int index, Object* value)
{
	vm->stack[vm->fp + index] = value;
}

Object* GetLocal(VM* vm, int index)
{
	return vm->stack[vm->fp + index];
}

char* ReadStringFromStdin()
{
	char* string = emalloc(1);
	size_t stringLength = 0;
	size_t stringCapacity = 1;

	int c;

	int i = 0;
	while((c = getc(stdin)) != '\n')
	{	
		if(stringLength + 1 >= stringCapacity)
		{
			stringCapacity *= 2;
			string = erealloc(string, stringCapacity);
		}
		
		string[i++] = c;
	}
	
	string[i] = '\0';
	return string;
}

void PushIndir(VM* vm, int nargs)
{
	vm->indirStack[vm->indirStackSize++] = nargs;
	vm->indirStack[vm->indirStackSize++] = vm->fp;
	vm->indirStack[vm->indirStackSize++] = vm->pc;
	
	vm->fp = vm->stackSize;
}

void PopIndir(VM* vm)
{
	if(vm->indirStackSize == 0)
	{
		vm->pc = -1;
		return;
	}
	
	if(vm->debug)
		printf("previous fp: %i\n", vm->fp);

	vm->stackSize = vm->fp;
	
	vm->pc = vm->indirStack[--vm->indirStackSize];
	vm->fp = vm->indirStack[--vm->indirStackSize];
	vm->stackSize -= vm->indirStack[--vm->indirStackSize];

	if(vm->debug)
		printf("new fp: %i\nnew stack size: %i\n", vm->fp, vm->stackSize);
}

void ExecuteCycle(VM* vm)
{
	if(vm->pc == -1) return;
	if(vm->debug)
		printf("pc %i: ", vm->pc);
	
	if(vm->stackSize < vm->numGlobals)
		printf("Global(s) were removed from the stack!\n");

	switch(vm->program[vm->pc])
	{
		case OP_PUSH:
		{
			if(vm->debug)
				printf("push\n");
			++vm->pc;
			Word isString = vm->program[vm->pc++];
			int index = ReadInteger(vm);
			
			if(isString)
				PushString(vm, vm->stringConstants[index]);
			else
				PushNumber(vm, vm->numberConstants[index]);
		} break;

		case OP_CREATE_ARRAY:
		{
			if(vm->debug)
				printf("create_array\n");
			++vm->pc;
			int length = (int)PopNumber(vm);
			PushArray(vm, length);
		} break;

		case OP_ARRAY_LENGTH:
		{
			if(vm->debug)
				printf("length\n");
			++vm->pc;
			Object* obj = PopObject(vm);
			PushNumber(vm, obj->array.length);
		} break;
	
		#define BIN_OP_TYPE(op, operator, type) case OP_##op: { if(vm->debug) printf("%s\n", #op); Object* val2 = PopObject(vm); Object* val1 = PopObject(vm); Object* result = NewObject(vm, OBJ_NUMBER); result->number = (type)val1->number operator (type)val2->number; PushObject(vm, result); ++vm->pc; } break;
		#define BIN_OP(op, operator) BIN_OP_TYPE(op, operator, double)
		
		BIN_OP(ADD, +)
		BIN_OP(SUB, -)
		BIN_OP(MUL, *)
		BIN_OP(DIV, /)
		BIN_OP_TYPE(MOD, %, int)
		BIN_OP_TYPE(OR, |, int)
		BIN_OP_TYPE(AND, &, int)
		BIN_OP(LT, <)
		BIN_OP(LTE, <=)
		BIN_OP(GT, >)
		BIN_OP(GTE, >=)
		BIN_OP(EQU, ==)
		BIN_OP(NEQU, !=)
		
		case OP_SETINDEX:
		{
			if(vm->debug)
				printf("setindex\n");
			++vm->pc;

			Object* value = PopObject(vm);
			int index = (int)PopNumber(vm);
			int arrayLength;
			Object** members = PopArray(vm, &arrayLength);

			if(index >= 0 && index < arrayLength)
				members[index] = value;
			else
			{
				fprintf(stderr, "Invalid array index %i\n", index);
				exit(1);
			}
		} break;

		case OP_GETINDEX:
		{
			if(vm->debug)
				printf("getindex\n");
			++vm->pc;

			int index = (int)PopNumber(vm);
			int arrayLength;
			Object** members = PopArray(vm, &arrayLength);

			if(index >= 0 && index < arrayLength)
			{
				if(members[index])
					PushObject(vm, members[index]);
				else
					PushNumber(vm, 0);
			}
			else
			{
				fprintf(stderr, "Invalid array index %i\n", index);
				exit(1);
			}
		} break;

		case OP_SET:
		{
			if(vm->debug)
				printf("set\n");
			++vm->pc;
			int index = ReadInteger(vm);
			
			Object* top = PopObject(vm);
			vm->stack[index] = top;
		} break;
		
		case OP_GET:
		{
			if(vm->debug)
				printf("get\n");
			++vm->pc;
			int index = ReadInteger(vm);
			
			PushObject(vm, (vm->stack[index]));
		} break;
		
		case OP_WRITE:
		{
			if(vm->debug)
				printf("write\n");
			Object* top = PopObject(vm);
			if(top->type == OBJ_NUMBER)
				printf("%g\n", top->number);
			if(top->type == OBJ_STRING)
				printf("%s\n", top->string);
			if(top->type == OBJ_NATIVE)
				printf("native pointer\n");
			++vm->pc;
		} break;
		
		case OP_READ:
		{
			if(vm->debug)
				printf("read\n");
			char* string = ReadStringFromStdin();
			PushString(vm, string);
			free(string);
			++vm->pc;
		} break;
		
		case OP_GOTO:
		{
			++vm->pc;
			int pc = ReadInteger(vm);
			vm->pc = pc;
		
			if(vm->debug)
				printf("goto %i\n", vm->pc);
		} break;
		
		case OP_GOTOZ:
		{
			++vm->pc;
			int pc = ReadInteger(vm);
			
			Object* top = PopObject(vm);
			if(top->number == 0)
			{
				vm->pc = pc;
				if(vm->debug)
					printf("gotoz %i\n", vm->pc);
			}
		} break;
		
		case OP_CALL:
		{
			if(vm->debug)
				printf("call\n");
			Word nargs = vm->program[++vm->pc];
			++vm->pc;
			int index = ReadInteger(vm);

			PushIndir(vm, nargs);

			vm->pc = vm->functionPcs[index];
		} break;
		
		case OP_RETURN:
		{
			if(vm->debug)
				printf("ret\n");
			PopIndir(vm);
		} break;
		
		case OP_RETURN_VALUE:
		{
			if(vm->debug)
				printf("retval\n");
			Object* returnValue = PopObject(vm);
			PopIndir(vm);
			PushObject(vm, returnValue);
		} break;
		
		case OP_CALLF:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			if(index < 0 || index >= vm->numExterns)
			{
				fprintf(stderr, "Invalid extern index %i\n", index);
				exit(1);
			}
			if(vm->debug)
				printf("callf %i\n", index);
			vm->externs[index](vm);
		} break;

		case OP_GETLOCAL:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			PushObject(vm, GetLocal(vm, index));
			if(vm->debug)
				printf("getlocal %i (fp: %i, stack size: %i)\n", index, vm->fp, vm->stackSize);
		} break;
		
		case OP_SETLOCAL:
		{
			if(vm->debug)
				printf("setlocal\n");
			++vm->pc;
			int index = ReadInteger(vm);
			SetLocal(vm, index, PopObject(vm));
		} break;
		
		case OP_HALT:
		{
			if(vm->debug)
				printf("halt\n");
			vm->pc = -1;
		} break;
	}
}

void RunVM(VM* vm)
{
	for(int i = 0; i < vm->numExterns; ++i)
	{
		if(!vm->externs[i])
			printf("Unhooked external function '%s'\n", vm->externNames[i]);
	}
	
	vm->pc = vm->entryPoint;
	while(vm->pc != -1)
		ExecuteCycle(vm);
}

void DeleteVM(VM* vm)
{
	if(vm->pc != -1) { fprintf(stderr, "Attempted to delete a running virtual machine\n"); exit(1); }
	ResetVM(vm);
	free(vm);	
}
