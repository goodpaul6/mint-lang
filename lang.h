// lang.h -- a language which compiles to mint vm bytecode
#ifndef MINT_LANG_H
#define MINT_LANG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "vm.h"

#define MAX_ID_NAME_LENGTH 64
#define MAX_SCOPES 32
#define MAX_ARGS 64
#define MAX_STRUCT_ELEMENTS 64
#define MAX_STRUCT_TRAITS 16

extern char ProduceDebugInfo;

void ErrorExit(const char* format, ...);
void Warn(const char* format, ...);

void AppendCode(Word code);
void AppendInt(int value);
void AllocatePatch(int length);

void EmplaceInt(int loc, int value);

void OutputCode(FILE* out);

struct _Expr;

typedef enum
{
	NUMBER,
	STRING,
	ARRAY,
	DICT,
	FUNC,
	NATIVE,
	VOID,
	DYNAMIC,
	USERTYPE,
	NUM_HINTS
} Hint;

extern const char* HintStrings[NUM_HINTS];

// TODO: don't store arguments in an array like that
typedef struct _TypeHint
{
	struct _TypeHint* next;
	Hint hint;
	
	// TODO: maybe don't use anonymous structs here
	union
	{
		// for indexable values
		struct _TypeHint* subType;
		// for usertypes
		struct
		{
			struct _TypeHint* traits[MAX_STRUCT_TRAITS];
			int numTraits;
			
			char isTrait;
			char name[MAX_ID_NAME_LENGTH];
			int numElements;
			struct _TypeHint* elements[MAX_STRUCT_ELEMENTS];  
			char* names[MAX_STRUCT_ELEMENTS];
		} user;
		// for funcs
		struct
		{
			struct _TypeHint* ret;
			int numArgs;
			struct _TypeHint* args[MAX_ARGS];
		} func;
	};
} TypeHint;

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
		char* string;
	};
} ConstDecl;

typedef struct _VarDecl
{
	struct _VarDecl* next;
	
	char isGlobal;
	char scope;
	char name[MAX_ID_NAME_LENGTH];
	int index;
	
	TypeHint* type;
} VarDecl;

typedef struct _Upvalue
{
	struct _Upvalue* next;
	VarDecl* decl;
} Upvalue;

typedef struct _FuncDecl
{	
	struct _FuncDecl* next;
	
	char inlined;
	char isAliased;
	char isExtern;
	
	char alias[MAX_ID_NAME_LENGTH];
	char name[MAX_ID_NAME_LENGTH];
	int index;
	int numLocals;
	Word numArgs;
	int pc;
	
	// type signature
	TypeHint* type;

	char hasEllipsis;
	
	// -1 = unknown, 0 = no return, 1 = returns value
	char hasReturn;
	
	char isLambda;
	
	// function which encloses the lambda
	struct _FuncDecl* prevDecl;
	// referenced upvalues (by lambda)
	Upvalue* upvalues;
	// env variable declaration (first argument to lambda)
	VarDecl* envDecl;
	// scope at which the function has been declared
	char scope;

	struct _Expr* bodyHead;
} FuncDecl;

ConstDecl* RegisterNumber(double number);
ConstDecl* RegisterString(const char* string);

void PushScope();
void PopScope();

FuncDecl* DeclareFunction(const char* name, int index);
FuncDecl* DeclareExtern(const char* name);

FuncDecl* EnterFunction(const char* name);
void ExitFunction();

FuncDecl* ReferenceFunction(const char* name);

VarDecl* RegisterVariable(const char* name);
VarDecl* RegisterArgument(const char* name);

VarDecl* ReferenceVariable(const char* name);

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
	TOK_LSHIFT = -22,
	TOK_RSHIFT = -23,
	TOK_NULL = -24,
	TOK_INLINE = -25,
	TOK_LAMBDA = -26,
	TOK_CADD = -27,
	TOK_CSUB = -28,
	TOK_CMUL = -29,
	TOK_CDIV = -30,
	TOK_CONTINUE = -31,
	TOK_BREAK = -32,
	TOK_ELLIPSIS = -33,
	TOK_AS = -34,
	TOK_DO = -35,
	TOK_THEN = -36,
	TOK_TYPE = -37,
	TOK_INST = -38,
	TOK_HAS = -39,
	TOK_TRAIT = -40,
	TOK_CAT = -41,
	TOK_IS = -42
};

extern size_t LexemeCapacity;
extern size_t LexemeLength;
extern char* Lexeme;
extern int CurTok;
extern char ResetLex;

int GetToken(FILE* in);

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
	EXP_FOR,
	EXP_DOT,
	EXP_DICT_LITERAL,
	EXP_NULL,
	EXP_CONTINUE,
	EXP_BREAK,
	EXP_COLON,
	EXP_LAMBDA,
	//EXP_LINKED_BINARY_CODE
	EXP_TYPE_DECL,
	EXP_TYPE_CAST,
	EXP_INST,
	NUM_EXPR_TYPES
} ExprType;

typedef struct _Expr
{
	struct _Expr* next;
	ExprType type;
	const char* file;
	int line;
	FuncDecl* curFunc;
	
	union
	{
		ConstDecl* constDecl; // NOTE: for both EXP_NUMBER and EXP_STRING
		struct { VarDecl* varDecl; char name[MAX_ID_NAME_LENGTH]; } varx; // NOTE: for both EXP_IDENT and EXP_VAR
		struct { struct _Expr* args[MAX_ARGS]; int numArgs; struct _Expr* func; } callx;
		struct { struct _Expr *lhs, *rhs; int op; } binx;
		struct _Expr* parenExpr;
		struct { struct _Expr *cond, *bodyHead; } whilex;
		struct { FuncDecl* decl; struct _Expr* bodyHead; } funcx;
		FuncDecl* extDecl;
		struct { struct _Expr *cond, *bodyHead, *alt; } ifx;
		struct { struct _Expr* exp; FuncDecl* decl; } retx;
		struct { struct _Expr* head; int length; } arrayx;
		struct { struct _Expr* arrExpr; struct _Expr* indexExpr; } arrayIndex;
		struct { int op; struct _Expr* expr; } unaryx;
		// comDecl: for array comprehensions
		struct { struct _Expr *init, *cond, *iter, *bodyHead; VarDecl* comDecl; } forx;
		struct { struct _Expr* dict; char name[MAX_ID_NAME_LENGTH]; char isColon; } dotx;
		struct { struct _Expr* pairsHead; int length; VarDecl* decl; } dictx;
		struct { struct _Expr* dict; char name[MAX_ID_NAME_LENGTH]; } colonx;
		struct { FuncDecl* decl; struct _Expr* bodyHead; VarDecl* dictDecl;} lamx;
		//struct { Word* bytes; int length; FuncDecl** toBeRetargeted; int* pcFileTable; int* pcLineTable; int numFunctions; } code;
		struct { struct _Expr* expr; TypeHint* newType; } castx;
		TypeHint* instType;
	};
} Expr;

void ErrorExitE(Expr* exp, const char* format, ...);
void WarnE(Expr* exp, const char* format, ...);

Expr* CreateExpr(ExprType type);
int GetNextToken(FILE* in);
Expr* ParseExpr(FILE* in);

extern const char* ExprNames[NUM_EXPR_TYPES];

char CompareTypes(const TypeHint* a, const TypeHint* b);
TypeHint* GetBroadTypeHint(Hint type);
TypeHint* ParseTypeHint(FILE* in);
void ResolveTypesExprList(Expr* head);
TypeHint* InferTypeFromExpr(Expr* exp);
const char* HintString(const TypeHint* type);
TypeHint* RegisterUserType(const char* name);
TypeHint* GetUserTypeElement(const TypeHint* type, const char* name);
int GetUserTypeElementIndex(const TypeHint* type, const char* name);
int GetUserTypeNumElements(const TypeHint* type);

typedef enum 
{
	PATCH_BREAK,
	PATCH_CONTINUE
} PatchType;

typedef struct _Patch
{
	struct _Patch* next;
	PatchType type;
	int loc;
} Patch;

void CompileExprList(Expr* head);

extern int LineNumber;
extern const char* FileName;

extern int NumNumbers;
extern int NumStrings;

extern int NumGlobals;
extern int NumFunctions;
extern int NumExterns;
extern int EntryPoint;

extern Word* Code;
extern int CodeCapacity;
extern int CodeLength;

extern ConstDecl* Constants;
extern ConstDecl* ConstantsCurrent;
 
extern FuncDecl* CurFunc;

extern FuncDecl* Functions;
extern FuncDecl* FunctionsCurrent;

extern VarDecl* VarStack[MAX_SCOPES];
extern int VarStackDepth;

#endif
