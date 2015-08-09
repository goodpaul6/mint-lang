// lang.h -- a language which compiles to mint vm bytecode

#include "vm.h"

#define MAX_ID_NAME_LENGTH 64
#define MAX_SCOPES 32
#define MAX_ARGS 64

typedef enum
{
	NUMBER,
	STRING,
	ARRAY,
	DICT,
	FUNC,
	NATIVE,
	VOID,
	DYNAMIC
} HintType;

typedef struct _TypeHint
{
	struct _TypeHint* next;
	HintType hint;
	
	// for indexable values
	const struct _TypeHint* subType;
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

struct _Expr;

typedef struct _VarDecl
{
	struct _VarDecl* next;
	
	char isGlobal;
	char scope;
	char name[MAX_ID_NAME_LENGTH];
	int index;
	
	const TypeHint* type;
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
	
	const TypeHint* argTypes;
	const TypeHint* returnType;
	
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
} FuncDecl;

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
	TOK_NEW = -34,
	TOK_AS = -35
};

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
	EXP_NEW,
	EXP_LAMBDA,
	EXP_LINKED_BINARY_CODE
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
		struct _Expr* retExpr;
		struct { struct _Expr* head; int length; } arrayx;
		struct { struct _Expr* arrExpr; struct _Expr* indexExpr; } arrayIndex;
		struct { int op; struct _Expr* expr; } unaryx;
		struct { struct _Expr *init, *cond, *iter, *bodyHead; } forx;
		struct { struct _Expr* dict; char name[MAX_ID_NAME_LENGTH]; } dotx;
		struct { struct _Expr* pairsHead; int length; VarDecl* decl; } dictx;
		struct { struct _Expr* dict; char name[MAX_ID_NAME_LENGTH]; } colonx;
		struct _Expr* newExpr;
		struct { FuncDecl* decl; struct _Expr* bodyHead; VarDecl* dictDecl;} lamx;
		struct { Word* bytes; int length; FuncDecl** toBeRetargeted; int* pcFileTable; int* pcLineTable; int numFunctions; } code;
	};
} Expr;

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

