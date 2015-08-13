#include "lang.h"

/* BINARY FORMAT:
entry point as integer

program length (in words) as integer
program code (must be no longer than program length specified previously)

number of global variables as integer
names of global variables as string lengths followed by characters

number of functions as integer
function entry points as integers
whether function has ellipsis as chars
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

char** ReadStringArrayFromBinaryFile(FILE* in, int amount)
{
	char** strings = malloc(sizeof(char*) * amount);
	assert(strings);
	
	for(int i = 0; i < amount; ++i)
	{
		int length;
		fread(&length, sizeof(int), 1, in);
		char* str = malloc(length + 1);
		fread(str, sizeof(char), length, in);
		str[length] = '\0';
		strings[i] = str;
	}
	
	return strings;
}

void FreeStringArray(char** array, int length)
{
	for(int i = 0; i < length; ++i)
		free(array[i]);
	free(array);
}

void RetargetIndex(int* indexLocation, int* indexTable, int length)
{
	int previousValue = *indexLocation;
	if(previousValue >= length || previousValue < 0)
		ErrorExit("invalid previous value when retargeting (%i)\n", previousValue);
	*indexLocation = indexTable[previousValue];
}


/*Expr* ParseBinaryFile(FILE* bin, const char* fileName)
{
	Expr* exp = CreateExpr(EXP_LINKED_BINARY_CODE);
	
	int entryPoint;
	fread(&entryPoint, sizeof(int), 1, bin);
	
	int programLength;
	fread(&programLength, sizeof(int), 1, bin);
	
	Word* program = malloc(sizeof(Word) * programLength);
	fread(program, sizeof(Word), programLength, bin);
	
	int numGlobals;
	fread(&numGlobals, sizeof(int), 1, bin);
	
	int* globalIndexTable = NULL;
	if(numGlobals > 0)
	{
		// this table is for retargeting global accesses in the binary file to the binary file produced in this execution of the compiler
		globalIndexTable = malloc(sizeof(int) * numGlobals);
		assert(globalIndexTable);
		
		char** globalNames = ReadStringArrayFromBinaryFile(bin, numGlobals);
		for(int i = 0; i < numGlobals; ++i)
		{
			VarDecl* decl = RegisterVariable(globalNames[i]);
			globalIndexTable[i] = decl->index;
		}
		FreeStringArray(globalNames, numGlobals);
	}
	
	int numFunctions;
	fread(&numFunctions, sizeof(int), 1, bin);
	
	exp->code.numFunctions = numFunctions;
	
	int* functionIndexTable = NULL;
	if(numFunctions > 0)
	{
		functionIndexTable = malloc(sizeof(int) * numFunctions);
		assert(functionIndexTable);
		
		int* functionPcs = malloc(sizeof(int) * numFunctions);
		assert(functionPcs);
		
		fread(functionPcs, sizeof(int), numFunctions, bin);
		
		char* functionHasEllipsis = malloc(sizeof(char) * numFunctions);
		assert(functionHasEllipsis);
		
		fread(functionHasEllipsis, sizeof(char), numFunctions, bin);
		
		int* functionNumArgs = malloc(sizeof(int) * numFunctions);
		assert(functionNumArgs);
		
		fread(functionNumArgs, sizeof(int), numFunctions, bin);
		
		char** functionNames = ReadStringArrayFromBinaryFile(bin, numFunctions);
		
		exp->code.toBeRetargeted = malloc(sizeof(FuncDecl*) * numFunctions);
		for(int i = 0; i < numFunctions; ++i)
		{
			FuncDecl* decl = DeclareFunction(functionNames[i], NumFunctions++);
			decl->pc = functionPcs[i];
			decl->numArgs = functionNumArgs[i];
			decl->hasEllipsis = functionHasEllipsis[i];
			exp->code.toBeRetargeted[i] = decl;
			functionIndexTable[i] = decl->index;
		}
		FreeStringArray(functionNames, numFunctions);
		free(functionPcs);
		free(functionHasEllipsis);
		free(functionNumArgs);
	}
	
	int numExterns;
	fread(&numExterns, sizeof(int), 1, bin);
	
	int* externIndexTable = NULL;
	if(numExterns > 0)
	{
		externIndexTable = malloc(sizeof(int) * numExterns);
		assert(externIndexTable);
		
		char** externNames = ReadStringArrayFromBinaryFile(bin, numExterns);
		for(int i = 0; i < numExterns; ++i)
			externIndexTable[i] = DeclareExtern(externNames[i])->index;
		FreeStringArray(externNames, numExterns);
	}
	
	
	int numNumberConstants;
	fread(&numNumberConstants, sizeof(int), 1, bin);
	
	int* numberIndexTable = NULL;
	
	if(numNumberConstants > 0)
	{
		numberIndexTable = malloc(sizeof(int) * numNumberConstants);
		assert(numberIndexTable);
		
		double* numberConstants = malloc(sizeof(double) * numNumberConstants);
		assert(numberConstants);
		
		fread(numberConstants, sizeof(double), numNumberConstants, bin);
		
		for(int i = 0; i < numNumberConstants; ++i)
			numberIndexTable[i] = RegisterNumber(numberConstants[i])->index;
		
		free(numberConstants);
	}
	
	int numStringConstants;
	fread(&numStringConstants, sizeof(int), 1, bin);
	
	int* stringIndexTable = NULL;
	
	if(numStringConstants > 0)
	{
		stringIndexTable = malloc(sizeof(int) * numStringConstants);
		assert(stringIndexTable);
		
		char** stringConstants = ReadStringArrayFromBinaryFile(bin, numStringConstants);
		for(int i = 0; i < numStringConstants; ++i)
			stringIndexTable[i] = RegisterString(stringConstants[i])->index;
		FreeStringArray(stringConstants, numStringConstants);
	}
	
	for(int i = 0; i < programLength; ++i)
	{
		switch(program[i])
		{
			case OP_PUSH_NULL: break;
			
			 case OP_PUSH_NUMBER:
			{
				i += 1;
				RetargetIndex((int*)(&program[i]), numberIndexTable, numNumberConstants);
				i += sizeof(int) / sizeof(Word) - 1;
			} break;
			
			case OP_PUSH_STRING:
			{
				i += 1;
				RetargetIndex((int*)(&program[i]), stringIndexTable, numStringConstants);
				i += sizeof(int) / sizeof(Word) - 1;
			} break;
			
			case OP_CREATE_ARRAY: break;
			case OP_CREATE_ARRAY_BLOCK: i += sizeof(int) / sizeof(Word); break;
			
			case OP_PUSH_FUNC: 
			{
				i += 4;
				RetargetIndex((int*)(&program[i]), functionIndexTable, numFunctions);
				i += sizeof(int) / sizeof(Word) - 1;
			} break;
			
			case OP_EXPAND_ARRAY: break;
			
			case OP_PUSH_DICT: break;
			//REMOVED: case OP_CREATE_DICT_BLOCK: i += sizeof(int) / sizeof(Word); break;
			
			// stores current stack size
			case OP_PUSH_STACK: break;
			// returns to previously stored stack size
			case OP_POP_STACK: break;
			
			case OP_LENGTH: break;
			case OP_ARRAY_PUSH: break;
			case OP_ARRAY_POP: break;
			case OP_ARRAY_CLEAR: break;
			
			// fast dictionary operations
			case OP_DICT_SET:
			case OP_DICT_GET:
			{
				i += 1;
				RetargetIndex((int*)(&program[i]), stringIndexTable, numStringConstants); 
				i += sizeof(int) / sizeof(Word) - 1; 
			} break;
			
			case OP_DICT_PAIRS: break;
			
			case OP_ADD:
			case OP_SUB:
			case OP_MUL:
			case OP_DIV:
			case OP_MOD:
			case OP_OR:
			case OP_AND:
			case OP_LT:
			case OP_LTE:
			case OP_GT:
			case OP_GTE:
			case OP_EQU:
			case OP_NEQU:
			case OP_NEG:
			case OP_LOGICAL_NOT:
			case OP_LOGICAL_AND:
			case OP_LOGICAL_OR:
			case OP_SHL:
			case OP_SHR: break;
			 
			case OP_SETINDEX: 
			case OP_GETINDEX: i += sizeof(int) / sizeof(Word); break;
			 
			case OP_SET:
			case OP_GET:
			{
				++i;
				RetargetIndex((int*)(&program[i]), globalIndexTable, numGlobals);
				i += sizeof(int) / sizeof(Word) - 1;
			} break; 
			
			case OP_WRITE: break;
			case OP_READ: break;
			 
			case OP_GOTO: i += sizeof(int) / sizeof(Word); break;
			case OP_GOTOZ: i += sizeof(int) / sizeof(Word); break;
			 
			case OP_CALL:
			{
				i += 2;
				RetargetIndex((int*)(&program[i]), functionIndexTable, numFunctions);
				i += sizeof(int) / sizeof(Word) - 1;
			} break;
			
			case OP_CALLP: i += 1; break;
			case OP_RETURN: break;
			case OP_RETURN_VALUE: break;
			 
			case OP_CALLF:
			{
				i += 1;
				RetargetIndex((int*)(&program[i]), externIndexTable, numFunctions);
				i += sizeof(int) / sizeof(Word) - 1;
			} break;
			 
			case OP_GETLOCAL:
			case OP_SETLOCAL: i += sizeof(int) / sizeof(Word); break;
			 
			case OP_HALT: break;
			
			case OP_SETVMDEBUG: i += 1; break;
			
			case OP_DICT_GET_RKEY: case OP_DICT_SET_RKEY: break;
			case OP_GETARGS: i += sizeof(int) / sizeof(Word); break;
		}
	}
	
	if(globalIndexTable) free(globalIndexTable);
	if(functionIndexTable) free(functionIndexTable);
	if(externIndexTable) free(externIndexTable);
	if(numberIndexTable) free(numberIndexTable);
	if(stringIndexTable) free(stringIndexTable);
	
	exp->code.bytes = program;
	exp->code.length = programLength;
	exp->line = -1;
	exp->file = fileName;
	
	return exp;
}*/

