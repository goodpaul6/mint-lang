// lang.c -- a language which compiles to mint vm bytecode
/* 
 * TODO:
 * - logical and/or (BUGGED RIGHT NOW: Weird order of operation? Need parentheses)
 * - fix error message line numbers (store line numbers in expressions)
 * 
 * PARTIALLY COMPLETE (I.E NOT FULLY SATISFIED WITH SOLUTION):
 * - proper operators: need more operators
 * - include/require other scripts (copy bytecode into vm at runtime?): can only include things at compile time (with cmd line)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "vm.h"

#define MAX_LEX_LENGTH 256
#define MAX_ID_NAME_LENGTH 64
#define MAX_SCOPES 32
#define MAX_ARGS 32 

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

void ErrorExit(const char* format, ...)
{
	fprintf(stderr, "Error (%s:%i): ", FileName, LineNumber);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	
	exit(1);
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
	{
		ErrorExit("Compiler error: attempted to emplace in outside of code array\n");
		
	}
	
	Word* code = (Word*)(&value);
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		Code[loc + i] = *code++;
}

typedef enum
{
	CONST_NUM,
	CONST_STR,
} ConstType;

typedef struct _ConstDecl
{
	struct _ConstDecl* next;
	ConstType type;
	int index;
	
	union
	{
		double number;
		char string[MAX_LEX_LENGTH];
	};
} ConstDecl;

typedef struct _FuncDecl
{	
	struct _FuncDecl* next;
	
	char isExtern;
	
	char name[MAX_ID_NAME_LENGTH];
	int index;
	int numLocals;
	Word numArgs;
	int pc;
} FuncDecl;

typedef struct _VarDecl
{
	struct _VarDecl* next;
	
	char isGlobal;
	char name[MAX_ID_NAME_LENGTH];
	int index;
} VarDecl;

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
	
	fwrite(&NumFunctions, sizeof(int), 1, out);	
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(!decl->isExtern)
			fwrite(&decl->pc, sizeof(int), 1, out);
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
	strcpy(decl->string, string);
	decl->index = NumStrings++;
	
	return decl;
}

FuncDecl* RegisterExtern(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(strcmp(decl->name, name) == 0)
			return decl;
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
	
	decl->isExtern = 1;
	strcpy(decl->name, name);
	
	decl->index = NumExterns++;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	
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

FuncDecl* EnterFunction(const char* name)
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
	
	decl->isExtern = 0;
	strcpy(decl->name, name);
	decl->index = NumFunctions++;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	
	CurFunc = decl;
	
	return decl;
}

void ExitFunction()
{
	CurFunc = NULL;
}

FuncDecl* ReferenceFunction(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(strcmp(decl->name, name) == 0)
			return decl;
	}
	
	return NULL;
}

VarDecl* RegisterVariable(const char* name)
{
	for(int i = VarStackDepth; i >= 0; --i)
	{
		for(VarDecl* decl = VarStack[i]; decl != NULL; decl = decl->next)
		{
			if(strcmp(decl->name, name) == 0)
			{					
				ErrorExit("Attempted to redeclare variable '%s'\n", name);
				
			}
		}
	}
	
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
	
	return decl;
}

void RegisterArgument(const char* name, int numArgs)
{
	if(!CurFunc)
		ErrorExit("(INTERNAL) Must be inside function when registering arguments\n");
	
	VarDecl* decl = RegisterVariable(name);
	decl->index = -(++CurFunc->numArgs);
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

enum
{
	TOK_IDENT = -1,
	TOK_NUMBER = -2,
	TOK_EOF = -3,
	TOK_VAR = -4,
	TOK_WHILE = -5,
	TOK_END = -6,
	TOK_FUNC = -7,
	TOK_IF = -8,
	TOK_RETURN = -9,
	TOK_EXTERN = -10,
	TOK_STRING = -11,
	TOK_EQUALS = -12,
	TOK_LTE = -13,
	TOK_GTE = -14,
	TOK_NOTEQUAL = -15,
	TOK_HEXNUM = -16,
	TOK_FOR = -17,
	TOK_ELSE = -18,
	TOK_ELIF = -19,
	TOK_AND = -20,
	TOK_OR = -21,
};

char Lexeme[MAX_LEX_LENGTH];
int CurTok = 0;
char ResetLex = 0;

int GetToken(FILE* in)
{
	static char last = ' ';
	
	if(ResetLex)
	{
		last = ' ';
		ResetLex = 0;
	}
	
	while(isspace(last))
	{
		if(last == '\n') ++LineNumber;
		last = getc(in);
	}
	
	if(isalpha(last) || last == '_')
	{
		int i = 0;
		while(isalnum(last) || last == '_')
		{
			Lexeme[i++] = last;
			last = getc(in);
		}
		Lexeme[i] = '\0';
		
		if(strcmp(Lexeme, "var") == 0) return TOK_VAR;
		if(strcmp(Lexeme, "while") == 0) return TOK_WHILE;
		if(strcmp(Lexeme, "end") == 0) return TOK_END;
		if(strcmp(Lexeme, "func") == 0) return TOK_FUNC;
		if(strcmp(Lexeme, "if") == 0) return TOK_IF;
		if(strcmp(Lexeme, "return") == 0) return TOK_RETURN;
		if(strcmp(Lexeme, "extern") == 0) return TOK_EXTERN;
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
		
		return TOK_IDENT;
	}
		
	if(isdigit(last))
	{	
		int i = 0;
		while(isdigit(last) || last == '.')
		{
			Lexeme[i++] = last;
			last = getc(in);
		}
		Lexeme[i] = '\0';
		
		return TOK_NUMBER;
	}

	if(last == '$')
	{
		last = getc(in);
		int i = 0;
		while(isxdigit(last))
		{
			Lexeme[i++] = last;
			last = getc(in);
		}
		Lexeme[i] = '\0';
		
		return TOK_HEXNUM;
	}
	
	if(last == '"')
	{
		int i = 0;
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
				}
			}
			
			Lexeme[i++] = last;
			last = getc(in);
		}
		Lexeme[i] = '\0';
		last = getc(in);
		
		return TOK_STRING;
	}
	
	if(last == '#')
	{
		last = getc(in);
		while(last != '\n' && last != '\r' && last != EOF)
			last = getc(in);
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
	
	return lastChar;
}

typedef enum
{
	EXP_NUMBER,
	EXP_STRING,
	EXP_IDENT,
	EXP_CALL,
	EXP_VAR,
	EXP_BIN,
	EXP_PAREN,
	EXP_WHILE,
	EXP_FUNC,
	EXP_IF,
	EXP_RETURN,
	EXP_EXTERN,
	EXP_ARRAY_LITERAL,
	EXP_ARRAY_INDEX,
	EXP_UNARY,
	EXP_FOR
} ExprType;

typedef struct _Expr
{
	struct _Expr* next;
	ExprType type;
	
	union
	{
		ConstDecl* constDecl; // NOTE: for both EXP_NUMBER and EXP_STRING
		struct { VarDecl* varDecl; char name[MAX_ID_NAME_LENGTH]; } varx; // NOTE: for both EXP_IDENT and EXP_VAR
		struct { struct _Expr* args[MAX_ARGS]; int numArgs; char funcName[MAX_ID_NAME_LENGTH]; FuncDecl* decl; } callx;
		struct { struct _Expr *lhs, *rhs; int op; } binx;
		struct _Expr* parenExpr;
		struct { struct _Expr *cond, *bodyHead; } whilex;
		struct { FuncDecl* decl; struct _Expr* bodyHead; } funcx;
		FuncDecl* extDecl;
		struct { struct _Expr *cond, *bodyHead, *alt; } ifx;
		struct _Expr* retExpr;
		struct _Expr* lengthExpr;
		struct { struct _Expr* arrExpr; struct _Expr* indexExpr; } arrayIndex;
		struct { int op; struct _Expr* expr; } unaryx;
		struct { struct _Expr *init, *cond, *iter, *bodyHead; } forx;
	};
} Expr;

int GetNextToken(FILE* in)
{
	CurTok = GetToken(in);
	return CurTok;
}

Expr* CreateExpr(ExprType type)
{
	Expr* exp = malloc(sizeof(Expr));
	assert(exp);
	
	exp->next = NULL;
	exp->type = type;

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
		
		case '[':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_ARRAY_LITERAL);
			exp->lengthExpr = ParseExpr(in);
		
			if(CurTok != ']')
				ErrorExit("Expected ']' after previous '['\n");
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_IDENT:
		{
			char name[MAX_ID_NAME_LENGTH];
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			if(CurTok != '(')
			{
				Expr* exp = CreateExpr(EXP_IDENT);
				exp->varx.varDecl = ReferenceVariable(name);
				strcpy(exp->varx.name, name);
				return exp;
			}
			
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_CALL);
			
			exp->callx.numArgs = 0;
			strcpy(exp->callx.funcName, name);
			
			// NOTE: If the function's undeclared, the compiler will search again during the compilation phase
			// and if it still cannot be found, the compiler will give an error prompt
			exp->callx.decl = ReferenceFunction(name);
			
			while(CurTok != ')')
			{
				exp->callx.args[exp->callx.numArgs++] = ParseExpr(in);
				if(CurTok == ',') GetNextToken(in);
			}
			
			if(exp->callx.decl && !exp->callx.decl->isExtern)
			{
				// NOTE: Kinda hacky fix to compiler throwing errors when using expand
				char isExpanded = 0;
				for(int i = 0; i < exp->callx.numArgs; ++i)
				{
					if(exp->callx.args[i]->type == EXP_CALL)
					{
						if(strcmp(exp->callx.args[i]->callx.funcName, "expand") == 0)
							isExpanded = 1;
					}
				}
				
				if(!isExpanded)
				{
					if(exp->callx.decl->numArgs != exp->callx.numArgs)
						ErrorExit("Function '%s' expected %i argument(s) but you gave it %i\n", name, exp->callx.decl->numArgs, exp->callx.numArgs);
				}
			}
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_VAR:
		{
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected ident after 'var' but received something else\n");
						
			Expr* exp = CreateExpr(EXP_VAR);
			exp->varx.varDecl = RegisterVariable(Lexeme);
			strcpy(exp->varx.name, Lexeme);
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case '(':
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_PAREN);
			exp->parenExpr = ParseExpr(in);
			
			if(CurTok != ')')
			{
				ErrorExit("Expected ')' after previous '('\n");
				
			}
			
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_WHILE:
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_WHILE);
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
			GetNextToken(in);
			
			PushScope();
			Expr* exp = CreateExpr(EXP_FOR);
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
			GetNextToken(in);
			
			char name[MAX_ID_NAME_LENGTH];
			if(CurTok != TOK_IDENT)
			{
				ErrorExit("Expected identifier after 'func'\n");
				
			}
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			if(CurTok != '(')
			{
				ErrorExit("Expected '(' after 'func' %s\n", name);
				
			}
			GetNextToken(in);
			
			char args[MAX_ARGS][MAX_ID_NAME_LENGTH];
			int numArgs = 0;
			
			while(CurTok != ')')
			{
				if(CurTok != TOK_IDENT)
				{
					ErrorExit("Expected identifier in argument list for function '%s' but received something else\n", name);
					
				}
				
				strcpy(args[numArgs++], Lexeme);
				GetNextToken(in);
				
				if(CurTok == ',') GetNextToken(in);
			}
			GetNextToken(in);
			
			PushScope();
			FuncDecl* decl = EnterFunction(name);
			
			for(int i = 0; i < numArgs; ++i)
				RegisterArgument(args[i], numArgs);
			
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
			ExitFunction();
			PopScope();
			
			Expr* exp = CreateExpr(EXP_FUNC);
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
				GetNextToken(in);
				Expr* exp = CreateExpr(EXP_RETURN);
				if(CurTok != ';')
				{
					exp->retExpr = ParseExpr(in);
					return exp;
				}
				GetNextToken(in);
			
				exp->retExpr = NULL;
				return exp;	
			}
			
			ErrorExit("WHAT ARE YOU TRYING TO RETURN FROM! YOU'RE NOT EVEN INSIDE A FUNCTION BODY!\n");
			
			
			return NULL;
		} break;
		
		case TOK_EXTERN:
		{
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_EXTERN);
			exp->extDecl = RegisterExtern(Lexeme);
			
			GetNextToken(in);
			
			return exp;
		} break;
	};
	
	ErrorExit("Unexpected token '%c' (%i) (%s)\n", CurTok, CurTok, Lexeme);
	
	
	return NULL;
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
			if(CurTok != ']')
				ErrorExit("Expected ']' after previous '['\n");
			GetNextToken(in);
			if(CurTok != '[') return exp;
			else ParsePost(in, exp);
		} break;
		
		default:
			return pre;
	}
}

Expr* ParseUnary(FILE* in)
{
	switch(CurTok)
	{
		case '-': case '!': case '@':
		{
			int op = CurTok;
			
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_UNARY);
			exp->unaryx.op = op;
			exp->unaryx.expr = ParseExpr(in);
			
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
		case '*': case '/': case '%': case '&': case '|': prec = 6; break;
		
		case '+': case '-': prec = 5; break;
	
		case '<': case '>': case TOK_LTE: case TOK_GTE:	prec = 4; break;
	
		case TOK_EQUALS: case TOK_NOTEQUAL: prec = 3; break;
		
		case TOK_AND: case TOK_OR: prec = 2; break;
			
		case '=': prec = 1; break;
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
	return ParseBinRhs(in, 0, unary);
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
			printf("%s(", exp->callx.funcName);
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
	}
}

void CompileExpr(Expr* exp);
char CompileIntrinsic(Expr* exp)
{
	if(strcmp(exp->callx.funcName, "len") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExit("Intrinsic 'len' only takes 1 argument\n");
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_LENGTH);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "write") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExit("Intrinsic 'write' only takes 1 argument\n");
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_WRITE);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "read") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExit("Intrinsic 'read' takes no arguments\n");
		AppendCode(OP_READ);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "push") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExit("Intrinsic 'push' only takes 2 arguments\n");
		CompileExpr(exp->callx.args[1]);
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_PUSH);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "pop") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExit("Intrinsic 'push' only takes 1 argument\n");
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_POP);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "clear") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExit("Intrinsic 'clear' only takes 1 argument\n");
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_CLEAR);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "call") == 0)
	{
		if(exp->callx.numArgs < 1)
			ErrorExit("Intrinsic 'call' takes at least 1 argument (the function reference)\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileExpr(exp->callx.args[i]);
		AppendCode(OP_CALLP);
		AppendCode(exp->callx.numArgs - 1);
		return 1;
	}
	else if(strcmp(exp->callx.funcName, "expand") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExit("Intrinsic 'expand' takes only 1 argument\n");
		
		CompileExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_EXPAND);
		return 1;
	}
	
	return 0;
}

void CompileExprList(Expr* head);
void CompileExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER: case EXP_STRING:
		{
			AppendCode(exp->constDecl->type == CONST_NUM ? OP_PUSH_NUMBER : OP_PUSH_STRING);
			AppendInt(exp->constDecl->index);
		} break;
		
		case EXP_UNARY:
		{
			switch(exp->unaryx.op)
			{
				case '-': CompileExpr(exp->unaryx.expr); AppendCode(OP_NEG); break;
				case '!': CompileExpr(exp->unaryx.expr); AppendCode(OP_LOGICAL_NOT); break;
				case '@':
				{
					Expr* func = exp->unaryx.expr;
					if(func->type == EXP_IDENT)
					{
						if(func->varx.varDecl)
							ErrorExit("Cannot reference variable '%s' using '@' operator\n", func->varx.name);
						FuncDecl* decl = ReferenceFunction(func->varx.name);
						if(!decl)
							ErrorExit("Attempted to reference non-existent function '%s'\n", func->varx.name);
						
						AppendCode(OP_PUSH_FUNC);
						AppendCode(decl->isExtern);
						AppendInt(decl->index);
					}
					else
						ErrorExit("Cannot apply '@' operator to anything but an identifier\n");
				} break;
			}
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			CompileExpr(exp->lengthExpr);
			AppendCode(OP_CREATE_ARRAY);
		} break;
		
		case EXP_IDENT:
		{
			if(!exp->varx.varDecl)
			{
				exp->varx.varDecl = ReferenceVariable(exp->varx.name);
				if(!exp->varx.varDecl)	
					ErrorExit("Attempted to reference undeclared identifier '%s'\n", exp->varx.name);
			}
			
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
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			CompileExpr(exp->arrayIndex.indexExpr);
			CompileExpr(exp->arrayIndex.arrExpr);
			
			AppendCode(OP_GETINDEX);
		} break;
		
		case EXP_CALL:
		{
			if(!CompileIntrinsic(exp))
			{
				if(!exp->callx.decl)
				{
					FuncDecl* decl = ReferenceFunction(exp->callx.funcName);
					if(!decl)
						ErrorExit("Attempted to call undeclared/undefined function '%s'\n", exp->callx.funcName);
					
					exp->callx.decl = decl;
				}
			
				for(int i = exp->callx.numArgs - 1; i >= 0; --i)
					CompileExpr(exp->callx.args[i]);
		
				
				if(!exp->callx.decl->isExtern)
				{
					AppendCode(OP_CALL);
					AppendCode(exp->callx.numArgs);
				}
				else
					AppendCode(OP_CALLF);
				
				AppendInt(exp->callx.decl->index);
			}
		} break;
		
		case EXP_BIN:
		{
			if(exp->binx.op == '=')
			{
				CompileExpr(exp->binx.rhs);
				
				if(exp->binx.lhs->type == EXP_VAR || exp->binx.lhs->type == EXP_IDENT)
				{
					if(!exp->binx.lhs->varx.varDecl)
					{
						exp->binx.lhs->varx.varDecl = ReferenceVariable(exp->binx.lhs->varx.name);
						if(!exp->binx.lhs->varx.varDecl)	
							ErrorExit("Attempted to reference undeclared identifier '%s'\n", exp->binx.lhs->varx.name);
					}
				
					if(exp->binx.lhs->varx.varDecl->isGlobal)
					{
						if(exp->binx.lhs->type == EXP_VAR) printf("Warning: assignment operations to global variables will not execute unless they are inside the entry point\n");
						AppendCode(OP_SET);
					}
					else AppendCode(OP_SETLOCAL);
				
					AppendInt(exp->binx.lhs->varx.varDecl->index);
				}
				else if(exp->binx.lhs->type == EXP_ARRAY_INDEX)
				{
					CompileExpr(exp->binx.lhs->arrayIndex.indexExpr);
					CompileExpr(exp->binx.lhs->arrayIndex.arrExpr);
					
					AppendCode(OP_SETINDEX);
				}
				else
					ErrorExit("Left hand side of assignment operator '=' must be a variable\n");
			}
			else
			{
				CompileExpr(exp->binx.lhs);
				CompileExpr(exp->binx.rhs);
				
				switch(exp->binx.op)
				{
					case '+': AppendCode(OP_ADD); break;
					case '-': AppendCode(OP_SUB); break;
					case '*': AppendCode(OP_MUL); break;
					case '/': AppendCode(OP_DIV); break;
					case '%': AppendCode(OP_MOD); break;
					case '<': AppendCode(OP_LT); break;
					case '>': AppendCode(OP_GT); break;
					case TOK_EQUALS: AppendCode(OP_EQU); break;
					case TOK_LTE: AppendCode(OP_LTE); break;
					case TOK_GTE: AppendCode(OP_GTE); break;
					case TOK_NOTEQUAL: AppendCode(OP_NEQU); break;
					case TOK_AND: AppendCode(OP_LOGICAL_AND); break;
					case TOK_OR: AppendCode(OP_LOGICAL_OR); break;
					
					default:
						ErrorExit("Unsupported binary operator %c\n", exp->binx.op);
						
				}
			}
		} break;
		
		case EXP_PAREN:
		{
			CompileExpr(exp->parenExpr);
		} break;
		
		case EXP_WHILE:
		{
			int loopPc = CodeLength;
			CompileExpr(exp->whilex.cond);
			AppendCode(OP_GOTOZ);
			// emplace integer of exit location here
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			CompileExprList(exp->whilex.bodyHead);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_FOR:
		{
			CompileExpr(exp->forx.init);
			int loopPc = CodeLength;
			
			CompileExpr(exp->forx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			CompileExprList(exp->forx.bodyHead);
			
			CompileExpr(exp->forx.iter);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_IF:
		{
			CompileExpr(exp->ifx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			CompileExprList(exp->ifx.bodyHead);
			EmplaceInt(emplaceLoc, CodeLength);
		
			if(exp->ifx.alt)
			{
				if(exp->ifx.alt->type != EXP_IF)
					CompileExprList(exp->ifx.alt);
				else
					CompileExpr(exp->ifx.alt);
			}
		} break;
		
		case EXP_FUNC:
		{
			AppendCode(OP_GOTO);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			exp->funcx.decl->pc = CodeLength;
			
			if(strcmp(exp->funcx.decl->name, "_main") == 0) EntryPoint = CodeLength;
			for(int i = 0; i < exp->funcx.decl->numLocals; ++i)
			{
				AppendCode(OP_PUSH_NUMBER);
				AppendInt(RegisterNumber(0)->index);
			}
			
			CompileExprList(exp->funcx.bodyHead);
			AppendCode(OP_RETURN);
			
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_RETURN:
		{
			if(exp->retExpr)
			{
				CompileExpr(exp->retExpr);
				AppendCode(OP_RETURN_VALUE);
			}
			else
				AppendCode(OP_RETURN);
		} break;
		
		default:
		{
			// NO CODE GENERATION NECESSARY
		} break;
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

int main(int argc, char* argv[])
{
	if(argc == 1)
		ErrorExit("Error: no input files detected!\n");
	
	const char* outPath = NULL;

	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;

	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-o") == 0)
		{
			outPath = argv[++i];
		}
		else
		{
			FILE* in = fopen(argv[i], "r");
			if(!in)
				ErrorExit("Cannot open file '%s' for reading\n", argv[i]);
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
	AppendCode(OP_HALT);
	
	FILE* out;
	
	if(!outPath)
		out = fopen("out.mb", "wb");
	else
		out = fopen(outPath, "wb");
	
	OutputCode(out);
	
	fclose(out);
	
	return 0;
}
