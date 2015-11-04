// macro.c -- compile time execution module in mint
#include "lang.h"
#include "linkedlist.h"
#include "vm.h"

char CompilingMacros = 0;

struct
{
	int line;
	const char* file;
	int varScope;
	FuncDecl* funcDecl;
} Context;

// TODO: This shouldn't be global
static Expr* NextExpr;

LinkedList MacroList;

static void RecordMacro(Expr** pExp)
{
	Expr* exp = *pExp;
	
	switch(exp->type)
	{
		case EXP_CALL:
		{
			for(int i = 0; i < exp->callx.numArgs; ++i)
				RecordMacro(&exp->callx.args[i]);
		} break;
		
		case EXP_MACRO_CALL:
		{			
			// later on just replace the pointer at the address with the result of the macro
			AddNode(&MacroList, pExp);
		} break;
		
		case EXP_BIN:
		{
			RecordMacro(&exp->binx.lhs);
			RecordMacro(&exp->binx.rhs);
		} break;
		
		case EXP_PAREN:
		{
			RecordMacro(&exp->parenExpr);
		} break;
		
		case EXP_WHILE:
		{
			Expr** node = &exp->whilex.bodyHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_FUNC:
		{
			Expr** node = &exp->funcx.bodyHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_IF:
		{
			RecordMacro(&exp->ifx.cond);
			Expr** node = &exp->ifx.bodyHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
			
			if(exp->ifx.alt)
			{
				if(exp->ifx.alt->type == EXP_IF)
					RecordMacro(&exp->ifx.alt);
				else
				{
					node = &exp->ifx.alt;
					while(*node)
					{
						RecordMacro(node);
						node = &(*node)->next;
					}
				}
			}
		} break;
		
		case EXP_RETURN:
		{
			RecordMacro(&exp->retx.exp);
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			Expr** node = &exp->arrayx.head;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_UNARY:
		{
			RecordMacro(&exp->unaryx.expr);
		} break;
		
		case EXP_FOR:
		{
			RecordMacro(&exp->forx.init);
			RecordMacro(&exp->forx.cond);
			RecordMacro(&exp->forx.iter);
			
			Expr** node = &exp->forx.bodyHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_DICT_LITERAL:
		{
			Expr** node = &exp->dictx.pairsHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		case EXP_LAMBDA:
		{
			Expr** node = &exp->lamx.bodyHead;
			while(*node)
			{
				RecordMacro(node);
				node = &(*node)->next;
			}
		} break;
		
		default:
			break;
	}
}

#define SET_CONTEXT FuncDecl* prevFunc = CurFunc; \
					int prevLine = LineNumber; \
					const char* prevFile = FileName; \
					CurFunc = Context.funcDecl; \
					LineNumber = Context.line; \
					FileName = Context.file; \
					VarScope = Context.varScope; 
					
#define UNSET_CONTEXT CurFunc = prevFunc; \
					LineNumber = prevLine; \
					FileName = prevFile; \
					VarScope = 0;

// TODO: Move these to the vm's standard library (perhaps)
static void Macro_MultiExpr(VM* vm)
{
	SET_CONTEXT
	
	Object* obj = PopArrayObject(vm);
	
	Expr* exp = CreateExpr(EXP_MULTI);
	Expr* node = NULL;
	for(int i = 0; i < obj->array.length; ++i)
	{
		if(obj->array.members[i]->type != OBJ_NATIVE)
			ErrorExitVM(vm, "Invalid expression in array argument to 'macro_multi'\n");
		if(!node)
		{
			exp->multiHead = obj->array.members[i]->native.value;
			node = exp->multiHead;
		}
		else
		{
			node->next = obj->array.members[i]->native.value;
			node = node->next;
		}
	}
	
	PushNative(vm, exp, NULL, NULL);
	ReturnTop(vm);
	
	UNSET_CONTEXT
}

static void Macro_NumExpr(VM* vm)
{	
	SET_CONTEXT
	
	Expr* exp = CreateExpr(EXP_NUMBER);
	exp->constDecl = RegisterNumber(PopNumber(vm));
		
	PushNative(vm, exp, NULL, NULL);
	ReturnTop(vm);
	
	UNSET_CONTEXT
}

static void Macro_StrExpr(VM* vm)
{
	SET_CONTEXT
	
	Expr* exp = CreateExpr(EXP_STRING);
	exp->constDecl = RegisterString(PopString(vm));
	
	PushNative(vm, exp, NULL, NULL);
	ReturnTop(vm);

	UNSET_CONTEXT
}

static void Macro_DeclareVariable(VM* vm)
{
	SET_CONTEXT
	
	const char* string = PopString(vm);
	VarDecl* decl = ReferenceVariable(string);
	
	if(!decl)
		decl = DeclareVariable(string);
	else
		ErrorExitVM(vm, "Attempted to declare previously existing variable '%s'\n", string);
		
	printf("declared variable '%s' at scope %i\n", decl->name, decl->scope);
	PushNative(vm, decl, NULL, NULL);
	ReturnTop(vm);
	
	UNSET_CONTEXT
}

static void Macro_ReferenceVariable(VM* vm)
{	
	SET_CONTEXT
	
	const char* string = PopString(vm);
	VarDecl* decl = ReferenceVariable(string);
	
	if(!decl)
	{
		PushNull(vm);
		ReturnTop(vm);
		return;
	}
		
	PushNative(vm, decl, NULL, NULL);
	ReturnTop(vm);
	
	UNSET_CONTEXT
}

static void Macro_IdentExpr(VM* vm)
{
	SET_CONTEXT
	
	VarDecl* decl = PopNative(vm);
	Expr* exp = CreateExpr(EXP_IDENT);
	strcpy(exp->varx.name, decl->name);
	exp->varx.varDecl = decl;
	
	PushNative(vm, exp, NULL, NULL);
	ReturnTop(vm);

	UNSET_CONTEXT
}

static void Macro_BinExpr(VM* vm)
{
	SET_CONTEXT
	
	const char* op = PopString(vm);
	Expr* lhs = PopNative(vm);
	Expr* rhs = PopNative(vm);
	
	Expr* exp = CreateExpr(EXP_BIN);
	exp->binx.lhs = lhs;
	exp->binx.rhs = rhs;
	
	// TODO: add more operators
	if(strcmp(op, "+") == 0) exp->binx.op = '+';
	else if(strcmp(op, "-") == 0) exp->binx.op = '-';
	else if(strcmp(op, "*") == 0) exp->binx.op = '*';
	else if(strcmp(op, "/") == 0) exp->binx.op = '/';
	else if(strcmp(op, "=") == 0) exp->binx.op = '=';
	else; // TODO: Fail
	
	PushNative(vm, exp, NULL, NULL);
	ReturnTop(vm);
	
	UNSET_CONTEXT
}

static void SetExprContext(VM* vm, Expr* exp)
{
	Context.line = exp->line;
	Context.file = exp->file;
	Context.varScope = exp->varScope;
	Context.funcDecl = exp->curFunc;
}

void ExecuteMacros(Expr** exp)
{
	InitList(&MacroList);
	
	Expr** node = exp;
	
	while(*node)
	{
		RecordMacro(node);
		node = &(*node)->next;
	}
	
	if(MacroList.length > 0)
	{
		CompilingMacros = 1;
		CompileExprList(*exp);
		CompilingMacros = 0;
		AppendCode(OP_HALT);
	
		VM* vm = NewVM();
	
		FILE* tmp = tmpfile();
		if(!tmp)
			ErrorExitE(*exp, "Failed to create temporary file to store code for compile-time execution\n");
		OutputCode(tmp);
		
		rewind(tmp);
		LoadBinaryFile(vm, tmp);
		fclose(tmp);
		
		HookStandardLibrary(vm);
		
		HookExternNoWarn(vm, "macro_multi", Macro_MultiExpr);
		HookExternNoWarn(vm, "macro_num_expr", Macro_NumExpr);
		HookExternNoWarn(vm, "macro_str_expr", Macro_StrExpr);
		HookExternNoWarn(vm, "macro_declare_variable", Macro_DeclareVariable);
		HookExternNoWarn(vm, "macro_reference_variable", Macro_ReferenceVariable);
		HookExternNoWarn(vm, "macro_ident_expr", Macro_IdentExpr);
		HookExternNoWarn(vm, "macro_bin_expr", Macro_BinExpr);
		
		for(LinkedListNode* node = MacroList.head; node != NULL; node = node->next)
		{
			Expr** pNodeExp = node->data;
			Expr* nodeExp = *pNodeExp;
			NextExpr = nodeExp->next;
			
			// NOTE: This assumes that exp->callx.func is an EXP_IDENT
			FuncDecl* decl = ReferenceFunction(nodeExp->callx.func->varx.name);
			if(!decl)
				ErrorExitE(nodeExp, "Attempted to call non-existent function '%s' at compile time\n", nodeExp->callx.func->varx.name); 
			
			vm->pc = nodeExp->pc;
			SetExprContext(vm, nodeExp);
			
			printf("ctx line: %i\n", Context.line);
			printf("ctx file: %s\n", Context.file);
			printf("ctx scope: %i\n", Context.varScope);
			
			// all macro call code ends in an OP_HALT
			while(vm->pc >= 0)
				ExecuteCycle(vm);
			
			if(decl->hasReturn > 0)
			{
				if(!vm->retVal || vm->retVal->type != OBJ_NATIVE)
					ErrorExitE(nodeExp, "Compile-time function '%s' has invalid resulting value\n", nodeExp->callx.func->varx.name);
				
				Expr* exp = vm->retVal->native.value;
				
				exp->next = NextExpr;
				*pNodeExp = exp;
				printf("'%s' generated expression of type: %s\n", decl->name, ExprNames[exp->type]); 
				if(exp->type == EXP_BIN)
				{
					printf("'%c'\n", exp->binx.op);
				}
			}
		}
		
		DeleteVM(vm);
	}
	
	ClearCode();
}
