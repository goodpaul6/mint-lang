// lang.c -- a language which compiles to mint vm bytecode
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "vm.h"

#define MAX_LEX_LENGTH 256
#define MAX_NUM_CONSTS 256
#define MAX_VARS 256
#define MAX_ID_NAME_LENGTH 64
#define MAX_SCOPES 32
#define MAX_ARGS 32

double Constants[MAX_NUM_CONSTS];
int NumConstants = 0;
int NumGlobals = 0;
int NumFunctions = 0;
int NumExterns = 0;
int EntryPoint = 0;

Word* Code = NULL;
int CodeCapacity = 0;
int CodeLength = 0;

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

void EmplaceInt(int loc, int value)
{
	Word* code = (Word*)(&value);
	for(int i = 0; i < sizeof(int) / sizeof(Word); ++i)
		Code[loc + i] = *code++;
}

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

FuncDecl* CurFunc = NULL;
FuncDecl* Functions = NULL;
VarDecl* VarStack[MAX_SCOPES];
int VarStackDepth = 0;

/* BINARY FORMAT:
entry point as integer

program length (in words) as integer
program code (must be no longer than program length specified previously)

number of global variables as integer

number of functions as integer
function entry points as integers

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
	printf("Number of constants: %i\n", NumConstants);
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
	
	fwrite(&NumConstants, sizeof(int), 1, out);
	fwrite(Constants, sizeof(double), NumConstants, out);
	
	int numStrings = 0;
	fwrite(&numStrings, sizeof(int), 1, out);
}

int RegisterNumber(double number)
{
	for(int i = 0; i < NumConstants; ++i)
	{
		if(Constants[i] == number)
			return i;
	}
	
	Constants[NumConstants] = number;
	return NumConstants++;
}

FuncDecl* RegisterExtern(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(strcmp(decl->name, name) == 0)
		{
			fprintf(stderr, "Attempted to redeclare previously defined extern/func '%s' as extern\n", name);
			exit(1);
		}
	}
	
	FuncDecl* decl = malloc(sizeof(FuncDecl));
	assert(decl);
	
	decl->next = Functions;
	Functions = decl;
	
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

	decl->next = Functions;
	Functions = decl;
	
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
				fprintf(stderr, "Attempted to redeclare variable '%s'\n", name);
				exit(1);
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
	{
		fprintf(stderr, "(INTERNAL) Must be inside function when registering arguments\n");
		exit(1);
	}
	
	VarDecl* decl = RegisterVariable(name);
	decl->index = -numArgs + CurFunc->numArgs++;
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
	
	fprintf(stderr, "Attempted to reference undeclared identifier '%s'\n", name);
	exit(1);
	
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
};

char Lexeme[MAX_LEX_LENGTH];
int CurTok = 0;

int GetToken(FILE* in)
{
	static char last = ' ';
	
	while(isspace(last))
		last = getc(in);

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
	
	if(last == EOF)
		return TOK_EOF;
	
	int lastChar = last;
	last = getc(in);
	
	return lastChar;
}

typedef enum
{
	EXP_NUMBER,
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
} ExprType;

typedef struct _Expr
{
	struct _Expr* next;
	ExprType type;
	
	union
	{
		int numIndex;
		VarDecl* varDecl; // NOTE: for both EXP_IDENT and EXP_VAR
		struct { struct _Expr* args[MAX_ARGS]; int numArgs; char funcName[MAX_ID_NAME_LENGTH]; FuncDecl* decl; } callx;
		struct { struct _Expr *lhs, *rhs; int op; } binx;
		struct _Expr* parenExpr;
		struct { struct _Expr *cond, *bodyHead; } whilex;
		struct { FuncDecl* decl; struct _Expr* bodyHead; } funcx;
		FuncDecl* extDecl;
		struct { struct _Expr *cond, *bodyHead; } ifx;
		struct _Expr* retExpr;
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

Expr* ParseFactor(FILE* in)
{
	switch(CurTok)
	{
		case TOK_NUMBER:
		{
			Expr* exp = CreateExpr(EXP_NUMBER);
			exp->numIndex = RegisterNumber(strtod(Lexeme, NULL));
			
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
				exp->varDecl = ReferenceVariable(name);
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
				if(exp->callx.decl->numArgs != exp->callx.numArgs)
				{
					fprintf(stderr, "Function '%s' expected %i argument(s) but you gave it %i\n", name, exp->callx.decl->numArgs, exp->callx.numArgs);
					exit(1);
				}
			}
			GetNextToken(in);
			
			return exp;
		} break;
		
		case TOK_VAR:
		{
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
			{
				fprintf(stderr, "Expected ident after 'var' but received something else\n");
				exit(1);
			}
			
			Expr* exp = CreateExpr(EXP_VAR);
			exp->varDecl = RegisterVariable(Lexeme);
			
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
				fprintf(stderr, "Expected ')' after previous '('\n");
				exit(1);
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
		
		case TOK_FUNC:
		{
			GetNextToken(in);
			
			char name[MAX_ID_NAME_LENGTH];
			if(CurTok != TOK_IDENT)
			{
				fprintf(stderr, "Expected identifier after 'func'\n");
				exit(1);
			}
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			if(CurTok != '(')
			{
				fprintf(stderr, "Expected '(' after 'func' %s\n", name);
				exit(1);
			}
			GetNextToken(in);
			
			char args[MAX_ARGS][MAX_ID_NAME_LENGTH];
			int numArgs = 0;
			
			while(CurTok != ')')
			{
				if(CurTok != TOK_IDENT)
				{
					fprintf(stderr, "Expected identifier in argument list for function '%s' but received something else\n", name);
					exit(1);
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
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_IF);
			
			exp->ifx.cond = ParseExpr(in);
			
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
			
			PopScope();
			
			exp->ifx.bodyHead = exprHead;
			
			return exp;
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
			
				exp->retExpr = NULL;
				return exp;	
			}
			
			fprintf(stderr, "WHAT ARE YOU TRYING TO RETURN FROM! YOU'RE NOT EVEN INSIDE A FUNCTION BODY!\n");
			exit(1);
			
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
	
	fprintf(stderr, "Unexpected character '%c'\n", CurTok);
	exit(1);
	
	return NULL;
}

int GetTokenPrec()
{
	int prec = -1;
	switch(CurTok)
	{
		case '*': case '/': case '%': case '&': case '|': prec = 5; break;
		
		case '+': case '-': prec = 4; break;
		
		case '<': case '>': prec = 3; break;
		
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

		Expr* rhs = ParseFactor(in);
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
	Expr* factor = ParseFactor(in);
	return ParseBinRhs(in, 0, factor);
}

void DebugExprList(Expr* head);
void DebugExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER:
		{
			printf("%g", Constants[exp->numIndex]);
		} break;
		
		case EXP_IDENT:
		{
			printf("%s", exp->varDecl->name);
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
			printf("var %s", exp->varDecl->name);
		} break;
		
		case EXP_BIN:
		{
			DebugExpr(exp->binx.lhs);
			printf("%c", exp->binx.op);
			DebugExpr(exp->binx.rhs);
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

void CompileExprList(Expr* head);
void CompileExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_NUMBER:
		{
			AppendCode(OP_PUSH);
			AppendCode(0);
			AppendInt(exp->numIndex);
		} break;
		
		case EXP_IDENT:
		{
			if(exp->varDecl->isGlobal)
			{
				AppendCode(OP_GET);
				AppendInt(exp->varDecl->index);
			}
			else
			{
				AppendCode(OP_GETLOCAL);
				AppendInt(exp->varDecl->index);
			}
		} break;
		
		case EXP_CALL:
		{
			if(!exp->callx.decl)
			{
				FuncDecl* decl = ReferenceFunction(exp->callx.funcName);
				if(!decl)
				{
					fprintf(stderr, "Attempted to call undeclared/undefined function '%s'\n", exp->callx.funcName);
					exit(1);
				}
				
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
		} break;
		
		case EXP_BIN:
		{
			CompileExpr(exp->binx.rhs);
			
			if(exp->binx.op == '=')
			{
				if(exp->binx.lhs->type == EXP_VAR || exp->binx.lhs->type == EXP_IDENT)
				{
					if(exp->binx.lhs->varDecl->isGlobal) AppendCode(OP_SET);
					else AppendCode(OP_SETLOCAL);
					AppendInt(exp->binx.lhs->varDecl->index);
				}
				else
				{
					fprintf(stderr, "Left hand side of assignment operator '=' must be a variable\n");
					exit(1);
				}
			}
			else
			{
				CompileExpr(exp->binx.rhs);
				CompileExpr(exp->binx.lhs);
				
				switch(exp->binx.op)
				{
					case '+': AppendCode(OP_ADD); break;
					case '-': AppendCode(OP_SUB); break;
					case '*': AppendCode(OP_MUL); break;
					case '/': AppendCode(OP_DIV); break;
					case '%': AppendCode(OP_MOD); break;
					case '<': AppendCode(OP_LT); break;
					case '>': AppendCode(OP_GT); break;
					
					default:
						fprintf(stderr, "Unsupported binary operator %c\n", exp->binx.op);
						exit(1);
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
			CodeLength += sizeof(int) / sizeof(Word);
			CompileExprList(exp->whilex.bodyHead);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_IF:
		{
			CompileExpr(exp->ifx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			CodeLength += sizeof(int) / sizeof(Word);
			CompileExprList(exp->ifx.bodyHead);
			EmplaceInt(emplaceLoc, CodeLength);
		} break;
		
		case EXP_FUNC:
		{
			AppendCode(OP_GOTO);
			int emplaceLoc = CodeLength;
			CodeLength += sizeof(int) / sizeof(Word);
			
			exp->funcx.decl->pc = CodeLength;
			
			if(strcmp(exp->funcx.decl->name, "_main") == 0) EntryPoint = CodeLength;
			for(int i = 0; i < exp->funcx.decl->numLocals; ++i)
			{
				AppendCode(OP_PUSH);
				AppendCode(0);
				AppendInt(RegisterNumber(0));
			}
			
			CompileExprList(exp->funcx.bodyHead);
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
	GetNextToken(stdin);
	
	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;
	
	while(CurTok != TOK_EOF)
	{
		if(!exprHead)
		{
			exprHead = ParseExpr(stdin);
			exprCurrent = exprHead;
		}
		else
		{
			exprCurrent->next = ParseExpr(stdin);
			exprCurrent = exprCurrent->next;
		}
	}
	
	CompileExprList(exprHead);
	AppendCode(OP_HALT);
	DebugExprList(exprHead);
	
	FILE* out = fopen("out.mb", "wb");
	OutputCode(out);
	
	fclose(out);
	
	return 0;
}
