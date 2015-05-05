#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

static char* ObjectTypeNames[] =
{
	"number",
	"string",
	"array",
	"native pointer",
	"function"
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

/* STANDARD LIBRARY */
void Std_Floor(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, floor(num));
}

void Std_Ceil(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, ceil(num));
}

void Std_Sin(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, sin(num));
}

void Std_Cos(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, cos(num));
}

void Std_Sqrt(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, sqrt(num));
}

void Std_Atan2(VM* vm)
{
	double y = PopNumber(vm);
	double x = PopNumber(vm);
	
	PushNumber(vm, atan2(y, x));
}

void Std_Printf(VM* vm)
{
	const char* format = PopString(vm);
	int len = strlen(format);
	
	for(int i = 0; i < len; ++i)
	{
		int c = format[i];
		
		switch(c)
		{
			case '%':
			{
				int type = format[++i];
				switch(type)
				{
					case 'g':
					{
						printf("%g", PopNumber(vm));
					} break;
					
					case 's':
					{
						printf("%s", PopString(vm));
					} break;
					
					case 'c':
					{
						printf("%c", (int)PopNumber(vm));
					} break;
					
					default:
						putc(c, stdout);
						putc(type, stdout);
				}
			} break;
			
			default:
				putc(c, stdout);
		}
	}
}

void Std_Strcat(VM* vm)
{
	const char* a = PopString(vm);
	const char* b = PopString(vm);
	
	int la = strlen(a);
	int lb = strlen(b);
	
	char* newString = alloca(la + lb + 1);
	if(!newString)
	{
		fprintf(stderr, "Out of stack space when alloca'ing\n");
		exit(1);
	}
	strcpy(newString, a);
	strcpy(newString + la, b);
	
	PushString(vm, newString);
}

void Std_Tonumber(VM* vm)
{
	const char* string = PopString(vm);
	PushNumber(vm, strtod(string, NULL));
}

void Std_Tostring(VM* vm)
{
	double number = PopNumber(vm);
	char buf[128];
	sprintf(buf, "%g", number);
	PushString(vm, buf);
}

void Std_Type(VM* vm)
{
	Object* obj = PopObject(vm);
	PushNumber(vm, obj->type);
}

void Std_Assert(VM* vm)
{
	int result = (int)PopNumber(vm);
	const char* message = PopString(vm);
	
	if(!result)
	{
		fprintf(stderr, "Assertion failed (pc %i)\n", vm->pc);
		fprintf(stderr, "%s\n", message);
		exit(1);
	}
}

void Std_Erase(VM* vm)
{
	Object* obj = PopArrayObject(vm);
	int index = (int)PopNumber(vm);
	
	if(index < 0 || index >= obj->array.length)
	{
		fprintf(stderr, "Attempted to erase non-existent index %i\n", index);
		exit(1);
	}
	
	if(index < obj->array.length - 1 && obj->array.length > 1)
		memmove(&obj->array.members[index], &obj->array.members[index + 1], sizeof(Object*) * (obj->array.length - index - 1));
	--obj->array.length;
}

/* END OF STANDARD LIBRARY */

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
	vm->freeHead = NULL;
	
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
		
	if(vm->functionNames)
	{
		for(int i = 0; i < vm->numFunctions; ++i)
			free(vm->functionNames[i]);
		free(vm->functionNames);
	}
	
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
	
	Object* obj = vm->freeHead;
	Object* next = NULL;
	
	while(obj)
	{
		next = obj->next;
		free(obj);
		obj = next;
	}
	
	InitVM(vm);
}

/* BINARY FORMAT:
entry point as integer

program length (in words) as integer
program code (must be no longer than program length specified previously)

number of global variables as integer

number of functions as integer
function entry points as integers
function names [string length followed by string as chars]

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
	
	vm->functionNames = emalloc(sizeof(char*) * numFunctions);
	vm->functionPcs = emalloc(sizeof(int) * numFunctions);
	vm->numFunctions = numFunctions;
	
	fread(vm->functionPcs, sizeof(int), numFunctions, in);
	
	for(int i = 0; i < numFunctions; ++i)
	{
		int len;
		fread(&len, sizeof(int), 1, in);
		char* string = emalloc(len + 1);
		fread(string, sizeof(char), len, in);
		string[len] = '\0';
		vm->functionNames[i] = string;
	}
	
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

void HookStandardLibrary(VM* vm)
{
	HookExternNoWarn(vm, "floor", Std_Floor);
	HookExternNoWarn(vm, "ceil", Std_Ceil);
	HookExternNoWarn(vm, "sin", Std_Sin);
	HookExternNoWarn(vm, "cos", Std_Cos);
	HookExternNoWarn(vm, "sqrt", Std_Sqrt);
	HookExternNoWarn(vm, "atan2", Std_Atan2);
	HookExternNoWarn(vm, "printf", Std_Printf);
	HookExternNoWarn(vm, "strcat", Std_Strcat);
	HookExternNoWarn(vm, "tonumber", Std_Tonumber);
	HookExternNoWarn(vm, "tostring", Std_Tostring);
	HookExternNoWarn(vm, "type", Std_Type);
	HookExternNoWarn(vm, "assert", Std_Assert);
	HookExternNoWarn(vm, "erase", Std_Erase);
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

void HookExternNoWarn(VM* vm, const char* name, ExternFunction func)
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
	
	if(index >= 0 && index < vm->numExterns)
		vm->externs[index] = func;
}

void CheckExterns(VM* vm)
{
	for(int i = 0; i < vm->numExterns; ++i)
	{
		if(!vm->externs[i])
		{
			fprintf(stderr, "Unbound extern '%s'\n", vm->externNames[i]);
			exit(1);
		}
	}
}

int GetFunctionId(VM* vm, const char* name)
{
	for(int i = 0; i < vm->numFunctions; ++i)
	{
		if(strcmp(vm->functionNames[i], name) == 0)
			return i;
	}
	
	printf("Warning: function '%s' does not exist in mint source\n", name);
	return -1;
}

void MarkObject(Object* obj)
{
	if(obj->marked) return;
	
	if(obj->type == OBJ_NATIVE)
	{
		if(obj->native.onMark)
			obj->native.onMark(obj->native.value);
	}
	if(obj->type == OBJ_ARRAY)
	{
		for(int i = 0; i < obj->array.length; ++i)
		{
			Object* mem = obj->array.members[i];
			if(mem)
				MarkObject(mem);
		}
	}

	obj->marked = 1;
}

void MarkAll(VM* vm)
{
	for(int i = 0; i < vm->stackSize; ++i)
	{
		Object* reachable = vm->stack[i];
		if(reachable)
			MarkObject(reachable);
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
			
			unreached->next = vm->freeHead;
			vm->freeHead = unreached;
			
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
	vm->maxObjectsUntilGc = vm->numObjects * 2 + vm->numGlobals;

	if(vm->debug)
	{
		printf("objects before collection: %i\n"
			   "objects after collection: %i\n", numObjects, vm->numObjects);
	}
}

Object* NewObject(VM* vm, ObjectType type)
{
	if(vm->numObjects == vm->maxObjectsUntilGc) 
		CollectGarbage(vm);

	if(vm->debug)
		printf("creating object: %s\n", ObjectTypeNames[type]);
	
	Object* obj;
	if(!vm->freeHead)
		obj = emalloc(sizeof(Object));
	else
	{
		obj = vm->freeHead;
		vm->freeHead = obj->next;
	}
	
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

void PushFunc(VM* vm, int id, Word isExtern)
{
	Object* obj = NewObject(vm, OBJ_FUNC);
	
	obj->func.index = id;
	obj->func.isExtern = isExtern;
	
	PushObject(vm, obj);
}

Object* PushArray(VM* vm, int length)
{
	Object* obj = NewObject(vm, OBJ_ARRAY);
	
	if(length == 0)
		obj->array.capacity = 2;
	else
		obj->array.capacity = length;
	
	obj->array.members = ecalloc(sizeof(Object*), obj->array.capacity);
	obj->array.length = length;
	
	PushObject(vm, obj);
	return obj;
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

int PopFunc(VM* vm, Word* isExtern)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_FUNC) { fprintf(stderr, "Expected function but received %s\n", ObjectTypeNames[obj->type]); exit(1); }
	if(isExtern)
		*isExtern = obj->func.isExtern;
	return obj->func.index;
}

Object** PopArray(VM* vm, int* length)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_ARRAY) { fprintf(stderr, "Expected array but recieved %s\n", ObjectTypeNames[obj->type]); exit(1); }
	if(length)
		*length = obj->array.length;
	return obj->array.members;
}

Object* PopArrayObject(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_ARRAY) { fprintf(stderr, "Expected array but received %s\n", ObjectTypeNames[obj->type]); exit(1); }
	return obj;
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
	vm->stack[vm->fp + index + vm->numGlobals] = value;
}

Object* GetLocal(VM* vm, int index)
{
	return vm->stack[vm->fp + index + vm->numGlobals];
}

char* ReadStringFromStdin()
{
	char buf[256];
	int c = getc(stdin);
	int i = 0;
	
	while(c != '\n')
	{
		if(i + 1 >= 256) 
		{
			buf[i] = '\0';
			return estrdup(buf);
		}
		
		buf[i++] = c;
		c = getc(stdin);
	}
	buf[i] = '\0';
	
	return estrdup(buf);
}

void PushIndir(VM* vm, int nargs)
{
	vm->indirStack[vm->indirStackSize++] = nargs;
	vm->indirStack[vm->indirStackSize++] = vm->fp;
	vm->indirStack[vm->indirStackSize++] = vm->pc;
	
	vm->fp = vm->stackSize - vm->numGlobals;
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

	vm->stackSize = vm->fp + vm->numGlobals;
	
	vm->pc = vm->indirStack[--vm->indirStackSize];
	vm->fp = vm->indirStack[--vm->indirStackSize];
	vm->stackSize -= vm->indirStack[--vm->indirStackSize];

	if(vm->debug)
		printf("new fp: %i\nnew stack size: %i\n", vm->fp, vm->stackSize);
}

void ExecuteCycle(VM* vm);
void CallFunction(VM* vm, int id, Word numArgs)
{
	if(id < 0) return;

	int startFp = vm->fp;
	PushIndir(vm, numArgs);
	
	vm->pc = vm->functionPcs[id];
	
	while(vm->fp >= startFp && vm->pc >= 0)
		ExecuteCycle(vm);
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
		case OP_PUSH_NUMBER:
		{
			if(vm->debug)
				printf("push_number\n");
			++vm->pc;
			int index = ReadInteger(vm);
			PushNumber(vm, vm->numberConstants[index]);
		} break;
		
		case OP_PUSH_STRING:
		{
			if(vm->debug)
				printf("push_string\n");
			++vm->pc;
			int index = ReadInteger(vm);
			PushString(vm, vm->stringConstants[index]);
		} break;
		
		case OP_PUSH_FUNC:
		{
			if(vm->debug)
				printf("push_func\n");
			Word isExtern = vm->program[++vm->pc];
			++vm->pc;
			int index = ReadInteger(vm);
			PushFunc(vm, index, isExtern);
		} break;

		case OP_CREATE_ARRAY:
		{
			if(vm->debug)
				printf("create_array\n");
			++vm->pc;
			int length = (int)PopNumber(vm);
			PushArray(vm, length);
		} break;

		case OP_LENGTH:
		{
			if(vm->debug)
				printf("length\n");
			++vm->pc;
			Object* obj = PopObject(vm);
			if(obj->type == OBJ_STRING)
				PushNumber(vm, strlen(obj->string));
			else if(obj->type == OBJ_ARRAY)
				PushNumber(vm, obj->array.length);
			else
			{
				fprintf(stderr, "Attempted to get length of %s\n", ObjectTypeNames[obj->type]);
				exit(1);
			}
		} break;
		
		case OP_ARRAY_PUSH:
		{
			if(vm->debug)
				printf("array_push\n");
			++vm->pc;
			
			Object* obj = PopArrayObject(vm);
			Object* value = PopObject(vm);

			while(obj->array.length + 1 >= obj->array.capacity)
			{
				obj->array.capacity *= 2;
				obj->array.members = erealloc(obj->array.members, obj->array.capacity * sizeof(Object*));
			}
			
			obj->array.members[obj->array.length++] = value;
		} break;
		
		case OP_ARRAY_POP:
		{
			if(vm->debug)
				printf("array_pop\n");
			Object* obj = PopArrayObject(vm);
			if(obj->array.length <= 0)
			{
				fprintf(stderr, "Cannot pop from empty array\n");
				exit(1);
			}
			
			PushObject(vm, obj->array.members[--obj->array.length]);
		} break;
		
		case OP_ARRAY_CLEAR:
		{
			if(vm->debug)
				printf("array_clear\n");
			++vm->pc;
			Object* obj = PopArrayObject(vm);
			obj->array.length = 0;
		} break;
	
		#define BIN_OP_TYPE(op, operator, type) case OP_##op: { if(vm->debug) printf("%s\n", #op); Object* val2 = PopObject(vm); Object* val1 = PopObject(vm); PushNumber(vm, (type)val1->number operator (type)val2->number); ++vm->pc; } break;
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
		BIN_OP_TYPE(LOGICAL_AND, &&, int)
		BIN_OP_TYPE(LOGICAL_OR, ||, int)
		
		case OP_NEG:
		{
			if(vm->debug)
				printf("neg\n");
			
			++vm->pc;
			Object* obj = PopObject(vm);
			PushNumber(vm, -obj->number);
		} break;
		
		case OP_LOGICAL_NOT:
		{
			if(vm->debug)
				printf("not\n");
			
			++vm->pc;
			Object* obj = PopObject(vm);
			PushNumber(vm, !obj->number);
		} break;
		
		case OP_SETINDEX:
		{
			++vm->pc;

			int arrayLength;
			Object** members = PopArray(vm, &arrayLength);
			int index = (int)PopNumber(vm);
			Object* value = PopObject(vm);
			if(vm->debug)
				printf("setindex %i\n", index);
			
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
			++vm->pc;

			Object* obj = PopObject(vm);
			int index = (int)PopNumber(vm);

			if(obj->type == OBJ_ARRAY)
			{
				int arrayLength = obj->array.length;
				Object** members = obj->array.members;
				
				if(index >= 0 && index < arrayLength)
				{
					if(members[index])
						PushObject(vm, members[index]);
					else
					{
						fprintf(stderr, "attempted to index non-existent value in array\n");
						exit(1);
					}
					if(vm->debug)
						printf("getindex %i\n", index);
				}
				else
				{
					fprintf(stderr, "Invalid array index %i\n", index);
					exit(1);
				}
			}
			else if(obj->type == OBJ_STRING)
				PushNumber(vm, obj->string[index]);
			else 
			{
				fprintf(stderr, "Attempted to index a %s\n", ObjectTypeNames[obj->type]);
				exit(1);
			}
		} break;

		case OP_SET:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			
			Object* top = PopObject(vm);
			vm->stack[index] = top;
			
			if(vm->debug)
			{
				if(top->type == OBJ_NUMBER) printf("set %i to %g\n", index, top->number);
				else if(top->type == OBJ_STRING) printf("set %i to %s\n", index, top->string);	
			}
		} break;
		
		case OP_GET:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			PushObject(vm, (vm->stack[index]));
			if(vm->debug)
				printf("get %i\n", index);
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
			Word nargs = vm->program[++vm->pc];
			++vm->pc;
			int index = ReadInteger(vm);

			if(vm->debug)
				printf("call %s\n", vm->functionNames[index]);

			PushIndir(vm, nargs);

			vm->pc = vm->functionPcs[index];
		} break;
		
		case OP_CALLP:
		{
			Word isExtern;		
			Word nargs = vm->program[++vm->pc];
			++vm->pc;
			int id = PopFunc(vm, &isExtern);
			
			if(vm->debug)
				printf("callp %s%s\n", isExtern ? "extern " : "", isExtern ? vm->externNames[id] : vm->functionNames[id]);
			
			if(isExtern)
				vm->externs[id](vm);
			else
			{
				PushIndir(vm, nargs);
				vm->pc = vm->functionPcs[id];
			}
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
			if(vm->debug)
				printf("callf %s\n", vm->externNames[index]);
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
