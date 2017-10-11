#include <string.h>
#include <stdlib.h>

#include "utils.h"

void* emalloc(size_t size)
{
    void* mem = malloc(size);

    if(!mem) {
        fprintf(stderr, "Out of memory!");
        exit(1);
    }

    return mem;
}

void* erealloc(void* mem, size_t newSize)
{
    void* newMem = realloc(mem, newSize);

    if(!newMem) {
        fprintf(stderr, "Out of memory!");
        exit(1);
    }

    return newMem;
}

char* estrdup(const char* str)
{
    size_t len = strlen(str);

    char* newStr = emalloc(len + 1);
    strcpy(newStr, str);

    return newStr;
}

void ErrorExit(const char* format, ...)
{
	fprintf(stderr, "Error (%s:%i): ", FileName, LineNumber);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	
	exit(1);
}

void Warn(const char* format, ...)
{
	fprintf(stderr, "Warning (%s:%i): ", FileName, LineNumber);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
