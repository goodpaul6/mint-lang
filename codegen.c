#include "lang.h"

int EntryPoint = 0;

Word* Code = NULL;
int CodeCapacity = 0;
int CodeLength = 0;

void ClearCode()
{
	free(Code);
	Code = NULL;
	CodeCapacity = 0;
	CodeLength = 0;
}

void AppendCode(Word code)
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
}
void AppendInt(int value)
{
	Word* code = (Word*)(&value);
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		AppendCode(*code++);
}

void AllocatePatch(int length)
{
	for(int i = 0; i < length; ++i)
		AppendCode(0);
}

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
	int numGlobalsOutput = 0;
	for(int i = 0; i < DeepestScope; ++i)
	{
		for(VarDecl* decl = VarList; decl != NULL; decl = decl->next)
		{
			if(decl->isGlobal)
			{
				int len = strlen(decl->name);
				fwrite(&len, sizeof(int), 1, out);
				fwrite(decl->name, sizeof(char), len, out);
				++numGlobalsOutput;
			}
		}
	}

	if(numGlobalsOutput < NumGlobals)
		printf("Not all global names were written to the binary file. (%d out of %d)\n", numGlobalsOutput, NumGlobals);
	
	fwrite(&NumFunctions, sizeof(int), 1, out);	
	
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_NORMAL || decl->what == DECL_MACRO || decl->what == DECL_OPERATOR || decl->what == DECL_LAMBDA)
			fwrite(&decl->pc, sizeof(int), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_NORMAL || decl->what == DECL_MACRO || decl->what == DECL_OPERATOR || decl->what == DECL_LAMBDA)
			fwrite(&decl->hasEllipsis, sizeof(char), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_NORMAL || decl->what == DECL_MACRO || decl->what == DECL_OPERATOR || decl->what == DECL_LAMBDA)
			fwrite(&decl->numArgs, sizeof(Word), 1, out);
	}
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_NORMAL || decl->what == DECL_MACRO || decl->what == DECL_OPERATOR || decl->what == DECL_LAMBDA)
		{
			int len = strlen(decl->name);
			fwrite(&len, sizeof(int), 1, out);
			fwrite(decl->name, sizeof(char), len, out);
		}
	}
	
	fwrite(&NumExterns, sizeof(int), 1, out);	
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_EXTERN || decl->what == DECL_EXTERN_ALIASED)
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
}
