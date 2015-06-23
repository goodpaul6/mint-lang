#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

Object NullObject;

static char* ObjectTypeNames[] =
{
	"null",
	"number",
	"string",
	"array",
	"native",
	"func",
	"dict"
};

static void* _emalloc(size_t size, int line)
{
	void* mem = malloc(size);
	if(!mem) { fprintf(stderr, "(vm.c:%i) Virtual machine ran out of memory!\n", line); exit(1); }
	return mem;
}

#define emalloc(size) _emalloc((size), __LINE__)

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

void WriteObject(VM* vm, Object* top);
void WriteNonVerbose(VM* vm, Object* obj)
{
	if(obj->type == OBJ_NUMBER || obj->type == OBJ_STRING || obj->type == OBJ_FUNC)
	{
		WriteObject(vm, obj);
		printf("\n");
	}
	else
		printf("%s\n", ObjectTypeNames[obj->type]);
}

Object* GetLocal(VM* vm, int index);
void ErrorExit(VM* vm, const char* format, ...)
{
	if(!vm->hasCodeMetadata) fprintf(stderr, "Error at pc %i (last function called: %s):\n", vm->pc, vm->lastFunctionName);
	else fprintf(stderr, "Error (%s:%i) (last function called: %s):\n", vm->stringConstants[vm->pcFileTable[vm->pc]], vm->pcLineTable[vm->pc], vm->lastFunctionName);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	
#if 0
	for(int stackIndex = vm->stackSize - 1; stackIndex >= 0; --stackIndex)
	{
		Object* obj = vm->stack[stackIndex];
		printf("%i: ", stackIndex);
		WriteNonVerbose(vm, obj);
	}
#endif
	
	printf("pc: %i, fp: %i, stackSize: %i\n", vm->pc, vm->fp, vm->stackSize);

	exit(1);
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

void WriteObject(VM* vm, Object* top)
{
	if(top->type == OBJ_NUMBER)
		printf("%g", top->number);
	else if(top->type == OBJ_STRING)
		printf("%s", top->string);
	else if(top->type == OBJ_NATIVE)
		printf("native pointer");
	else if(top->type == OBJ_FUNC)
	{
		if(top->func.isExtern)
			printf("extern %s", vm->externNames[top->func.index]);
		else
			printf("func %s", vm->functionNames[top->func.index]);
	}
	else if(top->type == OBJ_ARRAY)
	{
		printf("[");
		for(int i = 0; i < top->array.length; ++i)
		{	
			WriteObject(vm, top->array.members[i]);
			if(i + 1 < top->array.length)
				printf(",");
		}
		printf("]");
	}
	else if(top->type == OBJ_DICT)
	{
		printf("{ ");
		for(int i = 0; i < top->dict.active.length; ++i)
		{
			DictNode* node = top->dict.buckets[top->dict.active.data[i]];
			
			while(node)
			{
				printf("%s = ", node->key);
				WriteObject(vm, node->value);
				
				if(node->next || (i + 1 < top->dict.active.length))
					printf(", ");
				node = node->next;
			}
		}
		printf(" }");
	}
	else if(top->type == OBJ_NULL)
		printf("null");
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

					case 'o':
					{
						WriteObject(vm, PopObject(vm));
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
		ErrorExit(vm, "out of memory when alloca'ing\n");
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
	Object* obj = PopObject(vm);
	char buf[128];
	switch(obj->type)
	{
		case OBJ_NULL: sprintf(buf, "null"); break;
		case OBJ_STRING: PushString(vm, obj->string.raw); return;
		case OBJ_NUMBER: sprintf(buf, "%g", obj->number); break;
		case OBJ_ARRAY: sprintf(buf, "array(%i)", obj->array.length); break;
		case OBJ_FUNC: sprintf(buf, "func %s", obj->func.isExtern ? vm->externNames[obj->func.index] : vm->functionNames[obj->func.index]); break;
		case OBJ_DICT: sprintf(buf, "dict(%i)", obj->dict.numEntries); break; 
	}
	
	PushString(vm, buf);
}

void Std_Type(VM* vm)
{
	Object* obj = PopObject(vm);
	PushString(vm, ObjectTypeNames[obj->type]);
}

void Std_Assert(VM* vm)
{
	int result = (int)PopNumber(vm);
	const char* message = PopString(vm);
	
	if(!result)
		ErrorExit(vm, "Assertion failed: %s\n", message);
}

void Std_Erase(VM* vm)
{
	Object* obj = PopArrayObject(vm);
	int index = (int)PopNumber(vm);
	
	if(index < 0 || index >= obj->array.length)
		ErrorExit(vm, "Attempted to erase non-existent index %i\n", index);
	
	if(index < obj->array.length - 1 && obj->array.length > 1)
		memmove(&obj->array.members[index], &obj->array.members[index + 1], sizeof(Object*) * (obj->array.length - index - 1));
	--obj->array.length;
}

void Std_Fclose(void* fp)
{
	fclose(fp);
}

void Std_Fopen(VM* vm)
{
	const char* filename = PopString(vm);
	const char* mode = PopString(vm);
	
	FILE* file = fopen(filename, mode);
	if(!file)
	{	
		PushObject(vm, &NullObject);
		return;
	}
	
	PushNative(vm, file, Std_Fclose, NULL);
}

void Std_Getc(VM* vm)
{
	FILE* file = PopNative(vm);
	PushNumber(vm, getc(file));
}

void Std_Putc(VM* vm)
{
	FILE* file = PopNative(vm);
	int c = (int)PopNumber(vm);
	putc(c, file);
}

void Std_Srand(VM* vm)
{
	srand((unsigned int)time(NULL));
}

void Std_Rand(VM* vm)
{
	PushNumber(vm, (int)rand());
}

void Std_Char(VM* vm)
{
	static char buf[2] = {0};
	buf[0] = (char)PopNumber(vm);
	PushString(vm, buf);
}

void Std_Joinchars(VM* vm)
{
	Object* obj = PopArrayObject(vm);
	if(obj->array.length == 0)
	{
		PushString(vm, "");
		return;
	}
	
	char* str = alloca(obj->array.length + 1);
	for(int i = 0; i < obj->array.length; ++i)
		str[i] = (char)obj->array.members[i]->number;
	str[obj->array.length] = '\0';
	PushString(vm, str);
}

void Std_Clock(VM* vm)
{
	clock_t start = clock();
	PushNumber(vm, (double)start);
}

void Std_Clockspersec(VM* vm)
{
	PushNumber(vm, (double)CLOCKS_PER_SEC);
}

void Std_Halt(VM* vm)
{
	vm->pc = -1;
}

typedef struct
{
	size_t length;
	unsigned char* bytes;
} ByteArray;

void Std_FreeBytes(void* pba)
{
	ByteArray* ba = pba;
	free(ba->bytes);
}

void Std_Bytes(VM* vm)
{
	size_t length = (size_t)PopNumber(vm);
	ByteArray* ba = emalloc(sizeof(ByteArray));
	
	ba->length = length;
	ba->bytes = ecalloc(sizeof(unsigned char), length);
	
	PushNative(vm, ba, Std_FreeBytes, NULL);
}

void Std_GetByte(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	size_t i = (size_t)PopNumber(vm);
	
	PushNumber(vm, ba->bytes[i]);
}

void Std_SetByte(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	size_t i = (size_t)PopNumber(vm);
	unsigned char value = (unsigned char)PopNumber(vm);
	ba->bytes[i] = value;
}

void Std_SetInt(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	size_t i = (size_t)PopNumber(vm);
	int value = (int)PopNumber(vm);
	unsigned char* vp = (unsigned char*)(&value);
	
	for(int i = 0; i < sizeof(int) / sizeof(unsigned char); ++i)
		ba->bytes[i] = *vp++;
}

void Std_BytesLength(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	PushNumber(vm, ba->length);
}

/* END OF STANDARD LIBRARY */

void InitVM(VM* vm)
{
	NullObject.type = OBJ_NULL;
	
	vm->hasCodeMetadata = 0;
	vm->pcLineTable = NULL;
	vm->pcFileTable = NULL;
	
	vm->program = NULL;
	vm->programLength = 0;
	
	vm->entryPoint = 0;
	
	vm->numFunctions = 0;
	vm->functionPcs = NULL;
	
	vm->lastFunctionName = NULL;
	
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
	
	memset(vm->stack, 0, sizeof(vm->stack));
}

VM* NewVM()
{
	VM* vm = emalloc(sizeof(VM));
	InitVM(vm);
	return vm;
}

void ResetVM(VM* vm)
{
	if(vm->pc != -1) ErrorExit(vm, "Attempted to reset a running virtual machine\n");
	
	if(vm->pcLineTable)
		free(vm->pcLineTable);
	if(vm->pcFileTable)
		free(vm->pcFileTable);
	
	if(vm->program)
		free(vm->program);
	
	if(vm->functionPcs)
		free(vm->functionPcs);
	
	if(vm->functionNumArgs)
		free(vm->functionNumArgs);
		
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

	if(vm->globalNames)
	{
		for(int i = 0; i < vm->numGlobals; ++i)
			free(vm->globalNames[i]);
		free(vm->globalNames);
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
		if(obj->type != OBJ_NULL) free(obj);
		obj = next;
	}
	
	InitVM(vm);
}

/* BINARY FORMAT:
entry point as integer

program length (in words) as integer
program code (must be no longer than program length specified previously)

number of global variables as integer
names of global variables as string lengths followed by characters

number of functions as integer
function entry points as integers
number of arguments to each function as integers
function names [string length followed by string as chars]

number of external functions referenced as integer
string length followed by string as chars (names of external functions as referenced in the code)

number of number constants as integer
number constants as doubles

number of string constants
string length followed by string as chars

whether the program has code metadata (i.e line numbers and file names for errors) (represented by char)
if so (length of each of the arrays below must be the same as program length):
line numbers mapping to pcs
file names as indices into string table (integers)
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
	
	vm->globalNames = emalloc(sizeof(char*) * numGlobals);
	
	for(int i = 0; i < numGlobals; ++i)
	{
		int len;
		fread(&len, sizeof(int), 1, in);
		char* string = emalloc(len + 1);
		fread(string, sizeof(char), len, in);
		string[len] = '\0';
		vm->globalNames[i] = string;
	}
	
	vm->numGlobals = numGlobals;
	vm->stackSize = numGlobals;
	vm->maxObjectsUntilGc += numGlobals;

	for(int i = 0; i < vm->numGlobals; ++i)
		vm->stack[i] = NULL;
		
	int numFunctions, numNumberConstants, numStringConstants;
	
	fread(&numFunctions, sizeof(int), 1, in);
	vm->numFunctions = numFunctions;
	
	if(numFunctions > 0)
	{
		vm->functionNames = emalloc(sizeof(char*) * numFunctions);
		vm->functionNumArgs = emalloc(sizeof(int) * numFunctions);
		vm->functionPcs = emalloc(sizeof(int) * numFunctions);
		fread(vm->functionPcs, sizeof(int), numFunctions, in);
		fread(vm->functionNumArgs, sizeof(int), numFunctions, in);
	}
	
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
	
	if(numExterns > 0)
	{
		vm->externNames = emalloc(sizeof(char*) * numExterns);
		vm->externs = emalloc(sizeof(ExternFunction) * numExterns);
	}
	
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
	vm->numNumberConstants = numNumberConstants;
	
	if(numNumberConstants > 0)
	{
		vm->numberConstants = emalloc(sizeof(double) * numNumberConstants);
		fread(vm->numberConstants, sizeof(double), numNumberConstants, in);
	}
	
	fread(&numStringConstants, sizeof(int), 1, in);
	
	if(numStringConstants > 0)
	{
		vm->stringConstants = emalloc(sizeof(char*) * numStringConstants);
		vm->numStringConstants = numStringConstants;
	}
	
	for(int i = 0; i < numStringConstants; ++i)
	{
		int stringLength;
		fread(&stringLength, sizeof(int), 1, in);
		
		char* string = emalloc(sizeof(char) * (stringLength + 1));
		fread(string, sizeof(char), stringLength, in);
		
		string[stringLength] = '\0';
		
		vm->stringConstants[i] = string;
	}
	
	char hasCodeMetadata;
	fread(&hasCodeMetadata, sizeof(char), 1, in);
	vm->hasCodeMetadata = hasCodeMetadata;
	if(hasCodeMetadata)
	{
		vm->pcLineTable = emalloc(sizeof(int) * programLength);
		vm->pcFileTable = emalloc(sizeof(int) * programLength);
		
		fread(vm->pcLineTable, sizeof(int), programLength, in);
		fread(vm->pcFileTable, sizeof(int), programLength, in);
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
	HookExternNoWarn(vm, "rand", Std_Rand);
	HookExternNoWarn(vm, "srand", Std_Srand);
	HookExternNoWarn(vm, "char", Std_Char);
	HookExternNoWarn(vm, "joinchars", Std_Joinchars);
	HookExternNoWarn(vm, "clock", Std_Clock);
	HookExternNoWarn(vm, "getclockspersec", Std_Clockspersec);
	HookExternNoWarn(vm, "halt", Std_Halt);
	HookExternNoWarn(vm, "fopen", Std_Fopen);
	HookExternNoWarn(vm, "getc", Std_Getc);
	HookExternNoWarn(vm, "putc", Std_Putc);
	HookExternNoWarn(vm, "bytes", Std_Bytes);
	HookExternNoWarn(vm, "getbyte", Std_GetByte);
	HookExternNoWarn(vm, "setbyte", Std_SetByte);
	HookExternNoWarn(vm, "setint", Std_SetInt);
	HookExternNoWarn(vm, "lenbytes", Std_BytesLength);
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
			ErrorExit(vm, "Unbound extern '%s'\n", vm->externNames[i]);
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

void MarkObject(VM* vm, Object* obj)
{
	if(!obj)
	{
		fprintf(stderr, "Attempted to mark null object\n");
		return;
	}
	
	if(obj == &NullObject) return;
	if(obj->marked) return;
	
	obj->marked = 1;
	
	if(vm->debug)
		printf("marking %s\n", ObjectTypeNames[obj->type]); 
	
	if(obj->type == OBJ_NATIVE)
	{
		if(obj->native.onMark)
			obj->native.onMark(obj->native.value);
	}
	else if(obj->type == OBJ_ARRAY)
	{
		for(int i = 0; i < obj->array.length; ++i)
		{
			Object* mem = obj->array.members[i];
			if(mem)
				MarkObject(vm, mem);
		}
	}
	else if(obj->type == OBJ_DICT)
	{
		for(int i = 0; i < obj->dict.capacity; ++i)
		{	
			DictNode* node = obj->dict.buckets[i];
			
			while(node)
			{
				if(vm->debug)
					printf("marking node %s value\n", node->key);
				if(node->value)
					MarkObject(vm, node->value);
				node = node->next;
			}
		}
	}
}

void MarkAll(VM* vm)
{
	for(int i = 0; i < vm->stackSize; ++i)
	{
		Object* reachable = vm->stack[i];
		if(reachable)
			MarkObject(vm, reachable);
	}
}

void DeleteObject(Object* obj)
{
	if(obj->type == OBJ_STRING)
		free(obj->string.raw);
	if(obj->type == OBJ_NATIVE)
	{
		if(obj->native.onFree)
			obj->native.onFree(obj->native.value);
	}
	if(obj->type == OBJ_ARRAY)
	{
		free(obj->array.members);
		obj->array.capacity = 0;
		obj->array.length = 0;
	}
	if(obj->type == OBJ_DICT)
		FreeDict(&obj->dict);
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
	if(vm->stackSize == MAX_STACK) ErrorExit(vm, "Stack overflow!\n");
	vm->stack[vm->stackSize++] = obj;
}

Object* PopObject(VM* vm)
{
	if(vm->stackSize == vm->numGlobals) ErrorExit(vm, "Stack underflow!\n");
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
	
	obj->string.raw = estrdup(string);
	
	PushObject(vm, obj);
}

Object* PushFunc(VM* vm, int id, Word hasEllipsis, Word isExtern, Word numArgs)
{
	Object* obj = NewObject(vm, OBJ_FUNC);
	
	obj->func.index = id;
	obj->func.isExtern = isExtern;
	obj->func.hasEllipsis = hasEllipsis;
	obj->func.numArgs = numArgs;
	
	PushObject(vm, obj);
	
	return obj;
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

Object* PushDict(VM* vm)
{
	Object* obj = NewObject(vm, OBJ_DICT);
	InitDict(&obj->dict);
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
	if(obj->type != OBJ_NUMBER) ErrorExit(vm, "Expected number but recieved %s\n", ObjectTypeNames[obj->type]);
	return obj->number;
}

const char* PopString(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_STRING) ErrorExit(vm, "Expected string but recieved %s\n", ObjectTypeNames[obj->type]);
	return obj->string.raw;
}

int PopFunc(VM* vm, Word* hasEllipsis, Word* isExtern, Word* numArgs)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_FUNC) ErrorExit(vm, "Expected function but received %s\n", ObjectTypeNames[obj->type]);
	if(isExtern)
		*isExtern = obj->func.isExtern;
	if(hasEllipsis)
		*hasEllipsis = obj->func.hasEllipsis;
	if(numArgs)
		*numArgs = obj->func.numArgs;
	return obj->func.index;
}

Object** PopArray(VM* vm, int* length)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_ARRAY) ErrorExit(vm, "Expected array but recieved %s\n", ObjectTypeNames[obj->type]);
	if(length)
		*length = obj->array.length;
	return obj->array.members;
}

Object* PopArrayObject(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_ARRAY) ErrorExit(vm, "Expected array but received %s\n", ObjectTypeNames[obj->type]);
	return obj;
}

Object* PopDict(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_DICT) ErrorExit(vm, "Expected dictionary but received %s\n", ObjectTypeNames[obj->type]);
	return obj;
}

void* PopNative(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_NATIVE) ErrorExit(vm, "Expected native pointer but recieved %s\n", ObjectTypeNames[obj->type]);
	return obj->native.value;
}

void* PopNativeOrNull(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_NATIVE && obj->type != OBJ_NULL) ErrorExit(vm, "Expected native pointer or null but received %s\n", ObjectTypeNames[obj->type]);
	if(obj->type == OBJ_NULL) return NULL;
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


int GetGlobalId(VM* vm, const char* name)
{
	for(int i = 0; i < vm->numGlobals; ++i)
	{
		if(strcmp(vm->globalNames[i], name) == 0)
			return i;
	}
	
	return -1;
}

Object* GetGlobal(VM* vm, int id)
{
	if(id < 0) return NULL;
	return vm->stack[id];
}

void ExecuteCycle(VM* vm)
{
	if(vm->pc == -1) return;
	if(vm->debug)
	{
		if(!vm->hasCodeMetadata)
			printf("pc %i: ", vm->pc);
		else
			printf("(%s:%i): ", vm->stringConstants[vm->pcFileTable[vm->pc]], vm->pcLineTable[vm->pc]);
	}
	
	
	if(vm->stackSize < vm->numGlobals)
		printf("Global(s) were removed from the stack!\n");
	
	switch(vm->program[vm->pc])
	{
		case OP_PUSH_NULL:
		{
			if(vm->debug)
				printf("push_null\n");
			++vm->pc;
			PushObject(vm, &NullObject);
		} break;
		
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
			++vm->pc;
			int index = ReadInteger(vm);
			if(vm->debug)
				printf("push_string %s\n", vm->stringConstants[index]);
			PushString(vm, vm->stringConstants[index]);
		} break;
		
		case OP_PUSH_FUNC:
		{
			if(vm->debug)
				printf("push_func\n");
			Word hasEllipsis = vm->program[++vm->pc];
			Word isExtern = vm->program[++vm->pc];
			Word numArgs = vm->program[++vm->pc];
			++vm->pc;
			int index = ReadInteger(vm);
			
			PushFunc(vm, index, hasEllipsis, isExtern, numArgs);
		} break;
		
		case OP_PUSH_DICT:
		{
			if(vm->debug)
				printf("push_dict\n");
			++vm->pc;
			PushDict(vm);
		} break;

		case OP_CREATE_DICT_BLOCK:
		{
			if(vm->debug)
				printf("create_dict_block\n");
			++vm->pc;
			int length = ReadInteger(vm);
			Object* obj = PushDict(vm);
			if(length > 0)
			{
				// stack (before dict) is filled with key-value pairs (backwards, key is higher on stack)
				for(int i = 0; i < length * 2; i += 2)
					DictPut(&obj->dict, vm->stack[vm->stackSize - i - 2]->string.raw, vm->stack[vm->stackSize - i - 3]);
				vm->stackSize -= length * 2;
				vm->stack[vm->stackSize - 1] = obj;
			}
		} break;

		case OP_CREATE_ARRAY:
		{
			if(vm->debug)
				printf("create_array\n");
			++vm->pc;
			int length = (int)PopNumber(vm);
			PushArray(vm, length);
		} break;
		
		case OP_CREATE_ARRAY_BLOCK:
		{
			if(vm->debug)
				printf("create_array_block\n");
			++vm->pc;
			int length = ReadInteger(vm);
			Object* obj = PushArray(vm, length);
			if(length > 0)
			{
				for(int i = 0; i < length; ++i)
					obj->array.members[length - i - 1] = vm->stack[vm->stackSize - 2 - i];
				vm->stackSize -= length;
				vm->stack[vm->stackSize - 1] = obj;
			}
		} break;

		case OP_PUSH_STACK:
		{
			if(vm->debug)
				printf("push_stack\n");
			++vm->pc;
			vm->indirStack[vm->indirStackSize++] = vm->stackSize;
		} break;
		
		case OP_POP_STACK:
		{
			if(vm->debug)
				printf("pop_stack\n");
			++vm->pc;
			vm->stackSize = vm->indirStack[--vm->indirStackSize];
		} break;

		case OP_LENGTH:
		{
			if(vm->debug)
				printf("length\n");
			++vm->pc;
			Object* obj = PopObject(vm);
			if(obj->type == OBJ_STRING)
				PushNumber(vm, strlen(obj->string.raw));
			else if(obj->type == OBJ_ARRAY)
				PushNumber(vm, obj->array.length);
			else
				ErrorExit(vm, "Attempted to get length of %s\n", ObjectTypeNames[obj->type]);
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
			++vm->pc;
			Object* obj = PopArrayObject(vm);
			if(obj->array.length <= 0)
				ErrorExit(vm, "Cannot pop from empty array\n");
			
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

		case OP_DICT_SET:
		{
			++vm->pc;
			int keyIndex = ReadInteger(vm);
			
			if(vm->debug)
				printf("dict_set %s\n", vm->stringConstants[keyIndex]);
				
			Object* obj = PopDict(vm);
			Object* value = PopObject(vm);
			
			DictPut(&obj->dict, vm->stringConstants[keyIndex], value);
		} break;
		
		case OP_DICT_GET:
		{
			++vm->pc;
			int keyIndex = ReadInteger(vm);
			
			if(vm->debug)
				printf("dict_get %s\n", vm->stringConstants[keyIndex]);
				
			Object* obj = PopDict(vm);
			
			Object* value = DictGet(&obj->dict, vm->stringConstants[keyIndex]);
			if(value)
				PushObject(vm, value);
			else
				PushObject(vm, &NullObject);
		} break;

		case OP_DICT_PAIRS:
		{
			if(vm->debug)
				printf("dict_pairs\n");
			++vm->pc;
			Object* obj = PopDict(vm);
			Object* aobj = PushArray(vm, obj->dict.numEntries);
			
			int len = 0;
			for(int i = 0; i < obj->dict.capacity; ++i)
			{
				DictNode* node = obj->dict.buckets[i];
				while(node)
				{
					Object* pair = PushArray(vm, 2);
					
					Object* key = NewObject(vm, OBJ_STRING);
					key->string.raw = estrdup(node->key);
					
					pair->array.members[0] = key;
					pair->array.members[1] = node->value;
					
					aobj->array.members[len++] = PopObject(vm);
					
					node = node->next;
				}
			}
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
		BIN_OP_TYPE(LOGICAL_AND, &&, int)
		BIN_OP_TYPE(LOGICAL_OR, ||, int)
		BIN_OP_TYPE(SHL, >>, int)
		BIN_OP_TYPE(SHR, >>, int)
		
		/*#define CBIN_OP(op, operator) case OP_##op: { ++vm->pc; if(vm->debug) printf("%s\n", #op); Object* b = PopObject(vm); Object* a = PopObject(vm); a->number operator b->number; } break;
		
		CBIN_OP(CADD, +=)
		CBIN_OP(CSUB, -=)
		CBIN_OP(CMUL, *=)
		CBIN_OP(CDIV, /=)*/
		
		case OP_EQU:
		{
			++vm->pc;
			Object* o2 = PopObject(vm);
			Object* o1 = PopObject(vm);
			
			if(o1->type != o2->type) PushNumber(vm, 0);
			else
			{
				if(o1->type == OBJ_STRING) { PushNumber(vm, strcmp(o1->string.raw, o2->string.raw) == 0); }
				else if(o1->type == OBJ_NUMBER) { PushNumber(vm, o1->number == o2->number); }
				else PushNumber(vm, o1 == o2);
			}
		} break;
		
		case OP_NEQU:
		{
			++vm->pc;
			Object* o2 = PopObject(vm);
			Object* o1 = PopObject(vm);
			
			if(o1->type != o2->type) PushNumber(vm, 1);
			else
			{
				if(o1->type == OBJ_STRING) { PushNumber(vm, strcmp(o1->string.raw, o2->string.raw) != 0); }
				else if(o1->type == OBJ_NUMBER) { PushNumber(vm, o1->number != o2->number); }
				else PushNumber(vm, o1 != o2);
			}
		} break;
		
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

			Object* obj = PopObject(vm);
			Object* indexObj = PopObject(vm);
			Object* value = PopObject(vm);
			if(vm->debug)
				printf("setindex\n");
			
			if(obj->type == OBJ_ARRAY)
			{
				if(indexObj->type != OBJ_NUMBER)
					ErrorExit(vm, "Attempted to index array with a %s (expected number)\n", ObjectTypeNames[indexObj->type]);
				
				int index = (int)indexObj->number;
				
				int arrayLength = obj->array.length;
				Object** members = obj->array.members;
				
				if(index >= 0 && index < arrayLength)
					members[index] = value;
				else
					ErrorExit(vm, "Invalid array index %i\n", index);
			}
			else if(obj->type == OBJ_STRING)
			{				
				if(indexObj->type != OBJ_NUMBER)
					ErrorExit(vm, "Attempted to index string with a %s (expected number)\n", ObjectTypeNames[indexObj->type]);
				
				if(value->type != OBJ_NUMBER)
					ErrorExit(vm, "Attempted to assign a %s to an index of a string '%s' (expected number/character)\n", ObjectTypeNames[value->type], obj->string.raw);
				
				obj->string.raw[(int)indexObj->number] = (char)value->number;
			}
			else if(obj->type == OBJ_DICT)
			{
				if(indexObj->type != OBJ_STRING)
					ErrorExit(vm, "Attempted to index dict with a %s (expected string)\n", ObjectTypeNames[indexObj->type]);
				
				DictPut(&obj->dict, indexObj->string.raw, value);
			}
			else
				ErrorExit(vm, "Attempted to index a %s\n", ObjectTypeNames[obj->type]);
		} break;

		case OP_GETINDEX:
		{
			++vm->pc;

			Object* obj = PopObject(vm);
			Object* indexObj = PopObject(vm);

			if(obj->type == OBJ_ARRAY)
			{
				if(indexObj->type != OBJ_NUMBER)
					ErrorExit(vm, "Attempted to index array with a %s (expected number)\n", ObjectTypeNames[indexObj->type]);
				
				int index = (int)indexObj->number;
				
				int arrayLength = obj->array.length;
				Object** members = obj->array.members;
				
				if(index >= 0 && index < arrayLength)
				{
					if(members[index])
						PushObject(vm, members[index]);
					else
						PushObject(vm, &NullObject);
					if(vm->debug)
						printf("getindex %i\n", index);
				}
				else
					ErrorExit(vm, "Invalid array index %i\n", index);
			}
			else if(obj->type == OBJ_STRING)
			{
				if(indexObj->type != OBJ_NUMBER)
					ErrorExit(vm, "Attempted to index string with a %s (expected number)\n", ObjectTypeNames[indexObj->type]);
				
				PushNumber(vm, obj->string.raw[(int)indexObj->number]);
			}
			else if(obj->type == OBJ_DICT)
			{
				if(indexObj->type != OBJ_STRING)
					ErrorExit(vm, "Attempted to index dict with a %s (expected string)\n", ObjectTypeNames[indexObj->type]);
				
				Object* val = (Object*)DictGet(&obj->dict, indexObj->string.raw);
				if(val)
					PushObject(vm, val);
				else
					PushObject(vm, &NullObject);
			}
			else 
				ErrorExit(vm, "Attempted to index a %s\n", ObjectTypeNames[obj->type]);
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
			if(vm->stack[index])
				PushObject(vm, (vm->stack[index]));
			else
				PushObject(vm, &NullObject);
				
			if(vm->debug)
				printf("get %i\n", index);
		} break;
		
		case OP_WRITE:
		{
			if(vm->debug)
				printf("write\n");
			Object* top = PopObject(vm);
			WriteObject(vm, top);
			printf("\n");
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
			vm->lastFunctionName = vm->functionNames[index];
			
			PushIndir(vm, nargs);

			vm->pc = vm->functionPcs[index];
		} break;
		
		case OP_CALLP:
		{
			Word hasEllipsis, isExtern, numArgs;
			Word nargs = vm->program[++vm->pc];
			++vm->pc;
			int id = PopFunc(vm, &hasEllipsis, &isExtern, &numArgs);
			
			if(vm->debug)
				printf("callp %s%s\n", isExtern ? "extern " : "", isExtern ? vm->externNames[id] : vm->functionNames[id]);
			vm->lastFunctionName = isExtern ? vm->externNames[id] : vm->functionNames[id];
			
			if(isExtern)
				vm->externs[id](vm);
			else
			{
				if(!hasEllipsis)
				{
					if(nargs != numArgs)
						ErrorExit(vm, "Function '%s' expected %i args but recieved %i args\n", vm->functionNames[id], numArgs, nargs);
				}
				else
				{
					if(nargs < numArgs)
						ErrorExit(vm, "Function '%s' expected at least %i args but recieved %i args\n", vm->functionNames[id], numArgs, nargs);
				}
				
				if(!hasEllipsis) PushIndir(vm, nargs);
				else
				{
					// runtime collapsing of arguments:
					// the concrete arguments (known during compilation) are on the top of
					// the stack. We create an array (by pushing it and then decrementing the
					// stack pointer) and fill it up with the ellipsed arguments (behind the
					// concrete arguments). We then place this array just before the concrete
					// arguments in the stack so that it can be accessed as an argument "args".
					// The indirection info is pushed onto the indirection stack (the concrete and
					// non-concrete [aside from the one that the 'args' array replaces] are still 
					// present on the stack, so all of the arguments are to be removed)
					
					/*printf("args:\n");
					for(int i = 0; i < nargs; ++i)
					{
						WriteObject(vm, vm->stack[vm->stackSize - i - 1]);
						printf("\n");
					}
					printf("end\n");*/
					
					//printf("members:\n");
					Object* obj = PushArray(vm, nargs - numArgs);
					vm->stackSize -= 1;
					for(int i = 0; i < obj->array.length; ++i)
					{
						obj->array.members[i] = vm->stack[vm->stackSize - numArgs - 1 - i];
						/*WriteObject(vm, obj->array.members[i]);
						printf("\n");*/
					}
					//printf("end\n");
					
					vm->stack[vm->stackSize - numArgs - 1] = obj;
					
					/*printf("final args:\n");
					for(int i = 0; i < numArgs + 1; ++i)
					{
						WriteObject(vm, vm->stack[vm->stackSize - i - 1]);
						printf("\n");
					}
					printf("end\n");*/
					
					PushIndir(vm, nargs);
				}
				
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
		
		case OP_SETVMDEBUG:
		{
			if(vm->debug)
				printf("setvmdebug\n");
			char debug = vm->program[++vm->pc];
			++vm->pc;
			vm->debug = debug;
		} break;
		
		default:
			printf("Invalid instruction %i\n", vm->program[vm->pc]);
			break;
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
	if(vm->pc != -1) ErrorExit(vm, "Attempted to delete a running virtual machine\n");
	ResetVM(vm);
	free(vm);	
}
