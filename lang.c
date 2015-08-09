// lang.c -- a language which compiles to mint vm bytecode
/* 
 * TODO:
 * - FIX compiler structure, have a function to compile expressions which should have resulting values
 * 	 and a function to compile expressions which shouldn't
 * - (Possibly fixed, haven't tested new compile time dict creation) WEIRD BUG: Dict literals occasionally cause ffi_calls after their creation to fail? (Depends on # of elements in dict, idk, maybe dict creation pollutes the stack)
 * - the offset passed to getstruct/setstruct memmber should be a native object, not a number
 * - Finish meta api
 * - Safe extern calls (preserve stack size before and after, unless value is returned)
 * - Should function declarations require a 'return' modifier if they return values (so the programmer knows that they have to return values no matter what)
 *   or you could do compile-time checks to see if a function doesn't return (this would be difficult cause of control flow, etc)
 * - BUG - vm fails to load code if there is no _main function
 * - expand could leak values onto the stack if used incorrectly (but then again, that's not the job of the compiler to check, right?)
 * - MORE CONST OMG (CompileExpr doesn't [read: shouldn't] change the expressions)
 * - enums
 * 
 * REQUIRED FIXES:
 * - parenthesized function calls will pollute the stack if they aren't assigned (see FIX compiler structure for a possible fix)
 * 
 * IMPROVEMENTS:
 * - interpret intrinsic for compile time interpretation of simple expressions (addition, subtraction, etc)
 * - op_dict_*_rkey: maybe write a version of these which doesn't use a runtime key
 * 
 * PARTIALLY COMPLETE (I.E NOT FULLY SATISFIED WITH SOLUTION):
 * - extern aliasing (functionality is there but it's broken and doesn't compile)
 * - using compound binary operators causes an error
 * - include/require other scripts (copy bytecode into vm at runtime?): can only include things at compile time (with cmd line)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "lang.h"
int LineNumber = 0;
const char* FileName = NULL;

int NumNumbers = 0;
int NumStrings = 0;

int NumGlobals = 0;
int NumFunctions = 0;
int NumExterns = 0;
int EntryPoint = 0;

Word* Code = NULL;
int CodeCapacity = 0;
int CodeLength = 0;

struct
{
	int* lineNumbers;
	int* fileNameIndices;
	int length;
	int capacity;
} Debug = { NULL, NULL, 0, 0 };

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


ConstDecl* Constants = NULL;
ConstDecl* ConstantsCurrent = NULL;

FuncDecl* CurFunc = NULL;

FuncDecl* Functions = NULL;
FuncDecl* FunctionsCurrent = NULL;

VarDecl* VarStack[MAX_SCOPES];
int VarStackDepth = 0;

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

ConstDecl* RegisterNumber(double number)
{
	for(ConstDecl* decl = Constants; decl != NULL; decl = decl->next)
	{
		if(decl->type == CONST_NUM)
		{
			if(decl->number == number)
				return decl;
		}
	}
	
	ConstDecl* decl = malloc(sizeof(ConstDecl));
	assert(decl);
	
	decl->next = NULL;
	
	if(!Constants)
	{
		Constants = decl;
		ConstantsCurrent = Constants;
	}
	else
	{
		ConstantsCurrent->next = decl;
		ConstantsCurrent = decl;
	}

	decl->type = CONST_NUM;
	decl->number = number;
	decl->index = NumNumbers++;	
	
	return decl;
}

ConstDecl* RegisterString(const char* string)
{
	for(ConstDecl* decl = Constants; decl != NULL; decl = decl->next)
	{
		if(decl->type == CONST_STR)
		{
			if(strcmp(decl->string, string) == 0)
				return decl;
		}
	}
	
	ConstDecl* decl = malloc(sizeof(ConstDecl));
	assert(decl);
	
	decl->next = NULL;
	
	if(!Constants)
	{
		Constants = decl;
		ConstantsCurrent = Constants;
	}
	else
	{
		ConstantsCurrent->next = decl;
		ConstantsCurrent = decl;
	}

	decl->type = CONST_STR;
	decl->string = malloc(strlen(string) + 1);
	assert(decl->string);
	strcpy(decl->string, string);
	decl->index = NumStrings++;
	
	return decl;
}

FuncDecl* RegisterExtern(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isAliased)
		{
			if(strcmp(decl->name, name) == 0)
				return decl;
		}
		else
		{
			if(strcmp(decl->alias, name) == 0)
				return decl;
		}
	}
	
	FuncDecl* decl = malloc(sizeof(FuncDecl));
	assert(decl);
	
	decl->next = NULL;
	
	if(!Functions)
	{
		Functions = decl;
		FunctionsCurrent = Functions;
	}
	else
	{
		FunctionsCurrent->next = decl;
		FunctionsCurrent = decl;
	}
	
	decl->inlined = 0;
	decl->isAliased = 0;
	decl->isExtern = 1;
	
	strcpy(decl->name, name);
	
	decl->index = NumExterns++;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	decl->hasEllipsis = 0;
	decl->hasReturn = -1;
	
	decl->returnType = NULL;
	decl->argTypes = NULL;
	
	return decl;
}

FuncDecl* RegisterExternAs(const char* name, const char* alias)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isAliased)
		{
			if(strcmp(decl->name, name) == 0)
			{
				decl->isAliased = 1;
				strcpy(decl->alias, alias);
				return decl;
			}
		}
		else
		{
			strcpy(decl->alias, alias);
			return decl;
		}
	}
	
	FuncDecl* decl = malloc(sizeof(FuncDecl));
	assert(decl);
	
	decl->next = NULL;
	
	if(!Functions)
	{
		Functions = decl;
		FunctionsCurrent = Functions;
	}
	else
	{
		FunctionsCurrent->next = decl;
		FunctionsCurrent = decl;
	}
	
	decl->inlined = 0;
	decl->isAliased = 1;
	decl->isExtern = 1;
	
	strcpy(decl->name, name);
	strcpy(decl->alias, alias);
	
	decl->index = NumExterns++;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	decl->hasEllipsis = 0;
	
	decl->returnType = NULL;
	decl->argTypes = NULL;
	
	return decl;
}

void PushScope()
{
	VarStack[++VarStackDepth] = NULL;
}

void PopScope()
{
	--VarStackDepth;
}

FuncDecl* DeclareFunction(const char* name)
{
	FuncDecl* decl = malloc(sizeof(FuncDecl));
	assert(decl);

	decl->next = NULL;

	if(!Functions)
	{
		Functions = decl;
		FunctionsCurrent = Functions;
	}
	else
	{
		FunctionsCurrent->next = decl;
		FunctionsCurrent = decl;
	}
	
	decl->argTypes = NULL;
	decl->returnType = NULL;
	
	decl->isAliased = 0;
	decl->isExtern = 0;
	decl->hasEllipsis = 0;
	
	strcpy(decl->name, name);
	decl->index = NumFunctions++;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	
	decl->hasReturn = -1;

	decl->isLambda = 0;
	decl->prevDecl = NULL;
	decl->upvalues = NULL;
	decl->envDecl = NULL;
	decl->scope = VarStackDepth;
	
	return decl;
}

FuncDecl* EnterFunction(const char* name)
{
	CurFunc = DeclareFunction(name);	
	return CurFunc;
}

void ExitFunction()
{
	CurFunc = NULL;
}

FuncDecl* ReferenceFunction(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isAliased)
		{
			if(strcmp(decl->name, name) == 0)
				return decl;
		}
		else
		{
			if(strcmp(decl->alias, name) == 0)
				return decl;
		}
	}
	
	return NULL;
}

VarDecl* RegisterVariable(const char* name)
{	
	VarDecl* decl = malloc(sizeof(VarDecl));
	assert(decl);
	
	decl->next = VarStack[VarStackDepth];
	VarStack[VarStackDepth] = decl;
	
	decl->isGlobal = 0;
		
	strcpy(decl->name, name);
	if(!CurFunc)
	{	
		decl->index = NumGlobals++;
		decl->isGlobal = 1;
	}
	else
		decl->index = CurFunc->numLocals++;
	
	decl->scope = VarStackDepth;
	
	decl->type = NULL;
	
	return decl;
}

VarDecl* RegisterArgument(const char* name)
{
	if(!CurFunc)
		ErrorExit("(INTERNAL) Must be inside function when registering arguments\n");
	
	VarDecl* decl = RegisterVariable(name);
	--CurFunc->numLocals;
	decl->index = -(++CurFunc->numArgs);
	
	return decl;
}

VarDecl* ReferenceVariable(const char* name)
{
	for(int i = VarStackDepth; i >= 0; --i)
	{
		for(VarDecl* decl = VarStack[i]; decl != NULL; decl = decl->next)
		{
			if(strcmp(decl->name, name) == 0)
				return decl;
		}
	}
	
	return NULL;
}

size_t LexemeCapacity = 0;
size_t LexemeLength = 0;
char* Lexeme = NULL;
int CurTok = 0;
char ResetLex = 0;

void AppendLexChar(int c)
{
	if(!LexemeCapacity)
	{
		LexemeCapacity = 8;
		Lexeme = malloc(LexemeCapacity);
		assert(Lexeme);
	}
	
	if(LexemeLength + 1 >= LexemeCapacity)
	{
		LexemeCapacity *= 2;
		char* newLex = realloc(Lexeme, LexemeCapacity);
		assert(newLex);
		Lexeme = newLex;
	}
	
	Lexeme[LexemeLength++] = c;
}

void ClearLexeme()
{
	LexemeLength = 0;
}

int GetToken(FILE* in)
{
	static char last = ' ';
	
	if(ResetLex)
	{
		last = ' ';
		ResetLex = 0;
		ClearLexeme();
	}
	
	while(isspace(last))
	{
		if(last == '\n') ++LineNumber;
		last = getc(in);
	}
	
	if(isalpha(last) || last == '_')
	{
		ClearLexeme();
		while(isalnum(last) || last == '_')
		{
			AppendLexChar(last);
			last = getc(in);
		}
		AppendLexChar('\0');
		
		if(strcmp(Lexeme, "var") == 0) return TOK_VAR;
		if(strcmp(Lexeme, "while") == 0) return TOK_WHILE;
		if(strcmp(Lexeme, "end") == 0) return TOK_END;
		if(strcmp(Lexeme, "func") == 0) return TOK_FUNC;
		if(strcmp(Lexeme, "if") == 0) return TOK_IF;
		if(strcmp(Lexeme, "return") == 0) return TOK_RETURN;
		if(strcmp(Lexeme, "extern") == 0) return TOK_EXTERN;
		
		// NOTE: these assume that the lexeme capacity is greater than or equal to 2
		if(strcmp(Lexeme, "true") == 0)
		{
			Lexeme[0] = '1';
			Lexeme[1] = '\0';
			return TOK_NUMBER;
		}
		if(strcmp(Lexeme, "false") == 0)
		{
			Lexeme[0] = '0';
			Lexeme[1] = '\0';
			return TOK_NUMBER;
		}
		
		if(strcmp(Lexeme, "for") == 0) return TOK_FOR;
		if(strcmp(Lexeme, "else") == 0) return TOK_ELSE;
		if(strcmp(Lexeme, "elif") == 0) return TOK_ELIF;
		if(strcmp(Lexeme, "null") == 0) return TOK_NULL;
		if(strcmp(Lexeme, "inline") == 0) return TOK_INLINE;
		if(strcmp(Lexeme, "lambda") == 0) return TOK_LAMBDA;
		if(strcmp(Lexeme, "continue") == 0) return TOK_CONTINUE;
		if(strcmp(Lexeme, "break") == 0) return TOK_BREAK;
		if(strcmp(Lexeme, "new") == 0) return TOK_NEW;
		if(strcmp(Lexeme, "as") == 0) return TOK_AS;
		
		return TOK_IDENT;
	}
		
	if(isdigit(last))
	{	
		ClearLexeme();
		while(isdigit(last) || last == '.')
		{
			AppendLexChar(last);
			last = getc(in);
		}
		AppendLexChar('\0');
		
		return TOK_NUMBER;
	}

	if(last == '$')
	{
		last = getc(in);
		ClearLexeme();
		while(isxdigit(last))
		{
			AppendLexChar(last);
			last = getc(in);
		}
		AppendLexChar('\0');
		
		return TOK_HEXNUM;
	}
	
	if(last == '"')
	{
		ClearLexeme();
		last = getc(in);
	
		while(last != '"')
		{
			if(last == '\\')
			{
				last = getc(in);
				switch(last)
				{
					case 'n': last = '\n'; break;
					case 'r': last = '\r'; break;
					case 't': last = '\t'; break;
					case '"': last = '"'; break;
					case '\'': last = '\''; break;
					case '\\': last = '\\'; break;
				}
			}
			
			AppendLexChar(last);
			last = getc(in);
		}
		AppendLexChar('\0');
		last = getc(in);
		
		return TOK_STRING;
	}
	
	if(last == '\'')
	{
		last = getc(in);
		ClearLexeme();
		if(last == '\\')
		{
			last = getc(in);
			switch(last)
			{
				case 'n': last = '\n'; break;
				case 'r': last = '\r'; break;
				case 't': last = '\t'; break;
				case '"': last = '"'; break;
				case '\'': last = '\''; break;
				case '\\': last = '\\'; break;
			}
		}
		
		// assure minimum capacity to store integer
		if(LexemeCapacity < 32)
		{
			LexemeCapacity = 32;
			char* newLex = realloc(Lexeme, LexemeCapacity);
			assert(newLex);
			Lexeme = newLex;
		}
		
		sprintf(Lexeme, "%i", last);
		last = getc(in);
			
		if(last != '\'')
			ErrorExit("Expected ' after previous '\n");
		
		last = getc(in);
		return TOK_NUMBER;
	}
	
	if(last == '#')
	{
		last = getc(in);
		if(last == '~')
		{
			while(last != EOF)
			{
				last = getc(in);
				if(last == '~')
				{
					last = getc(in);
					if(last == '#')
						break;
				}
			}
		}
		else
		{
			while(last != '\n' && last != '\r' && last != EOF)
				last = getc(in);
		}
		if(last == EOF) return TOK_EOF;
		else return GetToken(in);
	}
	
	if(last == EOF)
		return TOK_EOF;
	
	int lastChar = last;
	last = getc(in);
	
	if(lastChar == '=' && last == '=')
	{
		last = getc(in);
		return TOK_EQUALS;
	}
	else if(lastChar == '!' && last == '=')
	{
		last = getc(in);
		return TOK_NOTEQUAL;
	}
	else if(lastChar == '<' && last == '=')
	{
		last = getc(in);
		return TOK_LTE;
	}
	else if(lastChar == '>' && last == '=')
	{
		last = getc(in);
		return TOK_GTE;
	}
	else if(lastChar == '<' && last == '<')
	{
		last = getc(in);
		return TOK_LSHIFT;
	}
	else if(lastChar == '>' && last == '>')
	{
		last = getc(in);
		return TOK_RSHIFT;
	}
	else if(lastChar == '&' && last == '&')
	{
		last = getc(in);
		return TOK_AND;
	}
	else if(lastChar == '|' && last == '|')
	{
		last = getc(in);
		return TOK_OR;
	}
	else if(lastChar == '+' && last == '=')
	{
		last = getc(in);
		return TOK_CADD;
	}
	else if(lastChar == '-' && last == '=')
	{
		last = getc(in);
		return TOK_CSUB;
	}
	else if(lastChar == '*' && last == '=')
	{
		last = getc(in);
		return TOK_CMUL;
	}
	else if(lastChar == '/' && last == '=')
	{
		last = getc(in);
		return TOK_CDIV;
	}
	else if(lastChar == '.' && last == '.')
	{
		last = getc(in);
		if(lastChar != '.')
			ErrorExit("Invalid token '...'\n");
		last = getc(in);
		return TOK_ELLIPSIS;
	}
	
	return lastChar;
}

void ErrorExitE(Expr* exp, const char* format, ...)
{
	fprintf(stderr, "Error (%s:%i): ", exp->file, exp->line);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	
	exit(1);
}

void WarnE(Expr* exp, const char* format, ...)
{
	fprintf(stderr, "Warning (%s:%i): ", exp->file, exp->line);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int GetNextToken(FILE* in)
{
	CurTok = GetToken(in);
	return CurTok;
}

const char* ExprNames[] = {
	"EXP_NUMBER",
	"EXP_STRING",
	"EXP_IDENT",
	"EXP_CALL",
	"EXP_VAR",
	"EXP_BIN",
	"EXP_PAREN",
	"EXP_WHILE",
	"EXP_FUNC",
	"EXP_IF",
	"EXP_RETURN",
	"EXP_EXTERN",
	"EXP_ARRAY_LITERAL",
	"EXP_ARRAY_INDEX",
	"EXP_UNARY",
	"EXP_FOR",
	"EXP_DOT",
	"EXP_DICT_LITERAL",
	"EXP_NULL",
	"EXP_CONTINUE",
	"EXP_BREAK",
	"EXP_COLON",
	"EXP_NEW",
	"EXP_LAMBDA",
	"EXP_LINKED_BINARY_CODE"
};

Expr* CreateExpr(ExprType type)
{
	Expr* exp = malloc(sizeof(Expr));
	assert(exp);
	
	exp->next = NULL;
	exp->type = type;
	exp->file = FileName;
	exp->line = LineNumber;
	exp->curFunc = CurFunc;

	return exp;
}

Expr* ParseExpr(FILE* in);

Expr* ParseIf(FILE* in)
{
	GetNextToken(in);
	
	Expr* exp = CreateExpr(EXP_IF);
	
	Expr* cond = ParseExpr(in);
				
	PushScope();
	
	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;
	
	while(CurTok != TOK_ELSE && CurTok != TOK_ELIF && CurTok != TOK_END)
	{
		if(!exprHead)
		{
			exprHead = ParseExpr(in);
			exprCurrent = exprHead;
		}
		else
		{
			exprCurrent->next = ParseExpr(in);
			exprCurrent = exprCurrent->next;
		}
	}
	
	exp->ifx.cond = cond;
	exp->ifx.bodyHead = exprHead;
	exp->ifx.alt = NULL;
	
	PopScope();
	
	if(CurTok == TOK_ELIF)
		exp->ifx.alt = ParseIf(in);
	else if(CurTok == TOK_ELSE)
	{
		GetNextToken(in);
	
		PushScope();
		
		Expr* exprHead = NULL;
		Expr* exprCurrent = NULL;
		
		while(CurTok != TOK_END)
		{
			if(!exprHead)
			{
				exprHead = ParseExpr(in);
				exprCurrent = exprHead;
			}
			else
			{
				exprCurrent->next = ParseExpr(in);
				exprCurrent = exprCurrent->next;
			}
		}
		GetNextToken(in);
		
		exp->ifx.alt = exprHead;
		
		PopScope();
	}
	else if(CurTok == TOK_END)
	{
		GetNextToken(in);
		exp->ifx.alt = NULL;
	}
	
	return exp;
}

const char* HintTypeStrings[] = {
	"number",
	"string",
	"array",
	"dict",
	"function",
	"native",
	"void",
	"dynamic"
};

const TypeHint NumberHint = { NULL, NUMBER };
const TypeHint StringHint = { NULL, STRING };
const TypeHint ArrayHint = { NULL, ARRAY };
const TypeHint DictHint = { NULL, DICT };
const TypeHint FunctionHint = { NULL, FUNC };
const TypeHint NativeHint = { NULL, NATIVE };
const TypeHint VoidHint = { NULL, VOID };
const TypeHint DynamicHint = { NULL, DYNAMIC };

const char* HintString(const TypeHint* type)
{
	if(type) return HintTypeStrings[type->hint];
	return "unknown";
}

char CompareTypes(const TypeHint* a, const TypeHint* b)
{
	if(!a || !b) return 1;
	
	if(a->hint == DYNAMIC || b->hint == DYNAMIC) return 1;
	return a->hint == b->hint;
}

const TypeHint* GetTypeHintFromString(const char* n)
{
	if(strcmp(n, "number") == 0) return &NumberHint;
	else if(strcmp(n, "string") == 0) return &StringHint;
	else if(strcmp(n, "array") == 0) return &ArrayHint;
	else if(strcmp(n, "dict") == 0) return &DictHint;
	else if(strcmp(n, "function") == 0) return &FunctionHint;
	else if(strcmp(n, "native") == 0) return &NativeHint;
	else if(strcmp(n, "dynamic") == 0) return &DynamicHint;
	else if(strcmp(n, "void") == 0) return &VoidHint;
	else ErrorExit("Invalid type hint '%s'\n", Lexeme);
	
	return NULL;
}

TypeHint* CreateTypeHint(HintType hint)
{
	TypeHint* type = malloc(sizeof(TypeHint));
	assert(type);
	type->hint = hint;
	type->next = NULL;

	return type;
}

TypeHint* ParseTypeHint(FILE* in)
{
	// multi-type hint is bad
	/*if(CurTok == '(')
	{
		GetNextToken(in);
		
		TypeHint* types = NULL;
		TypeHint* currentType = NULL;
		
		while(CurTok != ')')
		{
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identifier in type declaration\n");
			
			TypeHint* type = CreateTypeHint(GetTypeHintFromString(Lexeme)->hint);
			
			if(!types)
			{
				types = type;
				currentType = type;
			}
			else
			{
				currentType->next = type;
				currentType = type;
			}
			
			GetNextToken(in);
			
			if(CurTok == ',') GetNextToken(in);
			else if(CurTok != ')') ErrorExit("Expected ')' or ',' in type declaration list\n");
		}
		
		GetNextToken(in);
		return types;
	}
	else 
	*/
	if(CurTok == TOK_IDENT)
	{
		TypeHint* type = CreateTypeHint(GetTypeHintFromString(Lexeme)->hint);
		GetNextToken(in);
		return type;
	}
	else
		ErrorExit("Expected identifier for type declaration\n");
	
	return NULL;
}

const TypeHint* InferTypeFromExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NULL: return &DynamicHint; break;
		
		case EXP_NUMBER: return &NumberHint; break;
		
		case EXP_STRING: return &StringHint; break;
		
		case EXP_IDENT: 
		{
			if(!exp->varx.varDecl)
				exp->varx.varDecl = ReferenceVariable(exp->varx.name);
			
			if(exp->varx.varDecl)
				return exp->varx.varDecl->type;
			else
			{
				FuncDecl* decl = ReferenceFunction(exp->varx.name);
				
				if(decl)
					return &FunctionHint;
				else
					return NULL;
			}
		} break;
		
		case EXP_CALL:
		{
			if(exp->callx.func->type == EXP_IDENT)
			{
				FuncDecl* decl = ReferenceFunction(exp->callx.func->varx.name);
				if(decl)
					return decl->returnType;
				else
					return NULL;
			}
		} break;
		
		case EXP_VAR:
		{
			return exp->varx.varDecl->type;
		} break;
		
		case EXP_BIN:
		{
			const TypeHint* a = InferTypeFromExpr(exp->binx.lhs);
			const TypeHint* b = InferTypeFromExpr(exp->binx.rhs);
			
			if(exp->binx.op != '=')
			{
				if(CompareTypes(a, b))
					return &DynamicHint;
				else
					return NULL;
			}
			else
				return &VoidHint;
		} break;
		
		case EXP_PAREN:
		{
			return InferTypeFromExpr(exp->parenExpr);
		} break;
		
		case EXP_DICT_LITERAL:
		{
			return &DictHint;
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			return &ArrayHint;
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			return &DynamicHint;
		} break;
		
		case EXP_DOT:
		{
			return &DynamicHint;
		} break;
		
		case EXP_LAMBDA:
		{
			return exp->lamx.decl->returnType;
		} break;
		
		default:
			break;
	}
	
	return NULL;
}

Expr* ParseFactor(FILE* in)
{
	switch(CurTok)
	{
		case TOK_NUMBER:
		{
			Expr* exp = CreateExpr(EXP_NUMBER);
			exp->constDecl = RegisterNumber(strtod(Lexeme, NULL));
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_HEXNUM:
		{
			Expr* exp = CreateExpr(EXP_NUMBER);
			exp->constDecl = RegisterNumber(strtol(Lexeme, NULL, 16));
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_STRING:
		{
			Expr* exp = CreateExpr(EXP_STRING);
			exp->constDecl = RegisterString(Lexeme);
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_NULL:
		{
			GetNextToken(in);
			Expr* exp = CreateExpr(EXP_NULL);
			return exp;
		} break;
		
		case TOK_NEW:
		{
			GetNextToken(in);
			Expr* exp = CreateExpr(EXP_NEW);
			exp->newExpr = ParseExpr(in);
			return exp;
		} break;
		
		case TOK_CONTINUE:
		{
			GetNextToken(in);
			Expr* exp = CreateExpr(EXP_CONTINUE);
			return exp;
		} break;
		
		case TOK_BREAK:
		{
			GetNextToken(in);
			Expr* exp = CreateExpr(EXP_BREAK);
			return exp;
		} break;
		
		case '[':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_ARRAY_LITERAL);
			
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			int len = 0;
			while(CurTok != ']')
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
				if(CurTok == ',') GetNextToken(in);
				++len;
			}			
			GetNextToken(in);
			exp->arrayx.head = exprHead;
			exp->arrayx.length = len;
			
			return exp;
		} break;
		
		case '{':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_DICT_LITERAL);
			
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			int len = 0;
			while(CurTok != '}')
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
				if(CurTok == ',') GetNextToken(in);
				++len;
			}			
			GetNextToken(in);
			exp->dictx.pairsHead = exprHead;
			exp->dictx.length = len;
			
			static int nameIndex = 0;
			static char nameBuf[MAX_ID_NAME_LENGTH];
			
			sprintf(nameBuf, "__dl%i__", nameIndex++);
			
			exp->dictx.decl = RegisterVariable(nameBuf);
			
			return exp;
		} break;
		
		case TOK_IDENT:
		{
			Expr* exp = CreateExpr(EXP_IDENT);
			
			char name[MAX_ID_NAME_LENGTH];
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			if(CurFunc)
			{
				VarDecl* decl = ReferenceVariable(name);
				if(CurFunc->isLambda)
				{
					if(decl && !decl->isGlobal && decl->scope <= CurFunc->scope)
					{
						FuncDecl* funcDecl = CurFunc;
						
						Expr* dict = CreateExpr(EXP_IDENT);
						dict->varx.varDecl = funcDecl->envDecl;
						strcpy(dict->varx.name, funcDecl->envDecl->name);
						
						// the lambda which should have this variable as an upvalue
						FuncDecl* highestDecl = funcDecl;
						
						Expr* upvalueAcc = CreateExpr(EXP_DOT);
						
						funcDecl = funcDecl->prevDecl;
						
						while(funcDecl && funcDecl->isLambda && decl->scope <= funcDecl->scope)
						{	
							Expr* acc = CreateExpr(EXP_DOT);
							acc->dotx.dict = dict;
							strcpy(acc->dotx.name, funcDecl->envDecl->name);
							dict = acc;
							
							highestDecl = funcDecl;
							
							funcDecl = funcDecl->prevDecl;
						}
						
						Upvalue* upvalue = malloc(sizeof(Upvalue));
						upvalue->decl = decl;
						upvalue->next = highestDecl->upvalues;
						highestDecl->upvalues = upvalue;
						
						upvalueAcc->dotx.dict = dict;
						strcpy(upvalueAcc->dotx.name, decl->name);
					
						return upvalueAcc;
					}
				}
				else if(decl && !decl->isGlobal && decl->scope <= CurFunc->scope)
					ErrorExit("Attempted to access upvalue '%s' from non-capturing function '%s'\n", name, CurFunc->name);
			}
			exp->varx.varDecl = ReferenceVariable(name);
			strcpy(exp->varx.name, name);
			return exp;		
		} break;
		
		case TOK_VAR:
		{
			Expr* exp = CreateExpr(EXP_VAR);
			
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected ident after 'var' but received something else\n");
			
			exp->varx.varDecl = RegisterVariable(Lexeme);
			strcpy(exp->varx.name, Lexeme);
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				exp->varx.varDecl->type = ParseTypeHint(in);
			}
			return exp;
		} break;
		
		case '(':
		{
			Expr* exp = CreateExpr(EXP_PAREN);
			GetNextToken(in);
			
			exp->parenExpr = ParseExpr(in);
			
			if(CurTok != ')')
				ErrorExit("Expected ')' after previous '('\n");
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_WHILE:
		{
			Expr* exp = CreateExpr(EXP_WHILE);
			GetNextToken(in);
			
			exp->whilex.cond = ParseExpr(in);
		
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			
			PushScope();
			while(CurTok != TOK_END)
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
			}
			PopScope();
			GetNextToken(in);
			
			exp->whilex.bodyHead = exprHead;
			
			return exp;
		} break;
		
		case TOK_FOR:
		{
			Expr* exp = CreateExpr(EXP_FOR);
			GetNextToken(in);
			
			PushScope();
			exp->forx.init = ParseExpr(in);
			
			if(CurTok != ',')
				ErrorExit("Expected ',' after for initial expression\n");
			GetNextToken(in);
			
			exp->forx.cond = ParseExpr(in);
			
			if(CurTok != ',')
				ErrorExit("Expected ',' after for condition\n");
			GetNextToken(in);
				
			exp->forx.iter = ParseExpr(in);
			
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			
			while(CurTok != TOK_END)
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
			}
			GetNextToken(in);
			PopScope();
		
			exp->forx.bodyHead = exprHead;
			
			return exp;
		} break;
		
		case TOK_FUNC:
		{
			Expr* exp = CreateExpr(EXP_FUNC);
			GetNextToken(in);
			
			char name[MAX_ID_NAME_LENGTH];
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identifier after 'func'\n");
			
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after 'func' %s\n", name);
			GetNextToken(in);
			
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = EnterFunction(name);
			
			PushScope();
			
			TypeHint* argTypes = NULL;
			TypeHint* argTypesCurrent = NULL;
			
			while(CurTok != ')')
			{	
				if(CurTok == TOK_ELLIPSIS)
				{
					decl->hasEllipsis = 1;
					GetNextToken(in);
					if(CurTok != ')')
						ErrorExit("Attempted to place ellipsis in the middle of an argument list\n");
					break;
				}
				
				if(CurTok != TOK_IDENT)
					ErrorExit("Expected identifier/ellipsis in argument list for function '%s' but received something else (%c, %i)\n", name, CurTok, CurTok);
				
				VarDecl* argDecl = RegisterArgument(Lexeme);
				
				GetNextToken(in);
				
				if(CurTok == ':')
				{
					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
						
					if(!argTypes)
					{
						argTypes = type;
						argTypesCurrent = type;
					}
					else
					{
						argTypesCurrent->next = type;
						argTypesCurrent = type;
					}
					
					argDecl->type = type;
				}
			
				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in function '%s' argument list\n", name);
			}
			decl->argTypes = argTypes;
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->returnType = ParseTypeHint(in);
			}
			
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			
			while(CurTok != TOK_END)
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
			}
			GetNextToken(in);
			PopScope();
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->returnType, &VoidHint))
					Warn("Reached end of non-void hinted function '%s' without a value returned\n", decl->name);
			}
			exp->funcx.decl = decl;
			exp->funcx.bodyHead = exprHead;
			
			return exp;
		} break;
		
		case TOK_IF:
		{
			return ParseIf(in);
		} break;
		
		case TOK_RETURN:
		{
			if(CurFunc)
			{				
				Expr* exp = CreateExpr(EXP_RETURN);
				GetNextToken(in);
				if(CurTok != ';')
				{
					if(CurFunc->returnType && CurFunc->returnType->hint == VOID)
						Warn("Attempting to return a value in a function which was hinted to return %s\n", HintString(CurFunc->returnType));
						
					if(CurFunc->hasReturn == -1)
						CurFunc->hasReturn = 1;
					else if(!CurFunc->hasReturn)
						ErrorExit("Attempted to return value from function when previous return statement in the same function returned no value\n");
					exp->retExpr = ParseExpr(in);
					
					const TypeHint* inferredType = InferTypeFromExpr(exp->retExpr);
					if(!CompareTypes(inferredType, CurFunc->returnType))
						Warn("Return value type '%s' does not match with function return value type '%s'\n", HintString(inferredType), HintString(CurFunc->returnType));
				
					if(!CurFunc->returnType)
						CurFunc->returnType = InferTypeFromExpr(exp->retExpr);
					
					return exp;
				}
								
				if(!CompareTypes(CurFunc->returnType, &VoidHint))
					Warn("Attempting to return without a value in a function which was hinted to return a value\n");
				
				if(CurFunc->hasReturn == -1)
					CurFunc->hasReturn = 0;
				else if(CurFunc->hasReturn)
					ErrorExit("Attempted to return with no value when previous return statement in the same function returned a value\n");
				
				GetNextToken(in);
			
				exp->retExpr = NULL;
				return exp;	
			}
			
			ErrorExit("WHAT ARE YOU TRYING TO RETURN FROM! YOU'RE NOT EVEN INSIDE A FUNCTION BODY!\n");
			
			return NULL;
		} break;
		
		case TOK_EXTERN:
		{
			Expr* exp = CreateExpr(EXP_EXTERN);
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT) ErrorExit("Expected identifier after 'extern'\n");
			char name[MAX_ID_NAME_LENGTH];
			strcpy(name, Lexeme);
		
			TypeHint* argTypes = NULL;

			GetNextToken(in);
			if(CurTok == '(')
			{
				GetNextToken(in);
				
				TypeHint* argTypesCurrent = NULL;
				
				while(CurTok != ')')
				{
					if(CurTok != TOK_IDENT)
						ErrorExit("Expected identifier in extern type list\n");
					
					TypeHint* type = ParseTypeHint(in);
					
					if(!argTypes)
					{
						argTypes = type;
						argTypesCurrent = type;
					}
					else
					{
						argTypesCurrent->next = type;
						argTypesCurrent = type;
					}
					
					if(CurTok == ',') GetNextToken(in);
					else if(CurTok != ')') ErrorExit("Expected ')' or ',' in extern argument type list\n");
				}
				GetNextToken(in);
			}
			
			if(CurTok == TOK_AS)
			{
				// TODO: GET ALIASING WORKING WHY ISN'T IT WORKING
				ErrorExit("aliasing not supported\n");
				
				GetNextToken(in);
				
				if(CurTok != TOK_IDENT) ErrorExit("Expected identifier after 'as'\n");
				exp->extDecl = RegisterExternAs(name, Lexeme);
				
				printf("registering extern %s as %s\n", name, Lexeme);
				
				GetNextToken(in);
			}
			else
			{	
				exp->extDecl = RegisterExtern(name);
				if(CurTok == ':')
				{
					GetNextToken(in);
					if(CurTok != TOK_IDENT)
						ErrorExit("Expected identifier after ':'\n");
					
					exp->extDecl->returnType = ParseTypeHint(in);
				}
				
				exp->extDecl->argTypes = argTypes;
			}
			
			return exp;
		} break;
		
		case TOK_LAMBDA:
		{
			Expr* exp = CreateExpr(EXP_LAMBDA);
			
			if(VarStackDepth <= 0 || !CurFunc)
				ErrorExit("Lambdas cannot be declared at global scope (use a func and get a pointer to it instead)\n");
			
			GetNextToken(in);
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after lambda\n");
				
			GetNextToken(in);
			
			static int lambdaIndex = 0;
			static char buf[MAX_ID_NAME_LENGTH];
			
			sprintf(buf, "__ld%i__", lambdaIndex++);
			exp->lamx.dictDecl = RegisterVariable(buf);
			
			// NOTE: this must be done after the internal dist reg to make sure that we declare the lambda's internal
			// dict in the upvalue scope (i.e in the function where it's declared, not in the lambda function decl itself)
			sprintf(buf, "__l%i__", lambdaIndex);
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = EnterFunction(buf);
			
			decl->isLambda = 1;
			
			decl->prevDecl = prevDecl;
			
			PushScope();
			
			decl->envDecl = RegisterArgument("env");
			
			TypeHint* argTypes = NULL;
			TypeHint* argTypesCurrent = NULL;
			
			while(CurTok != ')')
			{	
				if(CurTok == TOK_ELLIPSIS)
				{
					decl->hasEllipsis = 1;
					GetNextToken(in);
					if(CurTok != ')')
						ErrorExit("Attempted to place ellipsis in the middle of an argument list\n");
					break;
				}
				
				if(CurTok != TOK_IDENT)
					ErrorExit("Expected identifier/ellipsis in argument list for lambda but received something else (%c, %i)\n", CurTok, CurTok);
				
				VarDecl* argDecl = RegisterArgument(Lexeme);
				
				GetNextToken(in);
				
				if(CurTok == ':')
				{
					GetNextToken(in);
					
					TypeHint* type = ParseTypeHint(in);
						
					if(!argTypes)
					{
						argTypes = type;
						argTypesCurrent = type;
					}
					else
					{
						argTypesCurrent->next = type;
						argTypesCurrent = type;
					}
					
					argDecl->type = type;
				}
			
				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in lambda argument list\n");
			}
			decl->argTypes = argTypes;
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->returnType = ParseTypeHint(in);
			}
			
			Expr* exprHead = NULL;
			Expr* exprCurrent = NULL;
			
			while(CurTok != TOK_END)
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
			}
			GetNextToken(in);
			
			PopScope();
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->returnType, &VoidHint))
					Warn("Reached end of non-void hinted lambda without a value returned\n");
			}
			
			exp->lamx.decl = decl;
			exp->lamx.bodyHead = exprHead;
			
			return exp;
		} break;
	};
	
	ErrorExit("Unexpected token '%c' (%i) (%s)\n", CurTok, CurTok, Lexeme);
	
	return NULL;
}

char IsPostOperator()
{
	switch(CurTok)
	{
		case '[': case '.':  case '(': case ':':
			return 1;
	}
	return 0;
}

Expr* ParsePost(FILE* in, Expr* pre)
{
	switch(CurTok)
	{
		case '[':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_ARRAY_INDEX);
			exp->arrayIndex.arrExpr = pre;
			exp->arrayIndex.indexExpr = ParseExpr(in);
			
			const TypeHint* arrType = InferTypeFromExpr(exp->arrayIndex.arrExpr);
			
			if(!CompareTypes(arrType, &ArrayHint) && !CompareTypes(arrType, &StringHint) && !CompareTypes(arrType, &DictHint))
				Warn("Attempted to index a '%s' (non-indexable type)\n", HintString(arrType));
				
			if(CurTok != ']')
				ErrorExit("Expected ']' after previous '['\n");
			GetNextToken(in);
			if(!IsPostOperator()) return exp;
			return ParsePost(in, exp);
		} break;
		
		case '.':
		{
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identfier after '.'\n");
			
			Expr* exp = CreateExpr(EXP_DOT);
			strcpy(exp->dotx.name, Lexeme);
			exp->dotx.dict = pre;
			
			const TypeHint* dictType = InferTypeFromExpr(exp->dotx.dict);
			
			if(!CompareTypes(dictType, &DictHint))
				Warn("Value being indexed using dot ('.') operator does not denote a dictionary (instead, it evaluates to a '%s')\n", HintString(dictType));
			
			GetNextToken(in);
			if(!IsPostOperator()) return exp;
			return ParsePost(in, exp);
		} break;
		
		case '(':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_CALL);
			
			exp->callx.numArgs = 0;
			exp->callx.func = pre;
			
			while(CurTok != ')')
			{
				if(exp->callx.numArgs >= MAX_ARGS) ErrorExit("Exceeded maximum argument amount");
				exp->callx.args[exp->callx.numArgs++] = ParseExpr(in);
				if(CurTok == ',') GetNextToken(in);
			}
			
			const TypeHint* funcType = InferTypeFromExpr(exp->callx.func);
			
			if(!CompareTypes(funcType, &FunctionHint) && !CompareTypes(funcType, &DictHint))
				Warn("Value being called does not denote a callable value (instead, it evaluates to a '%s')\n", HintString(funcType));
				
			GetNextToken(in);
			
			if(!IsPostOperator()) return exp;
			else return ParsePost(in, exp);
		} break;
		
		case ':':
		{
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identfier after '.'\n");
			
			Expr* exp = CreateExpr(EXP_COLON);
			strcpy(exp->colonx.name, Lexeme);
			exp->colonx.dict = pre;
			
			const TypeHint* dictType = InferTypeFromExpr(exp->dotx.dict);
			
			if(!CompareTypes(dictType, &DictHint))
				Warn("Value being index using colon (':') operator does not denote a dictionary (instead, it evaluates to a '%s')\n", HintString(dictType));
			
			GetNextToken(in);
			if(!IsPostOperator()) return exp;
			return ParsePost(in, exp);
		} break;
		
		default:
			return pre;
	}
}

Expr* ParseUnary(FILE* in)
{
	switch(CurTok)
	{
		case '-': case '!':
		{
			int op = CurTok;
			
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_UNARY);
			exp->unaryx.op = op;
			exp->unaryx.expr = ParseExpr(in);
			
			const TypeHint* expType = InferTypeFromExpr(exp->unaryx.expr);
			
			if(!CompareTypes(expType, &NumberHint))
				Warn("Applying unary operator to non-numerical value of type '%s'\n", HintString(expType));
				
			return exp;
		} break;
		
		default:
			return ParsePost(in, ParseFactor(in));
	}
}

int GetTokenPrec()
{
	int prec = -1;
	switch(CurTok)
	{
		case '*': case '/': case '%': case '&': case '|': case TOK_LSHIFT: case TOK_RSHIFT: prec = 6; break;
		
		case '+': case '-': prec = 5; break;
	
		case '<': case '>': case TOK_LTE: case TOK_GTE:	prec = 4; break;
	
		case TOK_EQUALS: case TOK_NOTEQUAL: prec = 3; break;
		
		case TOK_AND: case TOK_OR: prec = 2; break;
			
		case TOK_CADD: case TOK_CSUB: case TOK_CMUL: case TOK_CDIV: case '=': prec = 1; break;
	}
	
	return prec;
}

Expr* ParseBinRhs(FILE* in, int exprPrec, Expr* lhs)
{
	while(1)
	{
		int prec = GetTokenPrec();
		
		if(prec < exprPrec)
			return lhs;

		int binOp = CurTok;

		GetNextToken(in);

		Expr* rhs = ParseUnary(in);
		int nextPrec = GetTokenPrec();
		
		if(prec < nextPrec)
			rhs = ParseBinRhs(in, prec + 1, rhs);

		Expr* newLhs = CreateExpr(EXP_BIN);
		
		newLhs->binx.lhs = lhs;
		newLhs->binx.rhs = rhs;
		newLhs->binx.op = binOp;
		
		lhs = newLhs;
	}
} 

Expr* ParseExpr(FILE* in)
{
	Expr* unary = ParseUnary(in);
	Expr* exp = ParseBinRhs(in, 0, unary);

	if(exp->type == EXP_BIN && exp->binx.op == '=')
	{
		if(exp->binx.lhs->type == EXP_VAR)
		{
			if(!exp->binx.lhs->varx.varDecl->type)
				exp->binx.lhs->varx.varDecl->type = InferTypeFromExpr(exp->binx.rhs);
		}
		else if(exp->binx.lhs->type == EXP_IDENT)
		{
			if(exp->binx.lhs->varx.varDecl)
			{
				const TypeHint* inf = InferTypeFromExpr(exp->binx.rhs);
				if(!exp->binx.lhs->varx.varDecl->type)
					exp->binx.lhs->varx.varDecl->type = inf;
				else if(!CompareTypes(inf, exp->binx.lhs->varx.varDecl->type))
					Warn("Assigning a '%s' to a variable of type '%s'\n", HintString(inf), HintString(exp->binx.lhs->varx.varDecl->type));
			}
		}
	}
	
	return exp;
}

void DebugExprList(Expr* head);
void DebugExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER:
		{
			printf("%g", exp->constDecl->number);
		} break;
		
		case EXP_STRING:
		{
			printf("'%s'", exp->constDecl->string);
		} break;
		
		case EXP_IDENT:
		{
			printf("%s", exp->varx.varDecl->name);
		} break;
		
		case EXP_CALL:
		{
			DebugExpr(exp->callx.func);
			printf("(");
			for(int i = 0; i < exp->callx.numArgs; ++i)
			{
				DebugExpr(exp->callx.args[i]);
				if(i + 1 < exp->callx.numArgs)
					printf(",");
			}
			printf(")");
		} break;
		
		case EXP_VAR:
		{
			printf("var %s", exp->varx.varDecl->name);
		} break;
		
		case EXP_BIN:
		{
			printf("(");
			DebugExpr(exp->binx.lhs);
			switch(exp->binx.op)
			{
				case TOK_AND: printf("&&"); break;
				case TOK_OR: printf("||"); break;
				case TOK_GTE: printf(">="); break;
				case TOK_LTE: printf("<="); break;
				
				default:
					printf("%c", exp->binx.op);
			}
			DebugExpr(exp->binx.rhs);
			printf(")");
		} break;
		
		case EXP_PAREN:
		{
			printf("(");
			DebugExpr(exp->parenExpr);
			printf(")");
		} break;
		
		case EXP_WHILE:
		{
			printf("while ");
			DebugExpr(exp->whilex.cond);
			printf("\n");
			for(Expr* node = exp->whilex.bodyHead; node != NULL; node = node->next)
			{
				printf("\t");
				DebugExpr(node);
				printf("\n");
			}
			printf("end\n");
		} break;
		
		case EXP_FUNC:
		{
			printf("func %s(...)\n", exp->funcx.decl->name);
			DebugExprList(exp->funcx.bodyHead);
			printf("end\n");
		} break;
		
		case EXP_IF:
		{
			printf("if ");
			DebugExpr(exp->ifx.cond);
			printf("\n");
			
			for(Expr* node = exp->whilex.bodyHead; node != NULL; node = node->next)
			{
				printf("\t");
				DebugExpr(node);
				printf("\n");
			}
			
			printf("end\n");
		} break;
		
		case EXP_RETURN:
		{
			printf("return");
			
			if(exp->retExpr)
			{
				printf("(");
				DebugExpr(exp->retExpr);
				printf(")");
			}
			else
				printf(";");
		} break;
		
		case EXP_EXTERN:
		{
			printf("extern %s\n", exp->extDecl->name);
		} break;
		
		default:
			break;
	}
}

// allow code to access ast at runtime
void ExposeExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER:
		{
			AppendCode(OP_PUSH_NUMBER);
			AppendInt(exp->constDecl->index);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_number_expr")->index);
		} break;
		
		case EXP_STRING:
		{
			AppendCode(OP_PUSH_STRING);
			AppendInt(exp->constDecl->index);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_string_expr")->index);
		} break;
		
		case EXP_IDENT:
		{
			AppendCode(OP_PUSH_STRING);
			AppendInt(RegisterString(exp->varx.name)->index);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_ident_expr")->index);
		} break;
		
		case EXP_PAREN:
		{
			ExposeExpr(exp->parenExpr);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_paren_expr")->index);
		} break;
		
		case EXP_BIN:
		{
			AppendCode(OP_PUSH_NUMBER);
			AppendInt(RegisterNumber(exp->binx.op)->index);
			
			ExposeExpr(exp->binx.rhs);
			ExposeExpr(exp->binx.lhs);
			
			AppendCode(OP_CALL);
			AppendCode(3);
			AppendInt(ReferenceFunction("_binary_expr")->index);
		} break;
		
		case EXP_CALL:
		{
			for(int i = exp->callx.numArgs - 1; i >= 0; --i)
				ExposeExpr(exp->callx.args[i]);
			
			ExposeExpr(exp->callx.func);
			
			AppendCode(OP_CALL);
			AppendCode(exp->callx.numArgs + 1);
			AppendInt(ReferenceFunction("_call_expr")->index);
		} break;
		
		default:
		{
			AppendCode(OP_CALL);
			AppendCode(0);
			AppendInt(ReferenceFunction("_unknown_expr")->index);
		} break;
	}
}

void CompileExpr(Expr* exp);
void CompileValueExpr(Expr* exp);

char CompileIntrinsic(Expr* exp, const char* name)
{
	if(strcmp(name, "len") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'len' only takes 1 argument\n");
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_LENGTH);
		return 1;
	}
	else if(strcmp(name, "write") == 0)
	{
		if(exp->callx.numArgs < 1)
			ErrorExitE(exp, "Intrinsic 'write' takes at least 1 argument\n");
		for(int i = 0; i < exp->callx.numArgs; ++i)
		{
			CompileValueExpr(exp->callx.args[i]);
			AppendCode(OP_WRITE);
		}
		return 1;
	}
	else if(strcmp(name, "read") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic 'read' takes no arguments\n");
		AppendCode(OP_READ);
		return 1;
	}
	else if(strcmp(name, "push") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'push' only takes 2 arguments\n");
		CompileValueExpr(exp->callx.args[1]);
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_PUSH);
		return 1;
	}
	else if(strcmp(name, "pop") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'push' only takes 1 argument\n");
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_POP);
		return 1;
	}
	else if(strcmp(name, "clear") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'clear' only takes 1 argument\n");
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_CLEAR);
		return 1;
	}
	else if(strcmp(name, "array") == 0)
	{
		if(exp->callx.numArgs > 1)
			ErrorExitE(exp, "Intrinsic 'array' takes no more than 1 argument\n");
			
		if(exp->callx.numArgs == 1)
			CompileValueExpr(exp->callx.args[0]);
		else
		{
			AppendCode(OP_PUSH_NUMBER);
			AppendInt(RegisterNumber(0)->index);
		}
	
		AppendCode(OP_CREATE_ARRAY);
		return 1;
	}
	else if(strcmp(name, "assert_func_exists") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'assert_func_exists' only takes 2 arguments\n");
		
		if(exp->callx.args[0]->type != EXP_IDENT)
			ErrorExitE(exp->callx.args[0], "The first argument to intrinisic 'assert_func_exists' must be an identifier!\n");
			
		if(exp->callx.args[1]->type != EXP_STRING)
			ErrorExitE(exp->callx.args[1], "The second argument to intrinsic 'assert_func_exists' must be a string!\n");
		
		FuncDecl* decl = ReferenceFunction(exp->callx.args[0]->varx.name);
		if(!decl)
			ErrorExitE(exp, "%s\n", exp->callx.args[1]->constDecl->string);
		return 1;
	}
	else if(strcmp(name, "setvmdebug") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'setvmdebug' only takes 1 argument\n");
		
		if(exp->callx.args[0]->type != EXP_NUMBER)
			ErrorExitE(exp->callx.args[0], "Argument to intrinsic 'setvmdebug' must be a number/boolean!\n");
		
		AppendCode(OP_SETVMDEBUG);
		AppendCode((Word)exp->callx.args[0]->constDecl->number);
		return 1;
	}
	else if(strcmp(name, "push_stack") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic 'push_stack' takes no arguments\n");
		AppendCode(OP_PUSH_STACK);
		return 1;
	}
	else if(strcmp(name, "pop_stack") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic 'push_stack' takes no arguments\n");
		AppendCode(OP_POP_STACK);
		return 1;
	}
	else if(strcmp(name, "dict_get") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'dict_get' takes 2 arguments\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
		AppendCode(OP_DICT_GET_RKEY);
		return 1;
	}
	else if(strcmp(name, "dict_set") == 0)
	{
		if(exp->callx.numArgs != 3)
			ErrorExitE(exp, "Intrinsic 'dict_set' takes 3 arguments\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
		AppendCode(OP_DICT_SET_RKEY);
		return 1;
	}
	else if(strcmp(name, "expand") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'expand' takes 2 arguments\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
		AppendCode(OP_EXPAND_ARRAY);
		return 1;
	}
	else if(strcmp(name, "getargs") == 0)
	{
		AppendCode(OP_GETARGS);
		if(exp->callx.numArgs == 0)
			AppendInt(-1);
		else
		{
			if(exp->callx.numArgs != 1) ErrorExitE(exp, "Intrinsic 'getargs' takes up to 1 argument\n");
			if(exp->callx.args[0]->type != EXP_IDENT) ErrorExitE(exp->callx.args[0], "Expected first argument to intrinsic 'getargs' to be an identifier\n");
			
			Expr* arg = exp->callx.args[0];
	
			if(!arg->varx.varDecl)
				arg->varx.varDecl = ReferenceVariable(arg->varx.name);
			
			if(!arg->varx.varDecl) ErrorExitE(arg, "Undeclared identifier as argument to 'getargs'\n");
			AppendInt(arg->varx.varDecl->index - 1);
		}
		return 1;
	}
	else if(strcmp(name, "getfilename") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic 'getfilename' takes no arguments\n");
		AppendCode(OP_PUSH_STRING);
		AppendInt(RegisterString(exp->file)->index);
		return 1;
	}
	else if(strcmp(name, "getlinenumber") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic 'getlinenumber' takes no arguments\n");
		AppendCode(OP_PUSH_NUMBER);
		AppendInt(RegisterNumber(exp->line)->index);
		return 1;
	}
	else if(strcmp(name, "exp") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'getexpr' takes 1 argument\n");
		
		FuncDecl* meta = ReferenceFunction("__meta_mt");
		
		if(!meta)
			ErrorExitE(exp, "'getexpr' requires the 'meta.mt' library");
		
		static char init = 0;
		if(!init)
		{
			init = 1;
			AppendCode(OP_CALL);
			AppendCode(0);
			AppendInt(meta->index);
		}
		
		Expr* tree = exp->callx.args[0];
		ExposeExpr(tree);
		return 1;
	}
	else if(strcmp(name, "filestring") == 0)
	{	
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'filestring' takes 1 argument\n");
		
		if(exp->callx.args[0]->type != EXP_STRING)
			ErrorExitE(exp->callx.args[0], "First argument to 'filestring' must be a string\n");
		
		FILE* file = fopen(exp->callx.args[0]->constDecl->string, "rb");
		if(!file)
			ErrorExitE(exp->callx.args[0], "Failed to open file '%s' for reading\n", exp->callx.args[0]->constDecl->string);
		
		fseek(file, 0, SEEK_END);
		long fsize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		char* string = malloc(fsize + 1);
		assert(string);
		fread(string, fsize, 1, file);
		fclose(file);
		
		string[fsize] = '\0';
		
		AppendCode(OP_PUSH_STRING);
		AppendInt(RegisterString(string)->index);
		
		return 1;
	}
	/*else if(strcmp(name, "loader_code") == 0)
	{
		if(exp->callx.numArgs != 3)
			ErrorExitE(exp, "Intrinsic 'loader_code' expects 1 argument\n");
		
		if(exp->callx.args[0]->type != EXP_STRING || exp->callx.args[1]->type != EXP_STRING || exp->callx.args[2]->type != EXP_STRING)
			ErrorExitE(exp->callx.args[0], "Intrinsic 'loader_code' expects its arguments to be strings\n");
			
		const char* compiler = exp->callx.args[1]->constDecl->string;
		const char* args = exp->callx.args[2]->constDecl->string;
			
		FILE* file = fopen("mint_loader.c", "w");
		if(!file)
			ErrorExitE(exp, "Failed to open loader file for writing\n");
		
		fprintf(file, "%s\n", exp->callx.args[0]->constDecl->string);
		fclose(file);
		
		static char cmdbuf[1024];
		sprintf(cmdbuf, "%s mint_loader.c %s\n", compiler, args);
		
		return 1;
	}
	else if(strcmp(name, "cstruct") == 0)
	{
		FuncDecl* cffi_mt = ReferenceFunction("__cffi_mt");
		if(!cffi_mt)
			ErrorExitE(exp, "Intrinsic 'cstruct' depends on the 'cffi.mt' library\n");
		
		if(exp->callx.numArgs <= 2)
			ErrorExitE(exp, "Intrinsic 'cstruct' takes at least 3 arguments (no useful struct has 0 members)\n");
			
		if(exp->callx.args[0]->type != EXP_STRING || exp->callx.args[1]->type != EXP_STRING)
			ErrorExitE(exp->callx.args[0], "Arguments to 'cstruct' must be strings\n");
		
		FILE* file = fopen("cstruct.c", "w");
		if(!file)
			ErrorExitE(exp, "Unable to open cstruct.c for writing (cstruct failed)\n");
		fprintf(file, "#include <stdio.h>\n"
				"#include <stddef.h>\n"
				"%s;\n"
				"int main(int argc, char* argv[])\n"
				"{\n"
				"	#define s struct %s\n"
				"	FILE* out = fopen(\"cstruct_result\", \"wb\");\n"
				"	if(!out) return 1;\n"
				"	size_t struct_size = sizeof(s);\n"
				"	fwrite(&struct_size, sizeof(size_t), 1, out);\n"
				"	size_t member_offset = 0;\n", exp->callx.args[1]->constDecl->string, exp->callx.args[0]->constDecl->string);
		
		for(int i = 2; i < exp->callx.numArgs; ++i)
		{
			if(exp->callx.args[i]->type != EXP_STRING)
				ErrorExitE(exp->callx.args[i], "Arguments to intrinsic 'cstruct' must be strings\n");
			
			fprintf(file, "	member_offset = offsetof(s, %s);\n"
					"	fwrite(&member_offset, sizeof(size_t), 1, out);\n", exp->callx.args[i]->constDecl->string);
		}
		
		fprintf(file, "	fclose(out);\n"
				"	return 0;\n"
				"}");
		fclose(file);
		
		system("gcc cstruct.c -o cstruct");
		system("cstruct");
		FILE* result = fopen("cstruct_result", "rb");
		if(!result)
			ErrorExitE(exp, "cstruct failed\n");
		size_t size;
		fread(&size, sizeof(size_t), 1, result);
		
		int nmemb = exp->callx.numArgs - 2;
		
		size_t* offsets = malloc(sizeof(size_t) * nmemb);
		assert(offsets);
		
		fread(offsets, sizeof(size_t), nmemb, result);
		fclose(result);
		
		for(int i = nmemb - 1; i >= 0; --i)
		{
			AppendCode(OP_PUSH_NUMBER);
			AppendInt(RegisterNumber(offsets[i])->index);
		}
		
		AppendCode(OP_PUSH_NUMBER);
		AppendInt(RegisterNumber(size)->index);
		
		AppendCode(OP_CALL);
		AppendCode(nmemb + 1);
		AppendInt(ReferenceFunction("_struct")->index);

		remove("cstruct.c");
		remove("cstruct_result");
		remove("cstruct.exe");
		
		return 1;
	}
	else if(strcmp(name, "renamefunc") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'renamefunc' takes 2 arguments\n");
		
		if(exp->callx.args[0]->type != EXP_IDENT || exp->callx.args[1]->type != EXP_IDENT)
			ErrorExitE(exp->callx.args[0], "Both arguments to 'renamefunc' must be identifiers\n");
		
		FuncDecl* decl = ReferenceFunction(exp->callx.args[0]->varx.name);
		if(!decl)
			ErrorExitE(exp->callx.args[0], "Attempted to rename non-existent function '%s'\n", exp->callx.args[0]->varx.name);
		
		decl->isAliased = 1;
		strcpy(decl->alias, exp->callx.args[1]->varx.name);
		
		return 1;
	}*/
	
	return 0;
}

void ResolveListSymbols(Expr* head);
void ResolveSymbols(Expr* exp)
{
	/*switch(exp->type)
	{
		case EXP_BIN:
		{
			ResolveSymbols(exp->binx.lhs);
			ResolveSymbols(exp->binx.rhs);
		} break;
	}*/
}

void ResolveListSymbols(Expr* head)
{
	Expr* node = head;
	while(node)
	{
		ResolveSymbols(node);
		node = node->next;
	}
}

Patch* Patches = NULL;

void AddPatch(PatchType type, int loc)
{
	Patch* p = malloc(sizeof(Patch));
	assert(p);
	
	p->type = type;
	p->loc = loc;
	
	p->next = Patches;
	Patches = p;
}

void ClearPatches()
{
	Patch* next;
	for(Patch* p = Patches; p != NULL; p = next)
	{
		next = p->next;
		free(p);
	}
	Patches = NULL;
}

void CompileValueExpr(Expr* exp);
void CompileCallExpr(Expr* exp, char expectReturn)
{
	assert(exp->type == EXP_CALL);
	
	Word numExpansions = 0;
	// TODO(optimization): do not iterate over the args more than once
	for(int i = 0; i < exp->callx.numArgs; ++i)
	{
		if(exp->callx.args[i]->type == EXP_CALL)
		{
			Expr* argx = exp->callx.args[i];
			if(argx->callx.func->type == EXP_IDENT)
			{
				if(strcmp(argx->callx.func->varx.name, "expand") == 0) 
					++numExpansions;
			}
		}
	}
	
	if(exp->callx.func->type == EXP_IDENT)
	{
		FuncDecl* decl = ReferenceFunction(exp->callx.func->varx.name);
		if(decl)
		{
			const TypeHint* node = decl->argTypes;
			
			for(int i = 0; node && i < exp->callx.numArgs; ++i)
			{
				const TypeHint* inf = InferTypeFromExpr(exp->callx.args[i]);
				
				if(!CompareTypes(inf, node))
					WarnE(exp->callx.args[i], "Argument %i's type '%s' does not match the expected type '%s'\n", i + 1, HintString(inf), HintString(node));
			
				node = node->next;
			}
			
			for(int i = exp->callx.numArgs - 1; i >= 0; --i)
				CompileValueExpr(exp->callx.args[i]);
				
			if(decl->isExtern)
			{
				AppendCode(OP_CALLF);
				AppendInt(decl->index);
			}
			else
			{
				if(expectReturn)
				{
					if(decl->hasReturn == 0) 
						ErrorExitE(exp, "Expected a function which had a return value in this context (function '%s' does not return a value)\n", decl->name);
				}
					
				if(numExpansions == 0)
				{
					if(!decl->hasEllipsis)
					{
						if(exp->callx.numArgs != decl->numArgs)
							ErrorExitE(exp, "Attempted to pass %i arguments to function '%s' which takes %i arguments\n", exp->callx.numArgs, decl->name, decl->numArgs);
					}
					else
					{
						if(exp->callx.numArgs < decl->numArgs)
							ErrorExitE(exp, "Attempted to pass %i arguments to function '%s' which takes at least %i arguments\n", exp->callx.numArgs, decl->name, decl->numArgs);
					}
				}
				
				AppendCode(OP_CALL);
				AppendCode(exp->callx.numArgs - numExpansions);
				AppendInt(decl->index);						
			}
			
			if(expectReturn)
				AppendCode(OP_GET_RETVAL);
		}
		else if(!CompileIntrinsic(exp, exp->callx.func->varx.name))
		{
			for(int i = exp->callx.numArgs - 1; i >= 0; --i)
				CompileValueExpr(exp->callx.args[i]);
				
			CompileValueExpr(exp->callx.func);
			
			AppendCode(OP_CALLP);
			AppendCode(exp->callx.numArgs - numExpansions);
			
			if(expectReturn)
				AppendCode(OP_GET_RETVAL);
		}
	}
	else if(exp->callx.func->type == EXP_COLON)
	{
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
		// first argument to function is dictionary
		CompileValueExpr(exp->callx.func->colonx.dict);
		
		// retrieve function from dictionary
		CompileValueExpr(exp->callx.func->colonx.dict);
		AppendCode(OP_DICT_GET);
		AppendInt(RegisterString(exp->callx.func->colonx.name)->index);
		
		AppendCode(OP_CALLP);
		AppendCode(exp->callx.numArgs + 1 - numExpansions);
		
		if(expectReturn)
			AppendCode(OP_GET_RETVAL);
	}
	else
	{			
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
			
		CompileValueExpr(exp->callx.func);
		
		AppendCode(OP_CALLP);
		AppendCode(exp->callx.numArgs - numExpansions);
		
		if(expectReturn)
			AppendCode(OP_GET_RETVAL);
	}
}

/*void ResolveLambdaReferences(Expr** pExp, Upvalue** upvalues, VarDecl* envDecl)
{
	Expr* exp = *pExp;
	
	switch(exp->type)
	{	
		case EXP_UNARY:
		{
			ResolveLambdaReferences(&exp->unaryx.expr, upvalues, envDecl);
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			Expr** node = &exp->arrayx.head;
			while(*node)
			{
				ResolveLambdaReferences(node, upvalues, envDecl);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_IDENT:
		{	
			if(!exp->varx.varDecl)
				exp->varx.varDecl = ReferenceVariable(exp->varx.name);

			if(exp->varx.varDecl)
			{
				if(!exp->varx.varDecl->isGlobal && exp->varx.varDecl->scope < envDecl->scope)
				{
					Upvalue* val = malloc(sizeof(Upvalue));
					assert(val);
					
					val->exp = CreateExpr(EXP_IDENT);
					val->exp->varx.varDecl = exp->varx.varDecl;
					strcpy(val->exp->varx.name, exp->varx.name);
					
					val->next = *upvalues;
					*upvalues = val;				
					
					Expr* envVar = CreateExpr(EXP_IDENT);
					envVar->varx.varDecl = envDecl;
					
					Expr* rep = CreateExpr(EXP_DOT);
					rep->dotx.dict = envVar;
					strcpy(rep->dotx.name, exp->varx.name);
					
					*pExp = rep;
				}
			}
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			ResolveLambdaReferences(&exp->arrayIndex.arrExpr, upvalues, envDecl);
			ResolveLambdaReferences(&exp->arrayIndex.indexExpr, upvalues, envDecl);
		} break;
		
		case EXP_BIN:
		{
			ResolveLambdaReferences(&exp->binx.lhs, upvalues, envDecl);
			ResolveLambdaReferences(&exp->binx.rhs, upvalues, envDecl);
		} break;
		
		case EXP_CALL:
		{
			for(int i = 0; i < exp->callx.numArgs; ++i)
				ResolveLambdaReferences(&exp->callx.args[i], upvalues, envDecl);
		} break;
		
		case EXP_PAREN:
		{
			ResolveLambdaReferences(&exp->parenExpr, upvalues, envDecl);
		} break;
		
		case EXP_DOT:
		{
			ResolveLambdaReferences(&exp->dotx.dict, upvalues, envDecl);
		} break;
		
		case EXP_DICT_LITERAL:
		{
			Expr** pNode = &exp->dictx.pairsHead;
			
			// NOTE: double checking for correct dict literal statements (once here and once in CompileValueExpr)
			while(*pNode)
			{
				Expr* node = *pNode;
				
				if(node->type != EXP_BIN) ErrorExitE(exp, "Non-binary expression in dictionary literal (Expected something = something_else)\n");
				if(node->binx.op != '=') ErrorExitE(exp, "Invalid dictionary literal binary operator '%c'\n", node->binx.op);
				if(node->binx.lhs->type != EXP_IDENT) ErrorExitE(exp, "Invalid lhs in dictionary literal (Expected identifier)\n");
				
				if(strcmp(node->binx.lhs->varx.name, "pairs") == 0) ErrorExitE(exp, "Attempted to assign value to reserved key 'pairs' in dictionary literal\n");
				
				ResolveLambdaReferences(pNode, upvalues, envDecl);
				
				pNode = &node->next;
			}
		} break;
		
		case EXP_LAMBDA:
		{
			// Nested lambda references are resolved when being compiled
		} break;
		
		case EXP_WHILE:
		{
			ResolveLambdaReferences(&exp->whilex.cond, upvalues, envDecl);
			
			Expr** node = &exp->whilex.bodyHead;
			
			while(*node)
			{
				ResolveLambdaReferences(node, upvalues, envDecl);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_FOR:
		{
			ResolveLambdaReferences(&exp->forx.init, upvalues, envDecl);
			
			ResolveLambdaReferences(&exp->forx.cond, upvalues, envDecl);
			
			Expr** node = &exp->forx.bodyHead;
			
			while(*node)
			{
				ResolveLambdaReferences(node, upvalues, envDecl);
				node = &(*node)->next;
			}
			
			ResolveLambdaReferences(&exp->forx.iter, upvalues, envDecl);
		} break;
		
		case EXP_IF:
		{
			ResolveLambdaReferences(&exp->ifx.cond, upvalues, envDecl);
			
			Expr** node = &exp->ifx.bodyHead;
			
			while(*node)
			{
				ResolveLambdaReferences(node, upvalues, envDecl);
				node = &(*node)->next;
			}
			
			if(exp->ifx.alt)
			{
				if(exp->ifx.alt->type != EXP_IF)
				{
					node = &exp->ifx.alt;
					
					while(*node)
					{
						ResolveLambdaReferences(node, upvalues, envDecl);
						node = &(*node)->next;
					}
				}
				else
					ResolveLambdaReferences(&exp->ifx.alt, upvalues, envDecl);
			}
		} break;
		
		case EXP_RETURN:
		{
			if(exp->retExpr)
				ResolveLambdaReferences(&exp->retExpr, upvalues, envDecl);
		} break;
		
		default:
			// no upvalue references involved in this expression type
			break;
	}
}*/

void CompileExprList(Expr* head);
// Expression should have a resulting value (pushed onto the stack)
void CompileValueExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER: case EXP_STRING:
		{
			AppendCode(exp->constDecl->type == CONST_NUM ? OP_PUSH_NUMBER : OP_PUSH_STRING);
			AppendInt(exp->constDecl->index);
		} break;
		
		case EXP_NULL:
		{
			AppendCode(OP_PUSH_NULL);
		} break;
		
		case EXP_UNARY:
		{
			switch(exp->unaryx.op)
			{
				case '-': CompileValueExpr(exp->unaryx.expr); AppendCode(OP_NEG); break;
				case '!': CompileValueExpr(exp->unaryx.expr); AppendCode(OP_LOGICAL_NOT); break;
			}
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			Expr* node = exp->arrayx.head;
			while(node)
			{
				CompileValueExpr(node);
				node = node->next;
			}
			
			AppendCode(OP_CREATE_ARRAY_BLOCK);
			AppendInt(exp->arrayx.length);
		} break;
		
		case EXP_IDENT:
		{	
			if(!exp->varx.varDecl)
				exp->varx.varDecl = ReferenceVariable(exp->varx.name);

			if(!exp->varx.varDecl)
			{
				FuncDecl* decl = ReferenceFunction(exp->varx.name);
				if(!decl)
					ErrorExitE(exp, "Attempted to reference undeclared identifier '%s'\n", exp->varx.name);
				
				/*if(decl->hasEllipsis)
					ErrorExitE(exp, "Attempted to get function pointer to variadic function '%s', which is not supported yet.\nPlease use the function directly", exp->varx.name);*/
				
				AppendCode(OP_PUSH_FUNC);
				AppendCode(decl->hasEllipsis);
				AppendCode(decl->isExtern);
				AppendCode(decl->numArgs);
				AppendInt(decl->index);
			}
			else
			{
				if(exp->varx.varDecl->isGlobal)
				{
					AppendCode(OP_GET);
					AppendInt(exp->varx.varDecl->index);
				}
				else
				{
					AppendCode(OP_GETLOCAL);
					AppendInt(exp->varx.varDecl->index);
				}
			}
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			CompileValueExpr(exp->arrayIndex.indexExpr);
			CompileValueExpr(exp->arrayIndex.arrExpr);
			
			AppendCode(OP_GETINDEX);
		} break;
		
		case EXP_BIN:
		{	
			if(exp->binx.op == '=')
				ErrorExitE(exp, "Assignment expressions cannot be used as values\n");
			else
			{
				CompileValueExpr(exp->binx.lhs);
				CompileValueExpr(exp->binx.rhs);
				
				switch(exp->binx.op)
				{
					case '+': AppendCode(OP_ADD); break;
					case '-': AppendCode(OP_SUB); break;
					case '*': AppendCode(OP_MUL); break;
					case '/': AppendCode(OP_DIV); break;
					case '%': AppendCode(OP_MOD); break;
					case '<': AppendCode(OP_LT); break;
					case '>': AppendCode(OP_GT); break;
					case '|': AppendCode(OP_OR); break;
					case '&': AppendCode(OP_AND); break;
					case TOK_EQUALS: AppendCode(OP_EQU); break;
					case TOK_LTE: AppendCode(OP_LTE); break;
					case TOK_GTE: AppendCode(OP_GTE); break;
					case TOK_NOTEQUAL: AppendCode(OP_NEQU); break;
					case TOK_AND: AppendCode(OP_LOGICAL_AND); break;
					case TOK_OR: AppendCode(OP_LOGICAL_OR); break;
					case TOK_LSHIFT: AppendCode(OP_SHL); break;
					case TOK_RSHIFT: AppendCode(OP_SHR); break;
					/*case TOK_CADD: AppendCode(OP_CADD); break;
					case TOK_CSUB: AppendCode(OP_CSUB); break;
					case TOK_CMUL: AppendCode(OP_CMUL); break;
					case TOK_CDIV: AppendCode(OP_CDIV); break;*/
					
					default:
						ErrorExitE(exp, "Unsupported binary operator %c (%i)\n", exp->binx.op, exp->binx.op);		
				}
			}
		} break;
		
		case EXP_CALL:
		{
			CompileCallExpr(exp, 1);
		} break;
		
		case EXP_PAREN:
		{
			CompileValueExpr(exp->parenExpr);
		} break;
		
		case EXP_DOT:
		{
			if(strcmp(exp->dotx.name, "pairs") == 0)
			{
				CompileValueExpr(exp->dotx.dict);
				AppendCode(OP_DICT_PAIRS);
			}
			else
			{
				CompileValueExpr(exp->dotx.dict);
				AppendCode(OP_DICT_GET);
				AppendInt(RegisterString(exp->dotx.name)->index);
			}
		} break;
		
		case EXP_DICT_LITERAL:
		{
			Expr* node = exp->dictx.pairsHead;
			
			VarDecl* decl = exp->dictx.decl;
			
			AppendCode(OP_PUSH_DICT);
			AppendCode(OP_SETLOCAL);
			AppendInt(decl->index);
			
			while(node)
			{
				if(node->type != EXP_BIN) ErrorExitE(exp, "Non-binary expression in dictionary literal (Expected something = something_else)\n");
				if(node->binx.op != '=') ErrorExitE(exp, "Invalid dictionary literal binary operator '%c'\n", node->binx.op);
				if(node->binx.lhs->type != EXP_IDENT) ErrorExitE(exp, "Invalid lhs in dictionary literal (Expected identifier)\n");
				
				if(strcmp(node->binx.lhs->varx.name, "pairs") == 0) ErrorExitE(exp, "Attempted to assign value to reserved key 'pairs' in dictionary literal\n");
				
				/*CompileExpr(node->binx.rhs);
				AppendCode(OP_PUSH_STRING);
				AppendInt(RegisterString(node->binx.lhs->varx.name)->index);*/
				
				CompileValueExpr(node->binx.rhs);
				
				AppendCode(OP_GETLOCAL);
				AppendInt(decl->index);
				
				AppendCode(OP_DICT_SET);
				AppendInt(RegisterString(node->binx.lhs->varx.name)->index);
				
				node = node->next;
			}
			AppendCode(OP_GETLOCAL);
			AppendInt(decl->index);
			
			/*AppendCode(OP_CREATE_DICT_BLOCK);
			AppendInt(exp->dictx.length);*/
		} break;
		
		case EXP_LAMBDA:
		{
			/*Upvalue* upvalue = NULL;
			
			Expr** rnode = &exp->lamx.bodyHead;
			while(*rnode)
			{
				ResolveLambdaReferences(rnode, &upvalue, exp->lamx.decl->envDecl);
				rnode = &(*rnode)->next;
			}*/
			
			AppendCode(OP_GOTO);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			exp->lamx.decl->pc = CodeLength;
			
			for(int i = 0; i < exp->lamx.decl->numLocals; ++i)
				AppendCode(OP_PUSH_NULL);
			
			CompileExprList(exp->lamx.bodyHead);
			AppendCode(OP_RETURN);
			
			EmplaceInt(emplaceLoc, CodeLength);
			
			VarDecl* decl = exp->lamx.dictDecl;
		
			AppendCode(OP_PUSH_DICT);
			AppendCode(OP_SETLOCAL);
			AppendInt(decl->index);
			
			// each nested lambda stores the previous env so that values above the uppermost lambda are accessible
			if(exp->lamx.decl->prevDecl->isLambda)
			{
				Expr* prevEnv = CreateExpr(EXP_IDENT);
				prevEnv->varx.varDecl = exp->lamx.decl->prevDecl->envDecl;
				strcpy(prevEnv->varx.name, exp->lamx.decl->prevDecl->envDecl->name);
				CompileValueExpr(prevEnv);
				
				AppendCode(OP_GETLOCAL);
				AppendInt(decl->index);
				
				AppendCode(OP_DICT_SET);
				AppendInt(RegisterString(exp->lamx.decl->prevDecl->envDecl->name)->index);
			}
			
			Upvalue* upvalue = exp->lamx.decl->upvalues;
			
			while(upvalue)
			{	
				Expr* uexp = CreateExpr(EXP_IDENT);
				uexp->varx.varDecl = upvalue->decl;
				strcpy(uexp->varx.name, upvalue->decl->name);
				
				CompileValueExpr(uexp);
				
				AppendCode(OP_GETLOCAL);
				AppendInt(decl->index);
				
				AppendCode(OP_DICT_SET);
				AppendInt(RegisterString(upvalue->decl->name)->index);
				
				upvalue = upvalue->next;
			}
			
			// TODO: The OP_PUSH_FUNC should only take whether the function is an extern or not, the rest can be determined by the vm tables
			AppendCode(OP_PUSH_FUNC);
			AppendCode(exp->lamx.decl->hasEllipsis);
			AppendCode(exp->lamx.decl->isExtern);
			AppendCode(exp->lamx.decl->numArgs);
			AppendInt(exp->lamx.decl->index);
			
			AppendCode(OP_GETLOCAL);
			AppendInt(decl->index);
			
			AppendCode(OP_DICT_SET);
			AppendInt(RegisterString("CALL")->index);
			
			AppendCode(OP_GETLOCAL);
			AppendInt(decl->index);
		} break;
		
		// when used as a value, it becomes a function pointer
		case EXP_FUNC:
		{
			AppendCode(OP_GOTO);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			exp->funcx.decl->pc = CodeLength;
			
			if(strcmp(exp->funcx.decl->name, "_main") == 0) EntryPoint = CodeLength;
			for(int i = 0; i < exp->funcx.decl->numLocals; ++i)
				AppendCode(OP_PUSH_NULL);
			
			CompileExprList(exp->funcx.bodyHead);
			AppendCode(OP_RETURN);
			
			EmplaceInt(emplaceLoc, CodeLength);
			
			// TODO: The OP_PUSH_FUNC should only take whether the function is an extern or not, the rest can be determined by the vm tables
			AppendCode(OP_PUSH_FUNC);
			AppendCode(exp->funcx.decl->hasEllipsis);
			AppendCode(exp->funcx.decl->isExtern);
			AppendCode(exp->funcx.decl->numArgs);
			AppendInt(exp->funcx.decl->index);
		} break;
		
		default:
			ErrorExitE(exp, "Expected value expression in place of %s\n", ExprNames[exp->type]);
	}
}

void CompileExprList(Expr* head);
void CompileExpr(Expr* exp)
{
	// printf("compiling expression (%s:%i)\n", exp->file, exp->line);
	
	switch(exp->type)
	{	
		case EXP_EXTERN: case EXP_VAR: break;
		
		case EXP_BIN:
		{	
			if(exp->binx.op == '=')
			{	
				CompileValueExpr(exp->binx.rhs);
								
				if(exp->binx.lhs->type == EXP_VAR || exp->binx.lhs->type == EXP_IDENT)
				{	
					if(!exp->binx.lhs->varx.varDecl)
					{
						exp->binx.lhs->varx.varDecl = ReferenceVariable(exp->binx.lhs->varx.name);
						if(!exp->binx.lhs->varx.varDecl)
							ErrorExitE(exp, "Attempted to assign to non-existent variable '%s'\n", exp->binx.lhs->varx.name);
					}
					
					if(exp->binx.lhs->varx.varDecl->isGlobal)
					{
						if(EntryPoint != 0 && exp->binx.lhs->type == EXP_VAR) printf("Warning: assignment operations to global variables will not execute unless they are inside the entry point\n");
						AppendCode(OP_SET);
					}
					else AppendCode(OP_SETLOCAL);
				
					AppendInt(exp->binx.lhs->varx.varDecl->index);
				}
				else if(exp->binx.lhs->type == EXP_ARRAY_INDEX)
				{
					CompileValueExpr(exp->binx.lhs->arrayIndex.indexExpr);
					CompileValueExpr(exp->binx.lhs->arrayIndex.arrExpr);
					
					AppendCode(OP_SETINDEX);
				}
				else if(exp->binx.lhs->type == EXP_DOT)
				{
					if(strcmp(exp->binx.lhs->dotx.name, "pairs") == 0)
						ErrorExitE(exp, "Cannot assign dictionary entry 'pairs' to a value\n");
					
					CompileValueExpr(exp->binx.lhs->dotx.dict);
					AppendCode(OP_DICT_SET);
					AppendInt(RegisterString(exp->binx.lhs->dotx.name)->index);
				}
				else
					ErrorExitE(exp, "Left hand side of assignment operator '=' must be an assignable value (variable, array index, dictionary index) instead of %i\n", exp->binx.lhs->type);
			}
			else
				ErrorExitE(exp, "Invalid top level binary expression\n");
		} break;
		
		case EXP_CALL:
		{	
			CompileCallExpr(exp, 0);
		} break;
		
		case EXP_WHILE:
		{
			int loopPc = CodeLength;
			CompileValueExpr(exp->whilex.cond);
			AppendCode(OP_GOTOZ);
			// emplace integer of exit location here
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			CompileExprList(exp->whilex.bodyHead);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			int exitLoc = CodeLength;
			EmplaceInt(emplaceLoc, CodeLength);
			
			for(Patch* p = Patches; p != NULL; p = p->next)
			{
				if(p->type == PATCH_CONTINUE) EmplaceInt(p->loc, loopPc);
				else if(p->type == PATCH_BREAK) EmplaceInt(p->loc, exitLoc);
			}
			ClearPatches();
		} break;
		
		case EXP_FOR:
		{
			CompileExpr(exp->forx.init);
			int loopPc = CodeLength;
			
			CompileValueExpr(exp->forx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			CompileExprList(exp->forx.bodyHead);
			
			int continueLoc = CodeLength;

			CompileExpr(exp->forx.iter);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			int exitLoc = CodeLength;
			EmplaceInt(emplaceLoc, CodeLength);
		
			for(Patch* p = Patches; p != NULL; p = p->next)
			{
				if(p->type == PATCH_CONTINUE) EmplaceInt(p->loc, continueLoc);
				else if(p->type == PATCH_BREAK) EmplaceInt(p->loc, exitLoc);
			}
			ClearPatches();
		} break;
		
		case EXP_IF:
		{
			CompileValueExpr(exp->ifx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			CompileExprList(exp->ifx.bodyHead);
			
			AppendCode(OP_GOTO);
			int exitEmplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			EmplaceInt(emplaceLoc, CodeLength);
			if(exp->ifx.alt)
			{
				if(exp->ifx.alt->type != EXP_IF)
					CompileExprList(exp->ifx.alt);
				else
					CompileExpr(exp->ifx.alt);
			}
			EmplaceInt(exitEmplaceLoc, CodeLength);
		} break;
		
		case EXP_FUNC:
		{
			AppendCode(OP_GOTO);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			exp->funcx.decl->pc = CodeLength;
			
			if(strcmp(exp->funcx.decl->name, "_main") == 0) EntryPoint = CodeLength;
			for(int i = 0; i < exp->funcx.decl->numLocals; ++i)
				AppendCode(OP_PUSH_NULL);
			
			CompileExprList(exp->funcx.bodyHead);
			AppendCode(OP_RETURN);
			
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_RETURN:
		{
			if(exp->retExpr)
			{
				CompileValueExpr(exp->retExpr);
				AppendCode(OP_RETURN_VALUE);
			}
			else
				AppendCode(OP_RETURN);
		} break;
		
		case EXP_COLON:
		{
			ErrorExitE(exp, "Attempted to use ':' to index value inside dictionary; use '.' instead (or call the function you're indexing)\n");
		} break;
		
		case EXP_CONTINUE:
		{
			AppendCode(OP_GOTO);
			AddPatch(PATCH_CONTINUE, CodeLength);
			AllocatePatch(sizeof(int) / sizeof(Word));
		} break;
		
		case EXP_BREAK:
		{
			AppendCode(OP_GOTO);
			AddPatch(PATCH_BREAK, CodeLength);
			AllocatePatch(sizeof(int) / sizeof(Word));
		} break;
		
		case EXP_LINKED_BINARY_CODE:
		{
			for(int i = 0; i < exp->code.numFunctions; ++i)
			{
				exp->code.toBeRetargeted[i]->pc += CodeLength;
				if(strcmp(exp->code.toBeRetargeted[i]->name, "_main") == 0) EntryPoint = exp->code.toBeRetargeted[i]->pc;
			}
			
			AppendCode(OP_HALT);
			for(int i = 0; i < exp->code.length; ++i)
				AppendCode(exp->code.bytes[i]);
				
			free(exp->code.toBeRetargeted);
		} break;
		
		default:
			ErrorExitE(exp, "Invalid top level expression (expected non-value expression in place of %s)\n", ExprNames[exp->type]);
	}
}

void CompileExprList(Expr* head)
{
	for(Expr* node = head; node != NULL; node = node->next)
		CompileExpr(node);
}

void DebugExprList(Expr* head)
{
	for(Expr* node = head; node != NULL; node = node->next)
		DebugExpr(node);
	printf("\n");
}


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


Expr* ParseBinaryFile(FILE* bin, const char* fileName)
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
			FuncDecl* decl = DeclareFunction(functionNames[i]);
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
			externIndexTable[i] = RegisterExtern(externNames[i])->index;
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
}

const char* StandardSourceSearchPath = "C:\\Mint\\src\\";
const char* StandardLibSearchPath = "C:\\Mint\\lib\\";

int main(int argc, char* argv[])
{
	if(argc == 1)
		ErrorExit("no input files detected!\n");
	
	const char* outPath = NULL;

	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;

	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-o") == 0)
		{
			outPath = argv[++i];
		}
		else if(strcmp(argv[i], "-l") == 0)
		{		
			/*FILE* in = fopen(argv[++i], "rb");
			if(!in)
			{
				char buf[256];
				strcpy(buf, StandardLibSearchPath);
				strcat(buf, argv[i]);
				in = fopen(buf, "rb");
				if(!in)
					ErrorExit("Cannot open file '%s' for reading\n", argv[i]);
			}
			
			if(!exprHead)
			{
				exprHead = ParseBinaryFile(in, argv[i]);
				exprCurrent = exprHead;
			}
			else
			{
				exprCurrent->next = ParseBinaryFile(in, argv[i]);
				exprCurrent = exprCurrent->next;
			}
			
			fclose(in);*/
			
			// alright, deprecating binary file linking for now
			printf("cannot link binary files...\n");
		}
		else
		{
			FILE* in = fopen(argv[i], "r");
			if(!in)
			{
				char buf[256];
				strcpy(buf, StandardSourceSearchPath);
				strcat(buf, argv[i]);
				in = fopen(buf, "r");
				
				if(!in)
					ErrorExit("Cannot open file '%s' for reading\n", argv[i]);
			}
			LineNumber = 1;
			FileName = argv[i];
			
			GetNextToken(in);
			
			while(CurTok != TOK_EOF)
			{
				if(!exprHead)
				{
					exprHead = ParseExpr(in);
					exprCurrent = exprHead;
				}
				else
				{
					exprCurrent->next = ParseExpr(in);
					exprCurrent = exprCurrent->next;
				}
			}
			
			ResetLex = 1;
			fclose(in);
		}
	}					
	
	CompileExprList(exprHead);
	_AppendCode(OP_HALT, LineNumber, RegisterString(FileName)->index);
	
	FILE* out;
	
	if(!outPath)
		out = fopen("out.mb", "wb");
	else
		out = fopen(outPath, "wb");
	
	OutputCode(out);
	
	fclose(out);
	
	return 0;
}
