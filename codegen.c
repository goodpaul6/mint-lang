#include "lang.h"

int EntryPoint = 0;

Word* Code = NULL;
int CodeCapacity = NULL;
int CodeLength = NULL;

struct
{
	int* lineNumbers;
	int* fileNameIndices;
	int length;
	int capacity;
} Debug = { NULL, NULL, 0, 0 };

void _AppendCode(Word code, int line, int fileNameIndex)
{	
	while(CodeLength + 1 >= CodeCapacity)
	{
		if(!Code) CodeCapacity = 8;
		else CodeCapacity *= 2;
		
		void* newCode = realloc(Code, sizeof(Word) * CodeCapacity);
		assert(newCode);
		Code = newCode;
	}

	Code[CodeLength++] = code;
	
	while(Debug.length + 1 >= Debug.capacity)
	{
		if(Debug.capacity == 0) Debug.capacity = 8;
		else Debug.capacity *= 2;
		
		int* newLineNumbers = realloc(Debug.lineNumbers, sizeof(int) * Debug.capacity);
		assert(newLineNumbers);
		Debug.lineNumbers = newLineNumbers;
		
		int* newFileNameIndices = realloc(Debug.fileNameIndices, sizeof(int) * Debug.capacity);
		assert(newFileNameIndices);
		Debug.fileNameIndices = newFileNameIndices;
	}
	
	Debug.lineNumbers[Debug.length] = line;
	Debug.fileNameIndices[Debug.length] = fileNameIndex;
	Debug.length += 1;
}

#define AppendCode(code) _AppendCode((code), exp->line, RegisterString(exp->file)->index)

void _AppendInt(int value, int line, int file)
{
	Word* code = (Word*)(&value);
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		_AppendCode(*code++, line, file);
}

#define AppendInt(value) _AppendInt((value), exp->line, RegisterString(exp->file)->index)

void _AllocatePatch(int length, int line, int file)
{
	for(int i = 0; i < length; ++i)
		_AppendCode(0, line, file);
}

#define AllocatePatch(length) _AllocatePatch((length), exp->line, RegisterString(exp->file)->index)

void EmplaceInt(int loc, int value)
{
	if(loc >= CodeLength)
		ErrorExit("Compiler error: attempted to emplace in outside of code array\n");
		
	Word* code = (Word*)(&value);
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		Code[loc + i] = *code++;
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
void OutputCode(FILE* out)
{
	printf("=================================\n");
	printf("Summary:\n");
	printf("Entry point: %i\n", EntryPoint);
	printf("Program length: %i\n", CodeLength);
	printf("Number of global variables: %i\n", NumGlobals);
	printf("Number of functions: %i\n", NumFunctions);
	printf("Number of externs: %i\n", NumExterns);
	printf("Number of numbers: %i\n", NumNumbers);
	printf("Number of strings: %i\n", NumStrings);
	printf("=================================\n");
	
	
	fwrite(&EntryPoint, sizeof(int), 1, out);
	
	fwrite(&CodeLength, sizeof(int), 1, out);
	fwrite(Code, sizeof(Word), CodeLength, out);
	
	fwrite(&NumGlobals, sizeof(int), 1, out);
	for(VarDecl* decl = VarStack[0]; decl != NULL; decl = decl->next)
	{
		int len = strlen(decl->name);
		fwrite(&len, sizeof(int), 1, out);
		fwrite(decl->name, sizeof(char), len, out);
	}
	
	fwrite(&NumFunctions, sizeof(int), 1, out);	
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isExtern)
			fwrite(&decl->pc, sizeof(int), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isExtern)
			fwrite(&decl->hasEllipsis, sizeof(char), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isExtern)
			fwrite(&decl->numArgs, sizeof(Word), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isExtern)
		{
			int len = strlen(decl->name);
			fwrite(&len, sizeof(int), 1, out);
			fwrite(decl->name, sizeof(char), len, out);
		}
	}
	
	fwrite(&NumExterns, sizeof(int), 1, out);	
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->isExtern)
		{
			int len = strlen(decl->name);
			fwrite(&len, sizeof(int), 1, out);
			fwrite(decl->name, sizeof(char), len, out);
		}
	}
	
	fwrite(&NumNumbers, sizeof(int), 1, out);
	for(ConstDecl* decl = Constants; decl != NULL; decl = decl->next)
	{
		if(decl->type == CONST_NUM)
			fwrite(&decl->number, sizeof(double), 1, out);
	}
	
	fwrite(&NumStrings, sizeof(int), 1, out);
	for(ConstDecl* decl = Constants; decl != NULL; decl = decl->next)
	{
		if(decl->type == CONST_STR)
		{
			int len = strlen(decl->string);
			fwrite(&len, sizeof(int), 1, out);
			fwrite(decl->string, sizeof(char), len, out);
		}
	}
	
	char hasCodeMetadata = 1;
	fwrite(&hasCodeMetadata, sizeof(char), 1, out);
	fwrite(Debug.lineNumbers, sizeof(int), Debug.length, out);
	fwrite(Debug.fileNameIndices, sizeof(int), Debug.length, out);
}
