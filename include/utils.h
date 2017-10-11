#pragma once

#include <stddef.h>

void* emalloc(size_t size);
void* erealloc(void* mem, size_t newSize);
char* estrdup(const char* str);
