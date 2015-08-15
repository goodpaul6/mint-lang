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
	"EXP_FORWARD"
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
	
		case TOK_FORWARD:
		{
			GetNextToken(in);
			
			if(CurTok != TOK_IDENT)
				ErrorExit("Expected identifier after 'forward'\n");
				
			char name[MAX_ID_NAME_LENGTH];
			strcpy(name, Lexeme);
			
			GetNextToken(in);
			
			// function forward declaration
			if(CurTok == '(')
			{
				GetNextToken(in);
				
				FuncDecl* decl = ReferenceFunction(name);
				if(decl)
				{
					decl = malloc(sizeof(FuncDecl)); // allocate a dummy decl so that it doesn't affect the initial declaration
					assert(decl);
				}
				else
					decl = DeclareFunction(name, NumFunctions++);
				
				TypeHint* argTypes = NULL;
				TypeHint* currentType = NULL;
				
				while(CurTok != ')')
				{
					TypeHint* type = ParseTypeHint(in);
					
					if(!argTypes)
					{
						argTypes = type;
						currentType = type;
					}
					else
					{
						currentType->next = type;
						currentType = type;
					}
					
					if(CurTok == ',') GetNextToken(in);
					else if(CurTok != ')') ErrorExit("Expected '(' or ',' in forward type declaration list\n");
				}
				decl->argTypes = argTypes;
				GetNextToken(in);
				
				if(CurTok == ':')
				{
					GetNextToken(in);
					TypeHint* type = ParseTypeHint(in);
					decl->returnType = type;
				}
			}
			else if(CurTok == ':')
			{
				GetNextToken(in);
				
				VarDecl* decl = ReferenceVariable(name);
				if(decl)
				{
					decl = malloc(sizeof(VarDecl)); // allocate a dummy decl so that it doesn't affect the initial declaration
					assert(decl);
				}
				else
					decl = RegisterVariable(name);
				
				TypeHint* type = ParseTypeHint(in);
				decl->type = type;
			}
			else
				ErrorExit("Expected ':' or '(' after 'forward %s'\n", name);
		
			return CreateExpr(EXP_FORWARD);
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
			FuncDecl* decl = ReferenceFunction(name);
			if(decl)
			{
				if(decl->isExtern)
					ErrorExit("Function '%s' has the same name as an extern\n", name);
				CurFunc = decl;
			}
			else
				decl = EnterFunction(name);
			
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
			if(argTypes && !decl->argTypes)
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
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{	
				decl->hasReturn = 0;
				if(!CompareTypes(decl->returnType, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted function '%s' without a value returned\n", decl->name);
				else
					decl->returnType = GetBroadTypeHint(VOID);
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
					if(CurFunc->returnType && CurFunc->returnType->hint == VOID)
						Warn("Attempting to return a value in a function which was hinted to return %s\n", HintString(CurFunc->returnType));
						
					if(CurFunc->hasReturn == -1)
						CurFunc->hasReturn = 1;
					else if(!CurFunc->hasReturn)
						ErrorExit("Attempted to return value from function when previous return statement in the same function returned no value\n");
					exp->retx.exp = ParseExpr(in);
					
					return exp;
				}
								
				if(!CompareTypes(CurFunc->returnType, GetBroadTypeHint(VOID)))
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
				
				/*GetNextToken(in);
				
				if(CurTok != TOK_IDENT) ErrorExit("Expected identifier after 'as'\n");
				exp->extDecl = RegisterExternAs(name, Lexeme);
				
				printf("registering extern %s as %s\n", name, Lexeme);
				
				GetNextToken(in);*/
			}
			else
			{	
				exp->extDecl = ReferenceFunction(name);
				if(!exp->extDecl)
					exp->extDecl = DeclareExtern(name);
				else if(!exp->extDecl->isExtern)
					ErrorExit("Attempted to declare extern with same name as previously defined function '%s'\n", name);
				
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
			
			if(decl->hasReturn == -1 || decl->hasReturn == 0)
			{
				decl->hasReturn = 0;
				if(!CompareTypes(decl->returnType, GetBroadTypeHint(VOID)))
					Warn("Reached end of non-void hinted lambda without a value returned\n");
				else
					decl->returnType = GetBroadTypeHint(VOID);
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
		case '[': case '.':  case '(': case ':': case '{':
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
				
			GetNextToken(in);
			
			if(!IsPostOperator()) return exp;
			else return ParsePost(in, exp);
		} break;
		
		case '{':
		{
			Expr* dict = ParseExpr(in);

			Expr* exp = CreateExpr(EXP_CALL);
			exp->callx.numArgs = 1;
			exp->callx.args[0] = dict;
			exp->callx.func = pre;

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

