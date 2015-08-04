#include "vm.h"
#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <alloca.h>
#include <dlfcn.h>

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
	ReturnTop(vm);
}

void Std_Ceil(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, ceil(num));
	ReturnTop(vm);
}

void Std_Sin(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, sin(num));
	ReturnTop(vm);
}

void Std_Cos(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, cos(num));
	ReturnTop(vm);
}

void Std_Sqrt(VM* vm)
{
	double num = PopNumber(vm);
	PushNumber(vm, sqrt(num));
	ReturnTop(vm);
}

void Std_Atan2(VM* vm)
{
	double y = PopNumber(vm);
	double x = PopNumber(vm);
	
	PushNumber(vm, atan2(y, x));
	ReturnTop(vm);
}

void WriteObject(VM* vm, Object* top)
{
	if(top->type == OBJ_NUMBER)
		printf("%g", top->number);
	else if(top->type == OBJ_STRING)
		printf("%s", top->string.raw);
	else if(top->type == OBJ_NATIVE)
		printf("native pointer (0x%x)", (unsigned int)(intptr_t)(top->native.value));
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
	
	ReturnNullObject(vm);
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
	ReturnTop(vm);
}

void Std_Tonumber(VM* vm)
{
	Object* obj = PopObject(vm);
	
	if(obj->type == OBJ_STRING) PushNumber(vm, strtod(obj->string.raw, NULL));
	else if(obj->type == OBJ_NUMBER) PushObject(vm, obj);
	else PushNumber(vm, (intptr_t)(obj));
	ReturnTop(vm);
}

void Std_Tostring(VM* vm)
{
	Object* obj = PopObject(vm);
	char buf[128];
	switch(obj->type)
	{
		case OBJ_NULL: sprintf(buf, "null"); break;
		case OBJ_STRING: PushObject(vm, obj); return;
		case OBJ_NUMBER: sprintf(buf, "%g", obj->number); break;
		case OBJ_ARRAY: sprintf(buf, "array(%i)", obj->array.length); break;
		case OBJ_FUNC: sprintf(buf, "func %s", obj->func.isExtern ? vm->externNames[obj->func.index] : vm->functionNames[obj->func.index]); break;
		case OBJ_DICT: sprintf(buf, "dict(%i)", obj->dict.numEntries); break; 
		case OBJ_NATIVE: sprintf(buf, "native(%x)", (unsigned int)(intptr_t)(obj->native.value)); break;
	}
	
	PushString(vm, buf);
	ReturnTop(vm);
}

void Std_Type(VM* vm)
{
	Object* obj = PopObject(vm);
	PushString(vm, ObjectTypeNames[obj->type]);
	ReturnTop(vm);
}

void Std_Assert(VM* vm)
{
	int result = (int)PopNumber(vm);
	const char* message = PopString(vm);
	
	if(!result)
		ErrorExit(vm, "Assertion failed: %s\n", message);
	
	ReturnNullObject(vm);
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
	
	ReturnNullObject(vm);
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
	ReturnTop(vm);
}

void Std_Getc(VM* vm)
{
	FILE* file = PopNative(vm);
	PushNumber(vm, getc(file));
	ReturnTop(vm);
}

void Std_Putc(VM* vm)
{
	FILE* file = PopNative(vm);
	int c = (int)PopNumber(vm);
	putc(c, file);
	ReturnTop(vm);
}

void Std_Srand(VM* vm)
{
	srand((unsigned int)time(NULL));
}

void Std_Rand(VM* vm)
{
	PushNumber(vm, (int)rand());
	ReturnTop(vm);
}

void Std_Char(VM* vm)
{
	static char buf[2] = {0};
	buf[0] = (char)PopNumber(vm);
	PushString(vm, buf);
	ReturnTop(vm);
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
	ReturnTop(vm);
}

void Std_Clock(VM* vm)
{
	clock_t start = clock();
	PushNumber(vm, (double)start);
	ReturnTop(vm);
}

void Std_Clockspersec(VM* vm)
{
	PushNumber(vm, (double)CLOCKS_PER_SEC);
	ReturnTop(vm);
}

void Std_Halt(VM* vm)
{
	vm->pc = -1;
	ReturnNullObject(vm);
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
	ReturnTop(vm);
}

void Std_GetByte(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	size_t i = (size_t)PopNumber(vm);
	
	PushNumber(vm, ba->bytes[i]);
	ReturnTop(vm);
}

void Std_SetByte(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	size_t i = (size_t)PopNumber(vm);
	unsigned char value = (unsigned char)PopNumber(vm);
	ba->bytes[i] = value;
	ReturnTop(vm);
}

void Std_SetInt(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	int value = (int)PopNumber(vm);
	unsigned char* vp = (unsigned char*)(&value);
	
	for(int i = 0; i < sizeof(int) / sizeof(unsigned char); ++i)
		ba->bytes[i] = *vp++;
	ReturnNullObject(vm);
}

void Std_BytesLength(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	PushNumber(vm, ba->length);
	ReturnTop(vm);
}

enum
{
	NBA_U8,
	NBA_U16,
	NBA_U32,
	NBA_U64,
	NBA_S8,
	NBA_S16,
	NBA_S32,
	NBA_S64,
	NBA_FLOAT,
	NBA_DOUBLE,
	NBA_POINTER,
	NBA_VOID
};

void Std_NumberToBytes(VM* vm)
{
	double number = PopNumber(vm);
	int type = PopNumber(vm);
	
	ByteArray* ba = emalloc(sizeof(ByteArray));
	
	size_t length;
	switch(type)
	{
		case NBA_U8: length = sizeof(unsigned char); break;
		case NBA_U16: length = sizeof(unsigned short); break;
		case NBA_U32: length = sizeof(unsigned int); break;
		case NBA_U64: length = sizeof(unsigned long); break;
		
		case NBA_S8: length = sizeof(char); break;
		case NBA_S16: length = sizeof(short); break;
		case NBA_S32: length = sizeof(int); break;
		case NBA_S64: length = sizeof(long); break;
		
		case NBA_FLOAT: length = sizeof(float); break;
		case NBA_DOUBLE: length = sizeof(double); break;
		
		case NBA_POINTER: length = sizeof(void*); break; 
	}
	
	
	ba->length = length;
	ba->bytes = ecalloc(sizeof(unsigned char), ba->length);
	
	unsigned char u8n = (unsigned char)(number);
	unsigned short u16n = (unsigned short)(number);
	unsigned int u32n = (unsigned int)(number);
	unsigned long u64n = (unsigned long)(number);
	
	char s8n = (char)(number);
	short s16n = (short)(number);
	int s32n = (int)(number);
	long s64n = (long)(number);
	
	switch(type)
	{
		case NBA_U8: memcpy(ba->bytes, &u8n, sizeof(unsigned char)); break;
		case NBA_U16: memcpy(ba->bytes, &u16n, sizeof(unsigned short)); break;
		case NBA_U32: memcpy(ba->bytes, &u32n, sizeof(unsigned int)); break;
		case NBA_U64: memcpy(ba->bytes, &u64n, sizeof(unsigned long)); break;
		
		case NBA_S8: memcpy(ba->bytes, &s8n, sizeof(char)); break;
		case NBA_S16: memcpy(ba->bytes, &s16n, sizeof(short)); break;
		case NBA_S32: memcpy(ba->bytes, &s32n, sizeof(int)); break;
		case NBA_S64: memcpy(ba->bytes, &s64n, sizeof(long)); break;
		
		case NBA_FLOAT: memcpy(ba->bytes, &number, sizeof(float)); break;
		case NBA_DOUBLE: memcpy(ba->bytes, &number, sizeof(double)); break;
		
		case NBA_POINTER: memcpy(ba->bytes, &number, sizeof(void*)); break;
	}
	
	PushNative(vm, ba, Std_FreeBytes, NULL);
	ReturnTop(vm);
}

void Std_BytesToNumber(VM* vm)
{
	ByteArray* ba = PopNative(vm);
	int type = PopNumber(vm);
	
	double number = 0;
	
	switch(type)
	{
		case NBA_U8: number = *(unsigned char*)(ba->bytes);
		case NBA_U16: number = *(unsigned short*)(ba->bytes);
		case NBA_U32: number = *(unsigned int*)(ba->bytes);
		case NBA_U64: number = *(unsigned long*)(ba->bytes);
		
		case NBA_S8: number = *(char*)(ba->bytes);
		case NBA_S16: number = *(short*)(ba->bytes);
		case NBA_S32: number = *(int*)(ba->bytes);
		case NBA_S64: number = *(long*)(ba->bytes);
		
		case NBA_DOUBLE: number = *(double*)(ba->bytes);
	}
	
	PushNumber(vm, number);
	ReturnTop(vm);
}

void Std_GetFuncByName(VM* vm)
{
	const char* name = PopString(vm);
	
	int index = -1;
	for(int i = 0; i < vm->numFunctions; ++i)
	{
		if(strcmp(vm->functionNames[i], name) == 0)
			index = i;
	}
	
	if(index >= 0)
		PushFunc(vm, index, vm->functionHasEllipsis[index], 0, vm->functionNumArgs[index]);
	else
		PushObject(vm, &NullObject);
	ReturnTop(vm);
}

void Std_GetFuncName(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_FUNC)
		ErrorExit(vm, "extern 'getfuncname' expected a function pointer as its argument but received a %s\n", ObjectTypeNames[obj->type]);
	
	PushString(vm, vm->functionNames[obj->func.index]);
	ReturnTop(vm);
}

void Std_GetNumArgs(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_FUNC)
		ErrorExit(vm, "extern 'getnumargs' expected a function pointer as its argument but received a %s\n", ObjectTypeNames[obj->type]);
	
	PushNumber(vm, (int)obj->func.numArgs);
	ReturnTop(vm);
}

void Std_HasEllipsis(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_FUNC)
		ErrorExit(vm, "extern 'hasellipsis' expected a function pointer as its argument but received a %s\n", ObjectTypeNames[obj->type]);
		
	PushNumber(vm, (Word)obj->func.hasEllipsis);
	ReturnTop(vm);
}

void Std_StringHash(VM* vm)
{
	const char* string = PopString(vm);	
	PushNumber(vm, SuperFastHash(string, strlen(string)));
	ReturnTop(vm);
}

void Std_FreeLib(void* lib)
{
	dlclose(lib);
}

void Std_LoadLib(VM* vm)
{
	const char* libpath = PopString(vm);
	void* lib = dlopen(libpath, RTLD_LAZY);
	if(!lib)
		ErrorExit(vm, "Failed to load library '%s'\n", libpath);
	
	PushNative(vm, lib, Std_FreeLib, NULL);
	ReturnTop(vm);
}

void Std_GetProcAddress(VM* vm)
{
	void* lib = PopNative(vm);
	const char* name = PopString(vm);
	
	void* proc = dlsym(lib, name);
	if(!proc)
		ErrorExit(vm, "Failed to get proc '%s' address from library\n", name);
	
	PushNative(vm, proc, NULL, NULL);
	ReturnTop(vm);
}

void Std_Malloc(VM* vm)
{
	size_t size = (size_t)PopNumber(vm);
	void* mem = emalloc(size);
	
	PushNative(vm, mem, NULL, NULL);
	ReturnTop(vm);
}

void Std_Memcpy(VM* vm)
{
	void* dest = PopNative(vm);
	Object* srcobj = PopObject(vm);
	size_t size = (size_t)PopNumber(vm);
	
	if(srcobj->type == OBJ_NATIVE) memcpy(dest, srcobj->native.value, size);
	else if(srcobj->type == OBJ_NULL) memset(dest, 0, size);
	else ErrorExit(vm, "Invalid memcpy source parameter (%s instead of null or native)\n", ObjectTypeNames[srcobj->type]);
	ReturnNullObject(vm);
}

void Std_Free(VM* vm)
{
	void* mem = PopNative(vm);
	free(mem);
}

void Std_GetStructMember(VM* vm)
{
	char* addr = PopNative(vm);
	size_t offset = PopNumber(vm);
	int type = (int)PopNumber(vm);
	
	double number = 0;
	
	switch(type)
	{
		case NBA_U8: number = *(unsigned char*)(addr + offset); break;
		case NBA_U16: number = *(unsigned short*)(addr + offset); break;
		case NBA_U32: number = *(unsigned int*)(addr + offset); break;
		case NBA_U64: number = *(unsigned long*)(addr + offset); break;
		
		case NBA_S8: number = *(char*)(addr + offset); break;
		case NBA_S16: number = *(short*)(addr + offset); break;
		case NBA_S32: number = *(int*)(addr + offset); break;
		case NBA_S64: number = *(long*)(addr + offset); break;
		
		case NBA_FLOAT: number = *(float*)(addr + offset); break;
		case NBA_DOUBLE: number = *(double*)(addr + offset); break;
		
		case NBA_POINTER:
		{
			void* ptr = *(void**)(addr + offset);
			if(ptr)
				PushNative(vm, ptr, NULL, NULL);
			else
				PushObject(vm, &NullObject);
			ReturnTop(vm);
			return;
		} break;
	}
	
	PushNumber(vm, number);
	ReturnTop(vm);
}

void Std_SetStructMember(VM* vm)
{
	char* addr = PopNative(vm);
	size_t offset = PopNumber(vm);
	int type = PopNumber(vm);
	Object* obj = PopObject(vm);
	
	if(obj->type != OBJ_NUMBER && obj->type != OBJ_NATIVE && obj->type != OBJ_NULL)
		ErrorExit(vm, "'setstructmember' only accepts numbers, native pointers, or null as values\n");
	
	double number = obj->number;
	void* pointer = obj->native.value;
	
	unsigned char u8n = (unsigned char)(number);
	unsigned short u16n = (unsigned short)(number);
	unsigned int u32n = (unsigned int)(number);
	unsigned long u64n = (unsigned long)(number);
	
	char s8n = (char)(number);
	short s16n = (short)(number);
	int s32n = (int)(number);
	long s64n = (long)(number);
	
	float fn = (float)(number);
	
	switch(type)
	{
		case NBA_U8: memcpy(addr + offset, &u8n, sizeof(unsigned char)); break;
		case NBA_U16: memcpy(addr + offset, &u16n, sizeof(unsigned short)); break;
		case NBA_U32: memcpy(addr + offset, &u32n, sizeof(unsigned int)); break;
		case NBA_U64: memcpy(addr + offset, &u64n, sizeof(unsigned long)); break;
		
		case NBA_S8: memcpy(addr + offset, &s8n, sizeof(char)); break;
		case NBA_S16: memcpy(addr + offset, &s16n, sizeof(short)); break;
		case NBA_S32: memcpy(addr + offset, &s32n, sizeof(int)); break;
		case NBA_S64: memcpy(addr + offset, &s64n, sizeof(long)); break;
		
		case NBA_FLOAT: memcpy(addr + offset, &fn, sizeof(float)); break;
		case NBA_DOUBLE: memcpy(addr + offset, &number, sizeof(double)); break;
		
		case NBA_POINTER: 
		{
			if(obj->type != OBJ_NULL)
				memcpy(addr + offset, pointer, sizeof(void*)); 
			else
				memset(addr + offset, 0, sizeof(void*));
		} break;
	}
	ReturnNullObject(vm);
}

void Std_Sizeof(VM* vm)
{
	int type = PopNumber(vm);
	
	size_t size;
	switch(type)
	{
		case NBA_U8: size = sizeof(unsigned char); break;
		case NBA_U16: size = sizeof(unsigned short); break;
		case NBA_U32: size = sizeof(unsigned int); break;
		case NBA_U64: size = sizeof(unsigned long); break;
		
		case NBA_S8: size = sizeof(char); break;
		case NBA_S16: size = sizeof(short); break;
		case NBA_S32: size = sizeof(int); break;
		case NBA_S64: size = sizeof(long); break;
		
		case NBA_FLOAT: size = sizeof(float); break;
		case NBA_DOUBLE: size = sizeof(double); break;
		
		case NBA_POINTER: size = sizeof(void*); break;
		default:
			ErrorExit(vm, "Invalid type passed to sizeof");
	}
	
	PushNumber(vm, size);
	ReturnTop(vm);
}

void Std_Addressof(VM* vm)
{
	Object* obj = PopObject(vm);
	if(obj->type != OBJ_NATIVE)
		ErrorExit(vm, "'addressof' expected a native pointer but received a %s\n", ObjectTypeNames[obj->type]);
	
	PushNative(vm, &obj->native.value, NULL, NULL);
	ReturnTop(vm);
}

void Std_AtAddress(VM* vm)
{
	void* addr = PopNative(vm);
	int type = PopNumber(vm);
	
	double number = 0;
	
	switch(type)
	{
		case NBA_U8: number = *(unsigned char*)(addr); break;
		case NBA_U16: number = *(unsigned short*)(addr); break;
		case NBA_U32: number = *(unsigned int*)(addr); break;
		case NBA_U64: number = *(unsigned long*)(addr); break;
		
		case NBA_S8: number = *(char*)(addr); break;
		case NBA_S16: number = *(short*)(addr); break;
		case NBA_S32: number = *(int*)(addr); break;
		case NBA_S64: number = *(long*)(addr); break;
		
		case NBA_FLOAT: number = *(float*)(addr); break;
		case NBA_DOUBLE: number = *(double*)(addr); break;
		
		case NBA_POINTER:
		{
			void* ptr = *(void**)(addr);
			if(ptr)
				PushNative(vm, ptr, NULL, NULL);
			else
				PushObject(vm, &NullObject);
			ReturnTop(vm);
			return;
		} break;
	}
	
	PushNumber(vm, number);
	ReturnTop(vm);
}

void Std_ExternAddr(VM* vm)
{
	Object* ext = PopObject(vm);
	if(ext->type != OBJ_FUNC)
		ErrorExit(vm, "Expected func but received %s\n", ObjectTypeNames[ext->type]);
	
	if(!ext->func.isExtern)
		PushObject(vm, &NullObject);
	else
		PushNative(vm, (void*)vm->externs[ext->func.index], NULL, NULL);
	ReturnTop(vm);
}

void Std_FfiPrimType(VM* vm)
{
	int itype = PopNumber(vm);
	
	ffi_type* type;
	
	switch(itype)
	{
		case NBA_U8: type = &ffi_type_uchar; break;
		case NBA_U16: type = &ffi_type_ushort; break;
		case NBA_U32: type = &ffi_type_uint; break;
		case NBA_U64: type = &ffi_type_ulong; break;
		case NBA_S8: type = &ffi_type_schar; break;
		case NBA_S16: type = &ffi_type_sshort; break;
		case NBA_S32: type = &ffi_type_sint; break;
		case NBA_S64: type = &ffi_type_slong; break;
		case NBA_FLOAT: type = &ffi_type_float; break;
		case NBA_DOUBLE: type = &ffi_type_double; break;
		case NBA_POINTER: type = &ffi_type_pointer; break;
		case NBA_VOID: type = &ffi_type_void; break;
		
		default:
			ErrorExit(vm, "Invalid c type %i\n", itype);
	}
	
	PushNative(vm, type, NULL, NULL);
	ReturnTop(vm);
}

void Std_FfiCall(VM* vm)
{
	// TODO: don't call this for every call to ffi_call
	int ffi_result_id = GetGlobalId(vm, "ffi_result");
	if(ffi_result_id < 0)
		ErrorExit(vm, "'ffi_call' could not find global variable 'ffi_result', which is necessary for storing ffi_call results\n");
	
	void* funcptr = PopNative(vm);
	ffi_type* rtype = PopNative(vm);
	
	Object* types = PopObject(vm);
	if(types->type != OBJ_ARRAY)
		ErrorExit(vm, "Expected 3rd argument to 'ffi_call' to be an array but received %s\n", ObjectTypeNames[types->type]);
	unsigned int nargs = types->array.length;
	if(nargs >= MAX_CIF_ARGS)
		ErrorExit(vm, "Cannot pass more than %d arguments to foreign C function\n", MAX_CIF_ARGS);

	Object* args = PopObject(vm);
	if(args->type != OBJ_ARRAY)
		ErrorExit(vm, "Expected 4th argument to 'ffi_call' to be an array but received %s\n", ObjectTypeNames[args->type]);
	
	if(args->array.length != nargs)
		ErrorExit(vm, "Length of argument array does not match length of type array\n");
	
	ffi_type* argtypes[MAX_CIF_ARGS];
	for(int i = 0; i < nargs; ++i)
	{
		if(types->array.members[i]->type != OBJ_NATIVE)	
			ErrorExit(vm, "Invalid argument type in 'ffi_prep' argument type list\n");
		
		argtypes[i] = types->array.members[i]->native.value;
	}
	
	if(ffi_prep_cif(&vm->cif, FFI_DEFAULT_ABI, nargs, rtype, argtypes) == FFI_OK)
	{	
		vm->cifStackSize = rtype->size;
		vm->cifNumArgs = 0;
		memset(vm->cifValues, 0, sizeof(vm->cifValues));
		
		// PREPARE THE ARGUMENTS!!!
		for(int i = 0; i < nargs; ++i)
		{
			ffi_type* type = argtypes[i];
			void* value = &vm->cifStack[vm->cifStackSize];
			vm->cifStackSize += type->size;
			Object* obj = args->array.members[i];
			
			if(type != &ffi_type_pointer)
			{
				if(obj->type != OBJ_NUMBER)
					ErrorExit(vm, "ffi arg type value mismatch for argument %d (object type %s)\n", (i + 1), ObjectTypeNames[obj->type]);
				
				unsigned char ucn = (unsigned char)obj->number;
				unsigned short usn = (unsigned short)obj->number;
				unsigned int uin = (unsigned int)obj->number;
				unsigned long uln = (unsigned long)obj->number;
				
				char cn = (char)obj->number;
				short sn = (short)obj->number;
				int in = (int)obj->number;
				long ln = (long)obj->number;
			
				float fn = (float)obj->number;
			
				if(type == &ffi_type_uchar) memcpy(value, &ucn, type->size);
				else if(type == &ffi_type_ushort) memcpy(value, &usn, type->size);
				else if(type == &ffi_type_uint) memcpy(value, &uin, type->size);
				else if(type == &ffi_type_ulong) memcpy(value, &uln, type->size);
				else if(type == &ffi_type_schar) memcpy(value, &cn, type->size);
				else if(type == &ffi_type_sshort) memcpy(value, &sn, type->size);
				else if(type == &ffi_type_sint) memcpy(value, &in, type->size);
				else if(type == &ffi_type_slong) memcpy(value, &ln, type->size);
				else if(type == &ffi_type_float) memcpy(value, &fn, type->size);
				else if(type == &ffi_type_double) memcpy(value, &obj->number, type->size);
				else memset(value, 0, type->size);
			}
			else
			{
				if(obj->type != OBJ_NULL && obj->type != OBJ_NATIVE && obj->type != OBJ_STRING)
					ErrorExit(vm, "ffi arg type value mismatch for argument %d (object type %s)\n", (i + 1), ObjectTypeNames[obj->type]);
				
				if(obj->type == OBJ_NATIVE)
					memcpy(value, obj->native.value, type->size);
				else if(obj->type == OBJ_STRING)
					memcpy(value, &obj->string.raw, type->size);
				else
					memset(value, 0, type->size);
			}
			
			vm->cifValues[vm->cifNumArgs++] = value;
		}
		
		// call the function (actual return value is stored as the first value on the stack)
		ffi_call(&vm->cif, funcptr, &vm->cifStack[0], vm->cifValues);
		
		void* p = &vm->cifStack[0];
	
		if(rtype != &ffi_type_pointer)
		{
			double number;
			ffi_type* t = rtype;
			
			if(t == &ffi_type_uchar) number = *(unsigned char*)(p);
			else if(t == &ffi_type_ushort) number = *(unsigned short*)(p);
			else if(t == &ffi_type_uint) number = *(unsigned int*)(p);
			else if(t == &ffi_type_ulong) number = *(unsigned long*)(p);
			else if(t == &ffi_type_schar) number = *(char*)(p);
			else if(t == &ffi_type_sshort) number = *(short*)(p);
			else if(t == &ffi_type_sint) number = *(int*)(p);
			else if(t == &ffi_type_slong) number = *(long*)(p);
			else if(t == &ffi_type_float) number = *(float*)(p);
			else if(t == &ffi_type_double) number = *(double*)(p);
			else number = 0;
			
			PushNumber(vm, number);
			vm->stack[ffi_result_id] = PopObject(vm);
		}
		else
		{
			if(rtype != &ffi_type_void)
			{
				void* at_p = *(void**)(p);
				
				if(at_p)
				{
					PushNative(vm, at_p, NULL, NULL);
					vm->stack[ffi_result_id] = PopObject(vm);
				}
				else
					vm->stack[ffi_result_id] = &NullObject;
			}
		}
		
		// SUCCESSFUL CALL
		PushNumber(vm, 1);
		ReturnTop(vm);
	}
	else // failed to prep call, return 0
	{
		PushNumber(vm, 0);
		ReturnTop(vm);
	}
}

void Std_FreeFfiStructType(void* type)
{
	ffi_type* structType = type;
	free(structType->elements);
	free(type);
}

void Std_FfiStructTypeMark(void* type)
{
	ffi_type* structType = type;
	ffi_type** element = structType->elements;
	while((*element) != NULL)
	{
		if((*element)->type == FFI_TYPE_STRUCT)
			Std_FfiStructTypeMark(*element);
		++element;
	}
}

void Std_FfiStructType(VM* vm)
{
	Object* memberTypes = PopObject(vm);
	if(memberTypes->type != OBJ_ARRAY)
		ErrorExit(vm, "'ffi_struct_type' expected an array but received a %s\n", ObjectTypeNames[memberTypes->type]);
	if(memberTypes->array.length == 0)
		ErrorExit(vm, "empty member type array\n");

	ffi_type* structType = emalloc(sizeof(ffi_type));
	ffi_type** elements = emalloc(sizeof(ffi_type*) * (memberTypes->array.length + 1));
	
	structType->size = structType->alignment = 0;
	structType->elements = elements;
	structType->type = FFI_TYPE_STRUCT;
	
	for(int i = 0; i < memberTypes->array.length; ++i)
	{
		if(memberTypes->array.members[i]->type != OBJ_NATIVE)
			ErrorExit(vm, "invalid member type\n");
		elements[i] = memberTypes->array.members[i]->native.value;
	}
	elements[memberTypes->array.length] = NULL;
	
	ffi_type* argtypes[1] = { structType };
	
	if(ffi_prep_cif(&vm->cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, argtypes) == FFI_OK)
		PushNative(vm, structType, Std_FreeFfiStructType, Std_FfiStructTypeMark);
	else
		PushObject(vm, &NullObject);
	ReturnTop(vm);
}

void Std_FfiTypeSize(VM* vm)
{
	ffi_type* type = PopNative(vm);
	
	// TODO: Maybe add a static_number type object which is
	// a union of many c numerical values instead of casting everything
	// to doubles?
	PushNumber(vm, (double)type->size);
	ReturnTop(vm);
}

void Std_FfiStructTypeOffsets(VM* vm)
{
	// TODO: create a struct which stores type metadata 
	// behind the memory of the ffi_type so that it is 
	// possible to track the numelements of a type (and more)
	ffi_type* structType = PopNative(vm);
	int numElements = (int)PopNumber(vm);

	PushArray(vm, numElements);
	Object* obj = PopObject(vm);

	ffi_type** element = structType->elements;
	
	size_t offset = 0;
	
// TAKEN FROM LIBFFI SOURCE
#define ALIGN(v, a)  (((((size_t) (v))-1) | ((a)-1))+1)
	for(int i = 0; i < obj->array.length; ++i)
	{
		PushNumber(vm, offset);
		obj->array.members[i] = PopObject(vm);
		
		offset = ALIGN(offset, (*element)->alignment);
		offset += (*element)->size;
		++element;
	}
#undef ALIGN
	PushObject(vm, obj);
	ReturnTop(vm);
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
	vm->functionNames = NULL;
	vm->functionHasEllipsis = NULL;
	vm->functionPcs = NULL;
	vm->functionNumArgs = NULL;
	
	vm->externNames = NULL;
	vm->numExterns = 0;
	
	vm->lastFunctionName = NULL;
	vm->lastFunctionIndex = -1;

	vm->numExpandedArgs = 0;
	
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

	vm->inExternBody = 0;
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
		
	if(vm->functionHasEllipsis)
		free(vm->functionHasEllipsis);
	
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

	if(vm->externNames)
	{
		for(int i = 0; i < vm->numExterns; ++i)
			free(vm->externNames[i]);
		free(vm->externNames);
	}
	
	if(vm->externs)
		free(vm->externs);
	
	vm->stackSize = 0;
	CollectGarbage(vm);
	
	Object* obj = vm->freeHead;
	Object* next = NULL;
	
	while(obj)
	{
		next = obj->next;
		if(obj != &NullObject) free(obj);
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
whether function has ellipses as bytes (chars)
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
	
	if(numGlobals > 0)
	{
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
	}

	vm->numGlobals = numGlobals;
	vm->stackSize = numGlobals;
	vm->maxObjectsUntilGc += numGlobals;
		
	int numFunctions, numNumberConstants, numStringConstants;
	
	fread(&numFunctions, sizeof(int), 1, in);
	vm->numFunctions = numFunctions;
	
	if(numFunctions > 0)
	{
		vm->functionNames = emalloc(sizeof(char*) * numFunctions);
		vm->functionHasEllipsis = emalloc(sizeof(char) * numFunctions);
		vm->functionNumArgs = emalloc(sizeof(Word) * numFunctions);
		vm->functionPcs = emalloc(sizeof(int) * numFunctions);
		fread(vm->functionPcs, sizeof(int), numFunctions, in);
		fread(vm->functionHasEllipsis, sizeof(char), numFunctions, in);
		fread(vm->functionNumArgs, sizeof(Word), numFunctions, in);
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
	HookExternNoWarn(vm, "getfuncbyname", Std_GetFuncByName);
	HookExternNoWarn(vm, "getfuncname", Std_GetFuncName);
	HookExternNoWarn(vm, "getnumargs", Std_GetNumArgs);
	HookExternNoWarn(vm, "hasellipsis", Std_HasEllipsis);
	HookExternNoWarn(vm, "stringhash", Std_StringHash);
	HookExternNoWarn(vm, "number_to_bytes", Std_NumberToBytes);
	HookExternNoWarn(vm, "bytes_to_number", Std_BytesToNumber);
	HookExternNoWarn(vm, "loadlib", Std_LoadLib);
	HookExternNoWarn(vm, "getprocaddress", Std_GetProcAddress);
	
	HookExternNoWarn(vm, "malloc", Std_Malloc);
	HookExternNoWarn(vm, "memcpy", Std_Memcpy);
	HookExternNoWarn(vm, "free", Std_Free);
	HookExternNoWarn(vm, "getmem", Std_GetStructMember);
	HookExternNoWarn(vm, "setmem", Std_SetStructMember);
	HookExternNoWarn(vm, "sizeof", Std_Sizeof);
	HookExternNoWarn(vm, "addressof", Std_Addressof);
	HookExternNoWarn(vm, "ataddress", Std_AtAddress);
	HookExternNoWarn(vm, "externaddr", Std_ExternAddr);
	
	HookExternNoWarn(vm, "ffi_prim_type", Std_FfiPrimType);
	HookExternNoWarn(vm, "ffi_struct_type", Std_FfiStructType);
	HookExternNoWarn(vm, "ffi_struct_type_offsets", Std_FfiStructTypeOffsets);
	HookExternNoWarn(vm, "ffi_type_size", Std_FfiTypeSize);
	HookExternNoWarn(vm, "ffi_call", Std_FfiCall);
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

void FreeObject(VM* vm, Object* obj)
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
			FreeObject(vm, unreached);
			
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
	if(!vm->inExternBody && vm->numObjects == vm->maxObjectsUntilGc) 
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

void ReturnTop(VM* vm)
{
	Object* top = PopObject(vm);
	vm->retVal = top;
}

void ReturnNullObject(VM* vm)
{
	vm->retVal = &NullObject;
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

	vm->numExpandedArgs = 0;
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
	
	while(vm->fp > startFp && vm->pc >= 0)
		ExecuteCycle(vm);
}


int GetGlobalId(VM* vm, const char* name)
{
	for(int i = 0; i < vm->numGlobals; ++i)
	{
		if(strcmp(vm->globalNames[i], name) == 0)
			return vm->numGlobals - i - 1;
	}
	
	return -1;
}

Object* GetGlobal(VM* vm, int id)
{
	if(id < 0) return NULL;
	return vm->stack[id];
}

/* ALL OF THIS IS TERRIBLE; ABSOLUTELY HORRIBLE */
void CallOverloadedOperator(VM* vm, const char* name, Object* val1, Object* val2)
{
	if(vm->debug)
		printf("overload %s\n", name);
	if(val1->type != OBJ_DICT)																														
		ErrorExit(vm, "Invalid binary operation\n");																								
	Object* binFunc = DictGet(&val1->dict, name);
	if(!binFunc)
		ErrorExit(vm, "Attempted to perform binary operation with dictionary as lhs (and no operator overload) for op '%s'\n", name);
	if(binFunc->type != OBJ_FUNC)																													
		ErrorExit(vm, "Expected member '%s' in dictionary to be a function\n", name);									
	if(binFunc->func.numArgs != 2) ErrorExit(vm, "Expected member function '%s' in dictionary to take 2 arguments\n", name);
	vm->lastFunctionName = name;																	
	PushObject(vm, val2);			
	PushObject(vm, val1);
	CallFunction(vm, binFunc->func.index, 2);
}

char CallOverloadedOperatorIf(VM* vm, const char* name, Object* val1, Object* val2)
{
	if(vm->debug)
		printf("overload %s\n", name);
	if(val1->type != OBJ_DICT)																														
		ErrorExit(vm, "Invalid binary operation\n");																								
	Object* binFunc = DictGet(&val1->dict, name);
	if(!binFunc)
		return 0;
	if(binFunc->type != OBJ_FUNC)																													
		ErrorExit(vm, "Expected member '%s' in dictionary to be a function\n", name);									
	if(binFunc->func.numArgs != 2) ErrorExit(vm, "Expected member function '%s' in dictionary to take 2 arguments\n", name);
	vm->lastFunctionName = name;																	
	PushObject(vm, val2);			
	PushObject(vm, val1);
	CallFunction(vm, binFunc->func.index, 2);
	return 1;
}

char CallOverloadedOperatorEx(VM* vm, const char* name, Object* val1, Object* val2, Object* val3)
{
	if(vm->debug)
		printf("overload %s\n", name);
	if(val1->type != OBJ_DICT)																														
		ErrorExit(vm, "Invalid binary operation\n");																								
	Object* binFunc = DictGet(&val1->dict, name);
	if(!binFunc)
		return 0;
	if(binFunc->type != OBJ_FUNC)																													
		ErrorExit(vm, "Expected member '%s' in dictionary to be a function\n", name);									
	if(binFunc->func.numArgs != 3) ErrorExit(vm, "Expected member function '%s' in dictionary to take 3 arguments\n", name);
	vm->lastFunctionName = name;
	PushObject(vm, val3);
	PushObject(vm, val2);			
	PushObject(vm, val1);
	CallFunction(vm, binFunc->func.index, 3);
	return 1;
}
/* END OF HORRIBLENESS; FOR NOW :P
   VALVE PLS FIX */

void ExecuteCycle(VM* vm)
{
	if(vm->pc == -1) return;
	if(vm->debug)
	{
		if(vm->hasCodeMetadata)
			printf("(%s:%i:%i): ", vm->stringConstants[vm->pcFileTable[vm->pc]], vm->pcLineTable[vm->pc], vm->pc);
		else 
			printf("pc %i: ", vm->pc);
	}
	
	
	if(vm->stackSize < vm->numGlobals)
		printf("Global(s) were removed from the stack!\n");
	
	switch(vm->program[vm->pc])
	{
		case OP_GET_RETVAL:
		{
			if(vm->debug)
				printf("get_retval\n");
			++vm->pc;
			if(vm->retVal != NULL)
				PushObject(vm, vm->retVal);
			else
				PushObject(vm, &NullObject);
		} break;
		
		case OP_PUSH_NULL:
		{
			if(vm->debug)
				printf("push_null\n");
			++vm->pc;
			PushObject(vm, &NullObject);
		} break;
		
		case OP_PUSH_NUMBER:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			
			if(vm->debug)
				printf("push_number %g\n", vm->numberConstants[index]);
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

		/* REMOVED: see header
		 * case OP_CREATE_DICT_BLOCK:
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
		} break;*/

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

		case OP_EXPAND_ARRAY:
		{
			if(vm->debug)
				printf("expand_array\n");
			++vm->pc;
			Object* obj = PopObject(vm);
			if(obj->type != OBJ_ARRAY)
				ErrorExit(vm, "Expected array when expanding but received %s\n", ObjectTypeNames[obj->type]);
			int expand_amount = (int)PopNumber(vm);
			
			if(expand_amount < 0 || expand_amount > obj->array.length)
				ErrorExit(vm, "Expansion length out of array bounds\n");
			
			for(int i = expand_amount - 1; i >= 0; --i)
				PushObject(vm, obj->array.members[i]);
			
			vm->numExpandedArgs += expand_amount;
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
			else if(obj->type == OBJ_DICT)
			{
				Object* lenFunc = DictGet(&obj->dict, "LENGTH");
				if(!lenFunc)
					ErrorExit(vm, "Attempted to get length of dictionary without 'LENGTH' overload\n");
				
				PushObject(vm, obj);
				CallFunction(vm, lenFunc->func.index, 1);
			}
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
			
			/*Object* over = DictGet(&obj->dict, "GETINDEX");
			if(over && over->type == OBJ_FUNC)
			{
				PushObject(vm, value);
				PushString(vm, vm->stringConstants[keyIndex]);
				PushObject(vm, obj);
				CallFunction(vm, over->func.index, 3);
			}
			else*/	
			DictPut(&obj->dict, vm->stringConstants[keyIndex], value);
		} break;
		
		case OP_DICT_GET:
		{
			++vm->pc;
			int keyIndex = ReadInteger(vm);
			
			if(vm->debug)
				printf("dict_get %s\n", vm->stringConstants[keyIndex]);
				
			Object* obj = PopDict(vm);
			
			/*Object* over = DictGet(&obj->dict, "GETINDEX");
			if(over && over->type == OBJ_FUNC)
			{
				PushString(vm, vm->stringConstants[keyIndex]);
				PushObject(vm, obj);
				CallFunction(vm, over->func.index, 2);
			}
			else
			{
				Object* value = DictGet(&obj->dict, vm->stringConstants[keyIndex]);
				if(value)
					PushObject(vm, value);
				else
					PushObject(vm, &NullObject);
			}*/
		
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
		
		#define BIN_OP_TYPE(op, operator, ty) case OP_##op: { ++vm->pc; if(vm->debug) printf("%s\n", #op); Object* b = PopObject(vm); Object* a = PopObject(vm); { if(a->type != OBJ_NUMBER) CallOverloadedOperator(vm, #op, a, b); else if(b->type == OBJ_NUMBER) PushNumber(vm, (ty)a->number operator (ty)b->number); else ErrorExit(vm, "Invalid binary operation\n"); } } break;
		#define BIN_OP(op, operator) BIN_OP_TYPE(op, operator, double)
		
		BIN_OP(ADD, +)
		BIN_OP(SUB, -)
		BIN_OP(MUL, *)
		BIN_OP(DIV, /)
		BIN_OP_TYPE(MOD, %, long)
		BIN_OP_TYPE(OR, |, long)
		BIN_OP_TYPE(AND, &, long)
		BIN_OP(LT, <)
		BIN_OP(LTE, <=)
		BIN_OP(GT, >)
		BIN_OP(GTE, >=)
		BIN_OP_TYPE(LOGICAL_AND, &&, long)
		BIN_OP_TYPE(LOGICAL_OR, ||, long)
		BIN_OP_TYPE(SHL, <<, long)
		BIN_OP_TYPE(SHR, >>, long)
		
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
			if(vm->debug)
				printf("equ %s %s\n", ObjectTypeNames[o1->type], ObjectTypeNames[o2->type]);
			
			if(o1->type != o2->type && o1->type != OBJ_DICT) PushNumber(vm, 0);
			else
			{
				if(o1->type == OBJ_STRING) { PushNumber(vm, strcmp(o1->string.raw, o2->string.raw) == 0); }
				else if(o1->type == OBJ_NUMBER) { PushNumber(vm, o1->number == o2->number); }
				else if(o1->type == OBJ_DICT && CallOverloadedOperatorIf(vm, "EQUALS", o1, o2)) {}
				else if(o1->type == OBJ_NULL) PushNumber(vm, 1);
				else PushNumber(vm, o1 == o2);
			}
		} break;
		
		case OP_NEQU:
		{
			++vm->pc;
			Object* o2 = PopObject(vm);
			Object* o1 = PopObject(vm);
			
			if(vm->debug)
				printf("nequ %s %s\n", ObjectTypeNames[o1->type], ObjectTypeNames[o2->type]);
				
			if(o1->type != o2->type && o1->type != OBJ_DICT) PushNumber(vm, 1);
			else
			{
				if(o1->type == OBJ_STRING) { PushNumber(vm, strcmp(o1->string.raw, o2->string.raw) != 0); }
				else if(o1->type == OBJ_NUMBER) { PushNumber(vm, o1->number != o2->number); }
				else if(o1->type == OBJ_DICT && CallOverloadedOperatorIf(vm, "EQUALS", o1, o2)) { int result = (int)PopNumber(vm); PushNumber(vm, !result); }
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
				if(!CallOverloadedOperatorEx(vm, "SETINDEX", obj, indexObj, value))
				{	
					if(indexObj->type != OBJ_STRING)
						ErrorExit(vm, "Attempted to index dict with a %s (expected string)\n", ObjectTypeNames[indexObj->type]);
					DictPut(&obj->dict, indexObj->string.raw, value);
				}
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
				if(!CallOverloadedOperatorIf(vm, "GETINDEX", obj, indexObj))
				{
					if(indexObj->type != OBJ_STRING)
						ErrorExit(vm, "Attempted to index dict with a %s (expected string)\n", ObjectTypeNames[indexObj->type]);
					Object* val = (Object*)DictGet(&obj->dict, indexObj->string.raw);
					if(val)
						PushObject(vm, val);
					else
						PushObject(vm, &NullObject);
				}
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
				else if(top->type == OBJ_STRING) printf("set %i to %s\n", index, top->string.raw);	
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
			vm->lastFunctionIndex = index;
			
			if(!vm->functionHasEllipsis[index])
			{
				if(vm->functionNumArgs[index] != nargs + vm->numExpandedArgs)
					ErrorExit(vm, "Invalid number of arguments (%i) to function '%s' which expects %i arguments\n", nargs + vm->numExpandedArgs, vm->functionNames[index], vm->functionNumArgs[index]);
			}
			else
			{
				if(vm->functionNumArgs[index] > nargs + vm->numExpandedArgs)
					ErrorExit(vm, "Invalid number of arguments (%i) to function '%s' which expects at least %i arguments\n", nargs + vm->numExpandedArgs, vm->functionNames[index], vm->functionNumArgs[index]);
			}
			
			PushIndir(vm, nargs + vm->numExpandedArgs);

			vm->pc = vm->functionPcs[index];
		} break;
		
		case OP_CALLP:
		{
			int id;
			Word hasEllipsis, isExtern, numArgs;
			Word nargs = vm->program[++vm->pc];
			
			++vm->pc;
			
			Object* obj = PopObject(vm);
			
			if(obj->type == OBJ_DICT)
			{
				Object* fobj = DictGet(&obj->dict, "CALL");
				if(!fobj || fobj->type != OBJ_FUNC)
					ErrorExit(vm, "Attempted to call dictionary object without valid CALL overload\n");
				id = fobj->func.index;
				hasEllipsis = fobj->func.hasEllipsis;
				isExtern = fobj->func.isExtern;
				numArgs = fobj->func.numArgs;
				PushObject(vm, obj);
				nargs += 1;
			}
			else if(obj->type == OBJ_FUNC)
			{
				// TODO: this could be done better?
				PushObject(vm, obj);
				id = PopFunc(vm, &hasEllipsis, &isExtern, &numArgs);
			}
			else 
				ErrorExit(vm, "Expected function but received %s\n", ObjectTypeNames[obj->type]);
			
			if(vm->debug)
				printf("callp %s%s\n", isExtern ? "extern " : "", isExtern ? vm->externNames[id] : vm->functionNames[id]);
			vm->lastFunctionName = isExtern ? vm->externNames[id] : vm->functionNames[id];
			vm->lastFunctionIndex = id;
			
			nargs += vm->numExpandedArgs;
			
			if(isExtern)
			{
				vm->inExternBody = 1;
				vm->externs[id](vm);
				vm->inExternBody = 0;
			}
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
				
				PushIndir(vm, nargs);
				vm->pc = vm->functionPcs[id];
			}
		} break;
		
		case OP_RETURN:
		{
			if(vm->debug)
				printf("ret\n");
			PopIndir(vm);
			vm->retVal = NULL;
		} break;
		
		case OP_RETURN_VALUE:
		{
			if(vm->debug)
				printf("retval\n");
			Object* returnValue = PopObject(vm);
			PopIndir(vm);
			vm->retVal = returnValue;
		} break;
		
		case OP_CALLF:
		{
			++vm->pc;
			int index = ReadInteger(vm);
			if(vm->debug)
				printf("callf %s\n", vm->externNames[index]);
			vm->lastFunctionIndex = index;
			
			vm->inExternBody = 1;
			vm->externs[index](vm);
			vm->inExternBody = 0;
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
			++vm->pc;
			int index = ReadInteger(vm);
			if(vm->debug)
				printf("setlocal %i\n", index);
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
		
		case OP_DICT_GET_RKEY:
		{
			Object* obj = PopObject(vm);
			const char* key = PopString(vm);
			
			Object* val = (Object*)DictGet(&obj->dict, key);
			if(val)
				PushObject(vm, val);
			else
				PushObject(vm, &NullObject);
			++vm->pc;
		} break;
		
		case OP_DICT_SET_RKEY:
		{
			Object* obj = PopObject(vm);
			const char* key = PopString(vm);
			Object* value = PopObject(vm);
			
			DictPut(&obj->dict, key, value);
			++vm->pc;
		} break;
		
		case OP_GETARGS:
		{
			++vm->pc;
			int startArgIndex = -ReadInteger(vm) - 1;
			
			Word nargs = vm->indirStack[vm->indirStackSize - 3]; // number of arguments passed to the function
			
			if(nargs == 0) PushArray(vm, 0);
			else
			{
				Object* obj = PushArray(vm, nargs - startArgIndex);
				
				for(int i = startArgIndex; i < nargs; ++i)
					obj->array.members[i - startArgIndex] = GetLocal(vm, -i - 1);
			}
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
