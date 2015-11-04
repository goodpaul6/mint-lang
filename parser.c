#include "lang.h"

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
	"EXP_MACRO_CALL",
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
	"EXP_LAMBDA",
	"EXP_TYPE_DECL",
	"EXP_TYPE_CAST",
	"EXP_MULTI"
};

Expr* CreateExpr(ExprType type)
{
	Expr* exp = malloc(sizeof(Expr));
	assert(exp);

	exp->compiled = 0;
	exp->next = NULL;
	exp->type = type;
	exp->file = FileName;
	exp->line = LineNumber;
	exp->curFunc = CurFunc;
	exp->varScope = VarScope;
	exp->pc = -1;

	return exp;
}

Expr* ParseExpr(FILE* in);

Expr* ParseIf(FILE* in)
{
	GetNextToken(in);
	
	Expr* exp = CreateExpr(EXP_IF);
	
	Expr* cond = ParseExpr(in);
	
	if(CurTok != TOK_THEN)
		ErrorExit("Expected 'then' after if condition\n");
	GetNextToken(in);

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

	exprCurrent->next = NULL;
	
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
		
		case TOK_NULL:
		{
			GetNextToken(in);
			Expr* exp = CreateExpr(EXP_NULL);
			return exp;
		} break;

		case TOK_TYPE:
		{
			GetNextToken(in);
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identifier after 'type'\n");

			TypeHint* type = RegisterUserType(Lexeme);
			GetNextToken(in);

			if(CurTok == TOK_HAS)
			{
				GetNextToken(in);

				if(type->user.numElements != 0)
					ErrorExit("Attempted to incorporate another type into type '%s' which already had previous fields declared\n", type->user.name);

				TypeHint* parent = ParseTypeHint(in);
				if(parent->hint != USERTYPE)
					ErrorExit("Attempted to incorporate type '%s' (which is not a usertype) into type '%s'\n", HintString(parent), type->user.name);

				type->user.numElements += parent->user.numElements;
				if(type->user.numElements >= MAX_STRUCT_ELEMENTS)
					ErrorExit("Exceeded maximum element types in type declaration\n");

				for(int i = 0; i < type->user.numElements; ++i)
				{
					type->user.names[i] = malloc(strlen(parent->user.names[i]) + 1);
					assert(type->user.names[i]);
					strcpy(type->user.names[i], parent->user.names[i]);
					type->user.elements[i] = parent->user.elements[i];
				}
			}
			
			while(CurTok != TOK_END)
			{
				if(CurTok != TOK_IDENT)
					ErrorExit("Expected identifier in type declaration '%s'\n", type->user.name);
				
				if(type->user.numElements + 1 >= MAX_STRUCT_ELEMENTS)
					ErrorExit("Exceeded maximum number of usertype elements in type declaration '%s'\n", type->user.name);

				type->user.names[type->user.numElements] = malloc(strlen(Lexeme) + 1);
				assert(type->user.names[type->user.numElements]);
				strcpy(type->user.names[type->user.numElements], Lexeme);

				GetNextToken(in);
				if(CurTok != ':')
					ErrorExit("Expected ':' after identifier in type declaration '%s'\n", type->user.name);
				GetNextToken(in);
				TypeHint* elementType = ParseTypeHint(in);
				type->user.elements[type->user.numElements++] = elementType;
			}
			
			GetNextToken(in);

			return CreateExpr(EXP_TYPE_DECL);
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
			
			exp->dictx.decl = DeclareVariable(nameBuf);
			
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
				if(CurFunc->what == DECL_LAMBDA)
				{
					if(decl && !decl->isGlobal && decl->scope <= CurFunc->scope)
					{
						FuncDecl* funcDecl = CurFunc;
						
						// the environment dictionary
						Expr* dict = CreateExpr(EXP_IDENT);
						dict->varx.varDecl = funcDecl->envDecl;
						strcpy(dict->varx.name, funcDecl->envDecl->name);
						
						// the lambda which should have this variable as an upvalue
						FuncDecl* highestDecl = funcDecl;
						
						Expr* upvalueAcc = CreateExpr(EXP_DOT);
						
						funcDecl = funcDecl->prevDecl;
						
						// basically, reach up to the highest lambda
						// and retrieve its env. For instance, if the env
						// was a lambda up:
						// (anonymous env decl).env.(variable name)
						while(funcDecl && funcDecl->what == DECL_LAMBDA && decl->scope <= funcDecl->scope)
						{	
							Expr* acc = CreateExpr(EXP_DOT);
							acc->dotx.dict = dict;
							// NOTE: Cannot index dictionaries with empty strings so all
							// nested environments store a reference to the previous
							// environment under the name "env"
							strcpy(acc->dotx.name, "env");
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
			
			exp->varx.varDecl = DeclareVariable(Lexeme);
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
		
			if(CurTok != TOK_DO)
				ErrorExit("Expected 'do' after while condition\n");
			GetNextToken(in);

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
			
			static int comIndex = 0;
			static char buf[256];

			sprintf(buf, "__ac%i__", comIndex++);
			exp->forx.comDecl = DeclareVariable(buf);

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
			
			if(CurTok != TOK_DO)
				ErrorExit("Expected 'do' after for iterator expression\n");
			GetNextToken(in);

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
		
		case TOK_OPERATOR:
		{
			Expr* exp = CreateExpr(EXP_FUNC);
			GetNextToken(in);

			// TODO: check if valid operator token
			
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = EnterFunction("");
			decl->what = DECL_OPERATOR;
			decl->op = CurTok;
			
			GetNextToken(in);
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after 'operator ...'\n");
			GetNextToken(in);
			
			PushScope();

			decl->type = GetBroadTypeHint(FUNC);
			
			int nargs = 0;
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
					ErrorExit("Expected identifier/ellipsis in argument list for operator but received something else\n");
				
				VarDecl* argDecl = DeclareArgument(Lexeme);
				
				GetNextToken(in);
				
				if(nargs >= MAX_ARGS)
					ErrorExit("Exceeded maximum number of arguments in operator declaration\n");

				if(CurTok == ':')
				{
					// NOTE: broad function types have their argument amount set to -1 to indicate indeterminate argument amounts
					// so they must be set to 0 when they're about to be determined
					if(decl->type->func.numArgs < 0)
						decl->type->func.numArgs = 0;

					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
					
					decl->type->func.args[decl->type->func.numArgs++] = type;
					
					argDecl->type = type;
				}
			
				nargs += 1;

				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in operator argument list\n");
			}
			
			if(decl->type->func.numArgs != 2)
				ErrorExit("Operator declaration must have 2 arguments\n");
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->type->func.ret = ParseTypeHint(in);
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
			
			decl->bodyHead = exprHead;
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->type->func.ret, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted operator overload without a value returned\n");
				else
					decl->type->func.ret = GetBroadTypeHint(VOID);
			}
			
			exp->funcx.decl = decl;
			exp->funcx.bodyHead = exprHead;
			
			return exp;
		} break;
		
		case TOK_FUNC:
		{
			Expr* exp = CreateExpr(EXP_FUNC);
			GetNextToken(in);
			
			char name[MAX_ID_NAME_LENGTH];
			if(CurTok != TOK_IDENT)
			{
				if(CurTok != '(')
					ErrorExit("Expected identifier or '(' after 'func'\n");
				else
				{
					// anon function
					name[0] = '\0';
				}
			}
			else
			{
				strcpy(name, Lexeme);
				GetNextToken(in);
			}
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after 'func' %s\n", name);
			GetNextToken(in);
			
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = ReferenceFunction(name);
			if(strlen(name) > 0 && decl)
			{
				if(decl->what == DECL_EXTERN)
					ErrorExit("Function '%s' has the same name as an extern\n", name);
				else
					ErrorExit("Function '%s' has the same name as another function\n", name);
			}
			else
				decl = EnterFunction(name);
			
			PushScope();

			decl->type = GetBroadTypeHint(FUNC);
			
			int nargs = 0;
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
				
				VarDecl* argDecl = DeclareArgument(Lexeme);
				
				GetNextToken(in);
				
				if(nargs >= MAX_ARGS)
					ErrorExit("Exceeded maximum number of arguments in function '%s' declaration\n", name);

				if(CurTok == ':')
				{
					// NOTE: broad function types have their argument amount set to -1 to indicate indeterminate argument amounts
					// so they must be set to 0 when they're about to be determined
					if(decl->type->func.numArgs < 0)
						decl->type->func.numArgs = 0;

					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
					
					decl->type->func.args[decl->type->func.numArgs++] = type;
					
					argDecl->type = type;
				}
			
				nargs += 1;

				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in function '%s' argument list\n", name);
			}
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->type->func.ret = ParseTypeHint(in);
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
			
			decl->bodyHead = exprHead;
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->type->func.ret, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted function '%s' without a value returned\n", name);
				else
					decl->type->func.ret = GetBroadTypeHint(VOID);
			}

			exp->funcx.decl = decl;
			exp->funcx.bodyHead = exprHead;
			
			return exp;
		} break;
		
		case TOK_MACRO:
		{
			Expr* exp = CreateExpr(EXP_FUNC);
			GetNextToken(in);
			
			char name[MAX_ID_NAME_LENGTH];
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identifier after 'macro'\n");			
		
			strcpy(name, Lexeme);
			GetNextToken(in);
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after 'func' %s\n", name);
			GetNextToken(in);
			
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = ReferenceFunction(name);
			if(decl)
				ErrorExit("Macro '%s' has the same name as another function/macro/extern\n", name);
			else
				decl = EnterFunction(name);
				
			decl->what = DECL_MACRO;
						
			PushScope();

			decl->type = GetBroadTypeHint(FUNC);
			
			int nargs = 0;
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
				
				VarDecl* argDecl = DeclareArgument(Lexeme);
				
				GetNextToken(in);
				
				if(nargs >= MAX_ARGS)
					ErrorExit("Exceeded maximum number of arguments in function '%s' declaration\n", name);

				if(CurTok == ':')
				{
					// NOTE: broad function types have their argument amount set to -1 to indicate indeterminate argument amounts
					// so they must be set to 0 when they're about to be determined
					if(decl->type->func.numArgs < 0)
						decl->type->func.numArgs = 0;

					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
					
					decl->type->func.args[decl->type->func.numArgs++] = type;
					
					argDecl->type = type;
				}
			
				nargs += 1;

				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in function '%s' argument list\n", name);
			}
			
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->type->func.ret = ParseTypeHint(in);
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
			
			decl->bodyHead = exprHead;
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->type->func.ret, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted function '%s' without a value returned\n", name);
				else
					decl->type->func.ret = GetBroadTypeHint(VOID);
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
				exp->retx.decl = CurFunc;
				GetNextToken(in);
				
				if(CurTok != ';')
				{
					if(CurFunc->type->func.ret && CurFunc->type->func.ret->hint == VOID)
						Warn("Attempting to return a value in a function which was hinted to return %s\n", HintString(CurFunc->type->func.ret));
						
					if(CurFunc->hasReturn == -1)
						CurFunc->hasReturn = 1;
					else if(!CurFunc->hasReturn)
						ErrorExit("Attempted to return value from function when previous return statement in the same function returned no value\n");
					exp->retx.exp = ParseExpr(in);
					
					return exp;
				}
								
				if(!CompareTypes(CurFunc->type->func.ret, GetBroadTypeHint(VOID)))
					Warn("Attempting to return without a value in a function which was hinted to return a value\n");
				
				if(CurFunc->hasReturn == -1)
					CurFunc->hasReturn = 0;
				else if(CurFunc->hasReturn)
					ErrorExit("Attempted to return with no value when previous return statement in the same function returned a value\n");
				
				GetNextToken(in);
			
				exp->retx.exp = NULL;
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

			GetNextToken(in);

			FuncDecl* extDecl = ReferenceExternRaw(name);
			
			// TODO: Check for multiple definitions with same name
			if(extDecl)
				exp->extDecl = extDecl;
			else
				exp->extDecl = DeclareExtern(name);
			
			exp->extDecl->type = GetBroadTypeHint(FUNC);

			if(CurTok == '(')
			{
				GetNextToken(in);
				
				// NOTE: see TOK_FUNC note on this 
				exp->extDecl->type->func.numArgs = 0;

				while(CurTok != ')')
				{
					TypeHint* type = ParseTypeHint(in);
					
					if(exp->extDecl->type->func.numArgs >= MAX_ARGS)
						ErrorExit("Exceeded maximum number of arguments %d in extern '%s' type declaration\n", MAX_ARGS, name);

					exp->extDecl->type->func.args[exp->extDecl->type->func.numArgs++] = type;	
					
					if(CurTok == ',') GetNextToken(in);
					else if(CurTok != ')') ErrorExit("Expected ')' or ',' in extern argument type list\n");
				}
				GetNextToken(in);
			}
			
			if(CurTok == TOK_AS)
			{
				GetNextToken(in);
				
				if(CurTok != TOK_IDENT)
					ErrorExit("Expected identitifer after 'as'\n");
				
				exp->extDecl->what = DECL_EXTERN_ALIASED;
				strcpy(exp->extDecl->alias, Lexeme);
				
				GetNextToken(in);
			}
			else
			{	
				if(CurTok == ':')
				{
					GetNextToken(in);
					exp->extDecl->type->func.ret = ParseTypeHint(in);
				}
			}
			
			return exp;
		} break;
		
		case TOK_LAMBDA:
		{
			Expr* exp = CreateExpr(EXP_LAMBDA);
			
			if(!CurFunc)
				ErrorExit("Lambdas cannot be declared at global scope (use an anonymous function instead)\n");
			
			GetNextToken(in);
			
			if(CurTok != '(')
				ErrorExit("Expected '(' after lambda\n");
				
			GetNextToken(in);
			
			exp->lamx.dictDecl = DeclareVariable("");
			
			// NOTE: this must be done after the internal dist reg to make sure that we declare the lambda's internal
			// dict in the upvalue scope (i.e in the function where it's declared, not in the lambda function decl itself)
			FuncDecl* prevDecl = CurFunc;
			FuncDecl* decl = EnterFunction("");
			
			decl->what = DECL_LAMBDA;
			
			decl->prevDecl = prevDecl;
			
			PushScope();
			
			// NOTE: Is this okay?
			decl->envDecl = DeclareArgument("");
			
			decl->type = GetBroadTypeHint(FUNC);
			
			int nargs = 0;
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
				
				VarDecl* argDecl = DeclareArgument(Lexeme);
				
				GetNextToken(in);
				
				if(nargs >= MAX_ARGS)
					ErrorExit("Exceeded maximum number of arguments in lambda declaration\n");

				if(CurTok == ':')
				{
					// NOTE: see TOK_FUNC note on this 
					if(decl->type->func.numArgs < 0)
						decl->type->func.numArgs = 0;

					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
					
					decl->type->func.args[decl->type->func.numArgs++] = type;
					
					argDecl->type = type;
				}
			
				nargs += 1;

				if(CurTok == ',') GetNextToken(in);
				else if(CurTok != ')') ErrorExit("Expected ',' or ')' in lambda argument list\n");
			}
			GetNextToken(in);
			
			if(CurTok == ':')
			{
				GetNextToken(in);
				decl->type->func.ret = ParseTypeHint(in);
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

			decl->bodyHead = exprHead;
			CurFunc = prevDecl;
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{
				decl->hasReturn = 0;
				if(!CompareTypes(decl->type->func.ret, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted lambda without a value returned\n");
				else
					decl->type->func.ret = GetBroadTypeHint(VOID);
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
		case '[': case '.':  case '(': case ':': case '{': case '!': case TOK_AS:
			return 1;
	}
	return 0;
}

Expr* ConvertColonCall(Expr* callExp)
{
	assert(callExp->type == EXP_CALL);
	assert(callExp->callx.func->type == EXP_COLON);

	Expr* dotExp = CreateExpr(EXP_DOT);
	dotExp->dotx.dict = callExp->callx.func->colonx.dict;
	strcpy(dotExp->dotx.name, callExp->callx.func->colonx.name);
	dotExp->dotx.isColon = 1;

	Expr* exp = CreateExpr(EXP_CALL);
	exp->callx.numArgs = 1;
	exp->callx.args[0] = dotExp->dotx.dict;
	for(int i = 0; i < callExp->callx.numArgs; ++i)
		exp->callx.args[exp->callx.numArgs++] = callExp->callx.args[i];
	exp->callx.func = dotExp;

	return exp;
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
			exp->dotx.isColon = 0;
		
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

			if(pre->type == EXP_COLON)
				exp = ConvertColonCall(exp);

			while(CurTok != ')')
			{
				if(exp->callx.numArgs >= MAX_ARGS) ErrorExit("Exceeded maximum argument amount");
				exp->callx.args[exp->callx.numArgs++] = ParseExpr(in);
				if(CurTok == ',') GetNextToken(in);
			}
				
			GetNextToken(in);
			
			if(!IsPostOperator()) return exp;
			else return ParsePost(in, exp);
		} break;
		
		case '!':
		{
			if(pre->type != EXP_IDENT)
				ErrorExit("Invalid use of macro call operator '!'\n");
				
			// TODO: This code is pretty much a duplicate of the code above
			GetNextToken(in);
			if(CurTok != '(')
				ErrorExit("Expected '(' after '!'\n");
			GetNextToken(in);
			
			Expr* exp = CreateExpr(EXP_MACRO_CALL);
			
			exp->callx.numArgs = 0;
			exp->callx.func = pre;
			
			while(CurTok != ')')
			{
				if(exp->callx.numArgs >= MAX_ARGS) ErrorExit("Exceeded maximum argument amount");
				exp->callx.args[exp->callx.numArgs++] = ParseExpr(in);
				if(CurTok == ',') GetNextToken(in);
			}
				
			GetNextToken(in);
			
			if(!IsPostOperator()) return exp;
			else return ParsePost(in, exp);
		} break;
		
		case TOK_AS:
		{
			GetNextToken(in);

			Expr* exp = CreateExpr(EXP_TYPE_CAST);
			exp->castx.expr = pre;
			exp->castx.newType = ParseTypeHint(in);

			return exp;
		} break;

		case '{':
		{
			Expr* dict = ParseExpr(in);

			Expr* exp = CreateExpr(EXP_CALL);
			exp->callx.numArgs = 1;
			exp->callx.args[0] = dict;
			exp->callx.func = pre;

			if(pre->type == EXP_COLON)
				exp = ConvertColonCall(exp);
			
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
		case '-': case '!': case '$':
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
		case '*': case '/': case '%': case '&': case '|': case TOK_LSHIFT: case TOK_RSHIFT: case TOK_CAT: prec = 6; break;
		
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

Expr* ParseExpr(FILE* in)
{
	Expr* unary = ParseUnary(in);
	Expr* exp = ParseBinRhs(in, 0, unary);
	// NarrowTypes(exp);
	return exp;
}

