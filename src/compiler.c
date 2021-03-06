#include "lang.h"

void DebugExprList(Expr* head);
void DebugExpr(Expr* exp)
{
	switch(exp->type)
	{
		case EXP_BOOL:
		{
			printf("%s", exp->boolean ? "true" : "false");
		} break;

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
			
			if(exp->retx.exp)
			{
				printf("(");
				DebugExpr(exp->retx.exp);
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
			AppendCode(OP_GET_RETVAL);
		} break;
		
		case EXP_STRING:
		{
			AppendCode(OP_PUSH_STRING);
			AppendInt(exp->constDecl->index);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_string_expr")->index);
			AppendCode(OP_GET_RETVAL);
		} break;
		
		case EXP_IDENT:
		{
			AppendCode(OP_PUSH_STRING);
			AppendInt(RegisterString(exp->varx.name)->index);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_ident_expr")->index);
			AppendCode(OP_GET_RETVAL);
		} break;
		
		case EXP_PAREN:
		{
			ExposeExpr(exp->parenExpr);
			
			AppendCode(OP_CALL);
			AppendCode(1);
			AppendInt(ReferenceFunction("_paren_expr")->index);
			AppendCode(OP_GET_RETVAL);
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
			AppendCode(OP_GET_RETVAL);
		} break;
		
		case EXP_CALL:
		{
			for(int i = exp->callx.numArgs - 1; i >= 0; --i)
				ExposeExpr(exp->callx.args[i]);
			
			ExposeExpr(exp->callx.func);
			
			AppendCode(OP_CALL);
			AppendCode(exp->callx.numArgs + 1);
			AppendInt(ReferenceFunction("_call_expr")->index);
			AppendCode(OP_GET_RETVAL);
		} break;
		
		default:
		{
			AppendCode(OP_CALL);
			AppendCode(0);
			AppendInt(ReferenceFunction("_unknown_expr")->index);
			AppendCode(OP_GET_RETVAL);
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
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if(strcmp(name, "typename") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'typename' only takes 1 argument\n");
		
		const TypeHint* type = InferTypeFromExpr(exp->callx.args[0]);

		AppendCode(OP_PUSH_STRING);
		AppendInt(RegisterString(HintString(type))->index);
		AppendCode(OP_SET_RETVAL);
		
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
		AppendCode(OP_SET_RETVAL);

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
			ErrorExitE(exp, "Intrinsic 'pop' only takes 1 argument\n");
		
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_ARRAY_POP);
		AppendCode(OP_SET_RETVAL);

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
		AppendCode(OP_SET_RETVAL);

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
	else if(strcmp(name, "rawget") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'rawget' takes 2 arguments\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);
		AppendCode(OP_DICT_GET_RAW);
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if(strcmp(name, "rawset") == 0)
	{
		if(exp->callx.numArgs != 3)
			ErrorExitE(exp, "Intrinsic 'rawset' takes 3 arguments\n");
		
		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);

		AppendCode(OP_DICT_SET_RAW);
		
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
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if(strcmp(name, "_file_name") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic '_file_name' takes no arguments\n");

		AppendCode(OP_PUSH_STRING);
		AppendInt(RegisterString(exp->file)->index);
		AppendCode(OP_SET_RETVAL); 
		
		return 1;
	}
	else if(strcmp(name, "_line_number") == 0)
	{
		if(exp->callx.numArgs != 0)
			ErrorExitE(exp, "Intrinsic '_line_number' takes no arguments\n");
		
		AppendCode(OP_PUSH_NUMBER);
		AppendInt(RegisterNumber(exp->line)->index);
		AppendCode(OP_SET_RETVAL);

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

		AppendCode(OP_SET_RETVAL);

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
		AppendCode(OP_SET_RETVAL);
		
		return 1;
	}
	else if(strcmp(name, "setmeta") == 0)
	{
		if(exp->callx.numArgs != 2)
			ErrorExitE(exp, "Intrinsic 'setmeta' takes 2 arguments\n");

		for(int i = exp->callx.numArgs - 1; i >= 0; --i)
			CompileValueExpr(exp->callx.args[i]);

		AppendCode(OP_SET_META);
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if(strcmp(name, "getmeta") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'getmeta' takes 1 argument\n");
		
		CompileValueExpr(exp->callx.args[0]);

		AppendCode(OP_GET_META);
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if(strcmp(name, "typemembers") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'typemembers' takes 1 argument\n");
			
		if(exp->callx.args[0]->type != EXP_IDENT)
			ErrorExitE(exp->callx.args[0], "First argument to intrinsic 'typemembers' must be the type\n");
			
		TypeHint* type = RegisterUserType(exp->callx.args[0]->varx.name);
		if(!type || type->user.numElements == 0)
			ErrorExitE(exp->callx.args[0], "'%s' is not a valid usertype.\n", exp->callx.args[0]->varx.name);
			
		for(int i = 0; i < type->user.numElements; ++i)
		{
			AppendCode(OP_PUSH_STRING);
			AppendInt(RegisterString(type->user.names[i])->index);
		}
			
		AppendCode(OP_CREATE_ARRAY_BLOCK);
		AppendInt(type->user.numElements);
		AppendCode(OP_SET_RETVAL);
		
		return 1;
	}
	else if (strcmp(name, "thread") == 0)
	{
		if (exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'thread' takes 1 argument.\n");
		
		TypeHint* type = InferTypeFromExpr(exp->callx.args[0]);

		if (type->hint != FUNC)
			ErrorExitE(exp->callx.args[0], "Expected function as first argument to 'create_thread' but received", HintString(type));

		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_PUSH_THREAD);
		AppendCode(OP_SET_RETVAL);

		return 1;
	}
	else if (strcmp(name, "delete_thread") == 0)
	{
		if (exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'delete_thread' takes 1 argument.\n");

		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_THREAD_DELETE);

		return 1;
	}
	else if (strcmp(name, "run_thread") == 0)
	{
		if (exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'run_thread' takes 1 argument.\n");

		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_THREAD_RUN);
		// When the thread yields, the owner thread will automatically assume control and the retval would be set
		// to the yielded value

		return 1;
	}
	else if (strcmp(name, "yield") == 0)
	{
		if (exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'yield' takes 1 argument.\n");
		
		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_THREAD_YIELD);
		
		return 1;
	}
	else if(strcmp(name, "is_thread_done") == 0)
	{
		if(exp->callx.numArgs != 1)
			ErrorExitE(exp, "Intrinsic 'is_thread_done' takes 1 argument.\n");

		CompileValueExpr(exp->callx.args[0]);
		AppendCode(OP_THREAD_DONE);
		AppendCode(OP_SET_RETVAL);
		
		return 1;
	}

	return 0;
}

Patch* Patches = NULL;
int CurrentPatchScope = 0;

void PushPatchScope()
{
	++CurrentPatchScope;
}

void PopPatchScope()
{
	--CurrentPatchScope;
}

void AddPatch(PatchType type, int loc)
{
	Patch* p = malloc(sizeof(Patch));
	assert(p);
	
	p->type = type;
	p->loc = loc;
	p->scope = CurrentPatchScope;
	
	p->next = Patches;
	Patches = p;
}

void ClearPatches()
{
	Patch** p = &Patches;

	while(*p)
	{
		if((*p)->scope == CurrentPatchScope)
		{
			Patch* removed = *p;
			*p = removed->next;
			free(removed);
		}
		else
			p = &(*p)->next;
	}
}

void CheckArgumentTypes(const TypeHint* type, Expr* exp, FuncDecl* decl)
{
	assert(type);
	assert((exp->type == EXP_CALL || exp->type == EXP_MACRO_CALL));
	
	if(type->hint != FUNC) return;
	
	if(type->func.numArgs >= 0)
	{
		if(decl)
		{
			if(decl->hasEllipsis)
			{
				if(exp->callx.numArgs < type->func.numArgs)
					WarnE(exp, "The number of types specified for the function being called (%i) is greater than the number of arguments passed in (%i)\n", type->func.numArgs, exp->callx.numArgs);
			}
			else
			{	
				if(exp->callx.numArgs != type->func.numArgs)
					WarnE(exp, "The number of types specified for the function being called (%i) is not equal to the number of arguments passed in (%i)\n", type->func.numArgs, exp->callx.numArgs);
			}
		}
	}
	
	if(type && type->hint == FUNC)
	{
		for(int i = 0; i < type->func.numArgs && i < exp->callx.numArgs; ++i)
		{
			const TypeHint* inf = InferTypeFromExpr(exp->callx.args[i]);
			if(!CompareTypes(inf, type->func.args[i]))
				WarnE(exp->callx.args[i], "Argument %i's type '%s' does not match the expected type '%s'\n", i + 1, HintString(inf), HintString(type->func.args[i]));
		}
	}
}

void CompileValueExpr(Expr* exp);

// NOTE: Used when the function being called is resolved at compile time
static void CompileStaticCallExpr(Expr* exp, FuncDecl* decl, char expectReturn, int numExpansions)
{	
	assert(decl);
	
	if(decl->type)
		CheckArgumentTypes(decl->type, exp, decl);
	
	for(int i = exp->callx.numArgs - 1; i >= 0; --i)
		CompileValueExpr(exp->callx.args[i]);
		
	if(decl->what == DECL_EXTERN || decl->what == DECL_EXTERN_ALIASED)
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

// NOTE: Used when the function being called cannot be resolved at compile time
static void CompileDynamicCallExpr(Expr* exp, char expectReturn, int numExpansions)
{
	const TypeHint* type = InferTypeFromExpr(exp->callx.func);
	FuncDecl* decl = type ? GetSpecialOverload(type, NULL, OVERLOAD_CALL) : NULL;
	
	// overloaded call operator
	if(decl)
	{
        // Shift other args over
        memmove(&exp->callx.args[1], &exp->callx.args[0], sizeof(Expr*) * exp->callx.numArgs);

        // Pass in the "function" as the first argument
        exp->callx.args[0] = exp->callx.func; 
        exp->callx.numArgs += 1;

		CompileStaticCallExpr(exp, decl, expectReturn, numExpansions);
		return;
	}
	
	if(type)
		CheckArgumentTypes(type, exp, NULL);

	for(int i = exp->callx.numArgs - 1; i >= 0; --i)
		CompileValueExpr(exp->callx.args[i]);
	
	CompileValueExpr(exp->callx.func);

	AppendCode(OP_CALLP);
	AppendCode(exp->callx.numArgs - numExpansions);

	if(expectReturn)
		AppendCode(OP_GET_RETVAL);
}

static void CompileCallExpr(Expr* exp, char expectReturn)
{
	assert((exp->type == EXP_CALL || exp->type == EXP_MACRO_CALL));

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
	
	// TODO: This isn't really structured properly (like the dynamic call expression checks for overloads, etc)
	// see what you can do about this
	
	if(exp->callx.func->type == EXP_IDENT)
	{
		FuncDecl* decl = ReferenceFunction(exp->callx.func->varx.name);
		
		if(decl)
			CompileStaticCallExpr(exp, decl, expectReturn, numExpansions);
		else
		{
			char res = CompileIntrinsic(exp, exp->callx.func->varx.name);
			if(!res)
				CompileDynamicCallExpr(exp, expectReturn, numExpansions);
			else if(expectReturn)
				AppendCode(OP_GET_RETVAL);
		}
	}
	else
		CompileDynamicCallExpr(exp, expectReturn, numExpansions);
}

// NOTE: sets variable to top of stack
void SetVar(VarDecl* decl)
{
	assert(decl);

	if(decl->isGlobal)
	{
		AppendCode(OP_SET);
		AppendInt(decl->index);
	}
	else
	{
		AppendCode(OP_SETLOCAL);
		AppendInt(decl->index);
	}
}

void GetVar(VarDecl* decl)
{
	assert(decl);

	if(decl->isGlobal)
	{
		AppendCode(OP_GET);
		AppendInt(decl->index);
	}
	else
	{
		AppendCode(OP_GETLOCAL);
		AppendInt(decl->index);
	}
}


static void CheckMacroCall(Expr* exp)
{
	assert(exp->type == EXP_MACRO_CALL);
	
	FuncDecl* decl = ReferenceFunction(exp->callx.func->varx.name);
	if(!decl)
		ErrorExitE(exp, "Invalid reference to macro '%s'\n", exp->callx.func->varx.name);
	
	if(decl->what != DECL_MACRO)
		ErrorExitE(exp, "Attempted to call '%s' using macro call operator '!' when it's not a macro\n", exp->callx.func->varx.name);	
}

void CompileExprList(Expr* head);
// Expression should have a resulting value (pushed onto the stack)
void CompileValueExpr(Expr* exp)
{
	exp->pc = CodeLength;
	
	if(ProduceDebugInfo)
	{
		AppendCode(OP_FILE);
		AppendInt(RegisterString(exp->file)->index);
		
		AppendCode(OP_LINE);
		AppendInt(exp->line);
	}
	
	switch(exp->type)
	{
		case EXP_BOOL:
		{
			if (exp->boolean)
				AppendCode(OP_PUSH_TRUE);
			else
				AppendCode(OP_PUSH_FALSE);
		} break;

		case EXP_NUMBER: case EXP_STRING:
		{
			AppendCode(exp->constDecl->type == CONST_NUM ? OP_PUSH_NUMBER : OP_PUSH_STRING);
			AppendInt(exp->constDecl->index);
		} break;
		
		case EXP_NULL:
		{
			AppendCode(OP_PUSH_NULL);
		} break;

		case EXP_TYPE_CAST:
		{
			CompileValueExpr(exp->castx.expr);
		} break;

		case EXP_UNARY:
		{
			const TypeHint* type = InferTypeFromExpr(exp->unaryx.expr);
			FuncDecl* overload = type ? GetUnaryOverload(type, exp->unaryx.op) : NULL;
			
			CompileValueExpr(exp->unaryx.expr);
			if(overload)
			{
				AppendCode(OP_CALL);
				AppendCode(1); 	// 1 argument
				AppendInt(overload->index);
				
				AppendCode(OP_GET_RETVAL);
			}
			else
			{
				switch(exp->unaryx.op)
				{
					case '-': AppendCode(OP_NEG); break;
					case '!': AppendCode(OP_LOGICAL_NOT); break;
				}
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
			// NOTE: This is intended to skip checking if variables (not in macros) are declared until macros have executed
			if(CompilingMacros && (!exp->curFunc || exp->curFunc->what != DECL_MACRO))
			{
				// printf("%s elided\n", exp->varx.name);
				break;
			}
			// printf("name of var: %s\n", exp->varx.name);
			
			if(!exp->varx.varDecl)
				exp->varx.varDecl = ReferenceVariable(exp->varx.name);

			if(!exp->varx.varDecl)
			{
				FuncDecl* decl = ReferenceFunction(exp->varx.name);
				if(!decl)
					ErrorExitE(exp, "Attempted to reference undeclared identifier '%s'\n", exp->varx.name);
				
				AppendCode(OP_PUSH_FUNC);
				AppendCode(MINT_FALSE);
				AppendCode(decl->what == DECL_EXTERN || decl->what == DECL_EXTERN_ALIASED);
				AppendInt(decl->index);
			}
			else
				GetVar(exp->varx.varDecl);
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			const TypeHint* a = InferTypeFromExpr(exp->arrayIndex.arrExpr);
			const TypeHint* b = InferTypeFromExpr(exp->arrayIndex.indexExpr);
			FuncDecl* overload = a && b ? GetSpecialOverload(a, b, OVERLOAD_INDEX) : NULL;
		
			CompileValueExpr(exp->arrayIndex.indexExpr);
			CompileValueExpr(exp->arrayIndex.arrExpr);
			if(overload)
			{
				AppendCode(OP_CALL);
				AppendCode(2);
				AppendInt(overload->index);
				
				AppendCode(OP_GET_RETVAL);
			}
			else
				AppendCode(OP_GETINDEX);
		} break;
		
		case EXP_BIN:
		{	
			if(exp->binx.op == '=')
				ErrorExitE(exp, "Assignment expressions cannot be used as values\n");
			else
			{				
				const TypeHint* a = InferTypeFromExpr(exp->binx.lhs);
				const TypeHint* b = InferTypeFromExpr(exp->binx.rhs);
				
				FuncDecl* overload = GetBinaryOverload(a, b, exp->binx.op);
				
				if(overload)
				{
					CompileValueExpr(exp->binx.rhs);
					CompileValueExpr(exp->binx.lhs);
					
					// call the overload with 2 arguments
					AppendCode(OP_CALL);
					AppendCode(2);
					AppendInt(overload->index);
					
					AppendCode(OP_GET_RETVAL);
				}
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
						case TOK_CAT: AppendCode(OP_CAT); break;
						/*case TOK_CADD: AppendCode(OP_CADD); break;
						case TOK_CSUB: AppendCode(OP_CSUB); break;
						case TOK_CMUL: AppendCode(OP_CMUL); break;
						case TOK_CDIV: AppendCode(OP_CDIV); break;*/
						
						default:
							ErrorExitE(exp, "Unsupported binary operator %c (%i)\n", exp->binx.op, exp->binx.op);		
					}
				}
			}
		} break;
		
		case EXP_MACRO_CALL:
		{
			if(CompilingMacros)
			{
				CheckMacroCall(exp);
				CompileCallExpr(exp, 1);
				AppendCode(OP_HALT);
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
				TypeHint* type = InferTypeFromExpr(exp->dotx.dict);

				if(type && type->hint == USERTYPE)
				{
					int eindex = GetUserTypeElementIndex(type, exp->dotx.name);

					if(eindex < 0)
						ErrorExitE(exp, "Attempted to access non-existent field '%s' in usertype '%s'\n", exp->dotx.name, type->user.name);
				}
				
				AppendCode(OP_PUSH_STRING);
				AppendInt(RegisterString(exp->dotx.name)->index);
				CompileValueExpr(exp->dotx.dict);
				AppendCode(OP_DICT_GET);
			}
		} break;
		
		case EXP_COLON:
		{
			ErrorExitE(exp, "Attempted to use ':' to index value inside dictionary; use '.' instead (or call the function you're indexing)\n");
		} break;
		
		case EXP_DICT_LITERAL:
		{
			Expr* node = exp->dictx.pairsHead;
			
			VarDecl* decl = exp->dictx.decl;
			
			AppendCode(OP_PUSH_DICT);
			SetVar(decl);
			
			while(node)
			{
				if(node->type != EXP_BIN) ErrorExitE(exp, "Non-binary expression in dictionary literal (Expected something = something_else)\n");
				if(node->binx.op != '=') ErrorExitE(exp, "Invalid dictionary literal binary operator '%c'\n", node->binx.op);
				if(node->binx.lhs->type != EXP_IDENT && node->binx.lhs->type != EXP_STRING) ErrorExitE(exp, "Invalid lhs in dictionary literal (Expected identifier or string)\n");
				
				if(strcmp(node->binx.lhs->varx.name, "pairs") == 0) ErrorExitE(exp, "Attempted to assign value to reserved key 'pairs' in dictionary literal\n");
				
				/*CompileExpr(node->binx.rhs);
				AppendCode(OP_PUSH_STRING);
				AppendInt(RegisterString(node->binx.lhs->varx.name)->index);*/
				
				CompileValueExpr(node->binx.rhs);
				
				AppendCode(OP_PUSH_STRING);
				if(node->binx.lhs->type == EXP_IDENT)
					AppendInt(RegisterString(node->binx.lhs->varx.name)->index);
				else
					AppendInt(node->binx.lhs->constDecl->index);
				
				GetVar(decl);

				AppendCode(OP_DICT_SET_RAW);
				
				node = node->next;
			}
			GetVar(decl);
			
			/*AppendCode(OP_CREATE_DICT_BLOCK);
			AppendInt(exp->dictx.length);*/
		} break;
		
		case EXP_LAMBDA:
		{
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
			SetVar(decl);

			// each nested lambda stores the previous env so that values above the uppermost lambda are accessible
			if(exp->lamx.decl->prevDecl->what == DECL_LAMBDA)
			{
				Expr* prevEnv = CreateExpr(EXP_IDENT);
				prevEnv->varx.varDecl = exp->lamx.decl->prevDecl->envDecl;
				strcpy(prevEnv->varx.name, exp->lamx.decl->prevDecl->envDecl->name);
				CompileValueExpr(prevEnv);
				
				AppendCode(OP_PUSH_STRING);
				AppendInt(RegisterString("env")->index);
					
				GetVar(decl);

				AppendCode(OP_DICT_SET_RAW);
			}
			
			Upvalue* upvalue = exp->lamx.decl->upvalues;
			
			while(upvalue)
			{	
				Expr* uexp = CreateExpr(EXP_IDENT);
				uexp->varx.varDecl = upvalue->decl;
				strcpy(uexp->varx.name, upvalue->decl->name);
				
				CompileValueExpr(uexp);
				
				AppendCode(OP_PUSH_STRING);
				AppendInt(RegisterString(upvalue->decl->name)->index);
				
				GetVar(decl);
				
				AppendCode(OP_DICT_SET_RAW);
				
				upvalue = upvalue->next;
			}
		
			GetVar(decl);
			AppendCode(OP_PUSH_FUNC);
			AppendCode(MINT_TRUE);
			AppendCode(MINT_FALSE);
			AppendInt(exp->lamx.decl->index);
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
			
			AppendCode(OP_PUSH_FUNC);
			AppendCode(MINT_FALSE);
			AppendCode(MINT_FALSE);
			AppendInt(exp->funcx.decl->index);
		} break;

		case EXP_IF:
		{
			CompileValueExpr(exp->ifx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));

			Expr* node = exp->ifx.bodyHead;
			while(node)
			{
				if(node->next)
					CompileExpr(node);
				else
					CompileValueExpr(node);
				node = node->next;
			}
			
			AppendCode(OP_GOTO);
			int exitEmplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			EmplaceInt(emplaceLoc, CodeLength);
			if(exp->ifx.alt)
			{
				if(exp->ifx.alt->type != EXP_IF)
				{	
					Expr* node = exp->ifx.alt;
					while(node)
					{
						if(node->next)
							CompileExpr(node);
						else
							CompileValueExpr(node);
						node = node->next;
					}
				}
				else
					CompileValueExpr(exp->ifx.alt);
			}
			EmplaceInt(exitEmplaceLoc, CodeLength);
		} break;

		case EXP_FOR:
		{
			VarDecl* comDecl = exp->forx.comDecl;

			AppendCode(OP_PUSH_NUMBER);
			AppendInt(RegisterNumber(0)->index);

			AppendCode(OP_CREATE_ARRAY);
			SetVar(comDecl);

			PushPatchScope();

			CompileExpr(exp->forx.init);
			int loopPc = CodeLength;
			
			CompileValueExpr(exp->forx.cond);
			AppendCode(OP_GOTOZ);
			int emplaceLoc = CodeLength;
			AllocatePatch(sizeof(int) / sizeof(Word));
			
			Expr* node = exp->forx.bodyHead;
			while(node)
			{
				if(node->next)
					CompileExpr(node);
				else
				{
					CompileValueExpr(node);
					GetVar(comDecl);
					AppendCode(OP_ARRAY_PUSH);
				}
				node = node->next;
			}

			int continueLoc = CodeLength;

			CompileExpr(exp->forx.iter);
			
			AppendCode(OP_GOTO);
			AppendInt(loopPc);
			
			int exitLoc = CodeLength;
			EmplaceInt(emplaceLoc, CodeLength);
		
			for(Patch* p = Patches; p != NULL; p = p->next)
			{
				if(p->scope == CurrentPatchScope)
				{
					if(p->type == PATCH_CONTINUE) EmplaceInt(p->loc, continueLoc);
					else if(p->type == PATCH_BREAK) EmplaceInt(p->loc, exitLoc);
				}
			}

			ClearPatches();
			PopPatchScope();

			GetVar(comDecl);
		} break;

		default:
			ErrorExitE(exp, "Expected value expression in place of %s\n", ExprNames[exp->type]);
	}
	
	exp->compiled = 1;
}

void CompileExprList(Expr* head);
void CompileExpr(Expr* exp)
{
	exp->pc = CodeLength;
	
	// printf("compiling expression (%s:%i)\n", exp->file, exp->line);
	if(ProduceDebugInfo)
	{
		AppendCode(OP_FILE);
		AppendInt(RegisterString(exp->file)->index);
		
		AppendCode(OP_LINE);
		AppendInt(exp->line);
	}
	
	switch(exp->type)
	{	
		case EXP_EXTERN: case EXP_VAR: case EXP_TYPE_DECL: break;
		
		case EXP_UNARY:
		{
			const TypeHint* type = InferTypeFromExpr(exp->unaryx.expr);
			FuncDecl* overload = type ? GetUnaryOverload(type, exp->unaryx.op) : NULL;
			
			if(overload)
			{
				CompileValueExpr(exp->unaryx.expr);
				AppendCode(OP_CALL);
				AppendCode(1); 	// 1 argument
				AppendInt(overload->index);
				
				AppendCode(OP_GET_RETVAL);
			}
			else
				ErrorExitE(exp, "Invalid top level unary expression with no overload\n");
		} break;
		
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
					TypeHint* type = InferTypeFromExpr(exp->binx.lhs->dotx.dict);

					if(type ? type->hint != USERTYPE : 1)
					{
						if(strcmp(exp->binx.lhs->dotx.name, "pairs") == 0)
							ErrorExitE(exp, "Cannot assign dictionary entry 'pairs' to a value\n");
					}
					else
					{
						int eindex = GetUserTypeElementIndex(type, exp->binx.lhs->dotx.name);
						const TypeHint* etype = NULL;

						if(eindex < 0)
							ErrorExitE(exp, "Attempted to set non-existent field '%s' in usertype '%s'\n", exp->binx.lhs->dotx.name, type->user.name);
						etype = GetUserTypeElement(type, exp->binx.lhs->dotx.name);

						TypeHint* rtype = InferTypeFromExpr(exp->binx.rhs);
						if(!CompareTypes(rtype, etype))
							WarnE(exp, "Attempted to set field '%s' of type '%s' to value of type '%s' when field has type '%s'\n", exp->binx.lhs->dotx.name, HintString(type), 
								HintString(rtype), 
								HintString(etype));
					}

					AppendCode(OP_PUSH_STRING);
					AppendInt(RegisterString(exp->binx.lhs->dotx.name)->index);
					CompileValueExpr(exp->binx.lhs->dotx.dict);
					AppendCode(OP_DICT_SET);
				}
				else
					ErrorExitE(exp, "Left hand side of assignment operator '=' must be an assignable value (variable, array index, dictionary index, usertype) instead of %s\n", ExprNames[exp->binx.lhs->type]);
			}
			else
			{
				const TypeHint* a = InferTypeFromExpr(exp->binx.lhs);
				const TypeHint* b = InferTypeFromExpr(exp->binx.rhs);
			
				FuncDecl* decl = GetBinaryOverload(a, b, exp->binx.op);
					
				if(decl)
				{
					CompileValueExpr(exp->binx.rhs);
					CompileValueExpr(exp->binx.lhs);
					AppendCode(OP_CALL);
					AppendCode(2);
					AppendInt(decl->index);
					
					// NOTE: Discards return value of overloaded function if there is one
				}
				else
					ErrorExitE(exp, "Invalid top level binary expression\n");
			}
		} break;
		
		case EXP_MACRO_CALL:
		{
			if(CompilingMacros)
			{
				CheckMacroCall(exp);
				CompileCallExpr(exp, 0);
				AppendCode(OP_HALT);
			}
		} break;
		
		case EXP_CALL:
		{	
			CompileCallExpr(exp, 0);
		} break;
		
		case EXP_WHILE:
		{
			PushPatchScope();

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
				if(p->scope == CurrentPatchScope)
				{
					if(p->type == PATCH_CONTINUE) EmplaceInt(p->loc, loopPc);
					else if(p->type == PATCH_BREAK) EmplaceInt(p->loc, exitLoc);
				}
			}

			ClearPatches();
			PopPatchScope();
		} break;
		
		case EXP_FOR:
		{
			PushPatchScope();

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
				if(p->scope == CurrentPatchScope)
				{
					if(p->type == PATCH_CONTINUE) EmplaceInt(p->loc, continueLoc);
					else if(p->type == PATCH_BREAK) EmplaceInt(p->loc, exitLoc);
				}
			}

			ClearPatches();
			PopPatchScope();
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
			if(exp->retx.exp)
			{
				CompileValueExpr(exp->retx.exp);
				AppendCode(OP_RETURN_VALUE);
			}
			else
				AppendCode(OP_RETURN);
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
		
		case EXP_MULTI:
		{
			CompileExprList(exp->multiHead);
		} break;
		
		/*case EXP_LINKED_BINARY_CODE:
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
		} break;*/
		
		default:
			ErrorExitE(exp, "Invalid top level expression (expected non-value expression in place of %s)\n", ExprNames[exp->type]);
	}
	
	exp->compiled = 1;
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
