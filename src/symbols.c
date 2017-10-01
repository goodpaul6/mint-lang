#include "lang.h"

int NumNumbers = 0;
int NumStrings = 0;

int NumGlobals = 0;
int NumFunctions = 0;
int NumExterns = 0;

ConstDecl* Constants = NULL;
ConstDecl* ConstantsCurrent = NULL;
 
FuncDecl* CurFunc = NULL;

FuncDecl* Functions = NULL;
FuncDecl* FunctionsCurrent = NULL;

VarDecl* VarList = NULL;
int VarScope = 0;
int DeepestScope = 0;

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

/*FuncDecl* RegisterExternAs(const char* name, const char* alias)
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
	
	decl->isLambda = 0;
	decl->returnType = NULL;
	decl->argTypes = NULL;
	decl->prevDecl = NULL;
	
	return decl;
}*/

void PushScope()
{
	++VarScope;
	if(VarScope > DeepestScope)
		DeepestScope = VarScope;
}

void PopScope()
{
	// remove non-global variables
	VarDecl** decl = &VarList;	

	while(*decl)
	{
		if((*decl)->scope == VarScope && !(*decl)->isGlobal)
		{
			VarDecl* removed = *decl;
			*decl = removed->next;
		}
		else
			decl = &(*decl)->next;
	}

	--VarScope;
}

FuncDecl* DeclareFunction(const char* name, int index)
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
	
	decl->type = NULL;
	
	decl->what = DECL_NORMAL;
	
	decl->hasEllipsis = 0;
	
	strcpy(decl->name, name);
	decl->index = index;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	
	decl->hasReturn = -1;

	decl->prevDecl = NULL;
	decl->upvalues = NULL;
	decl->envDecl = NULL;
	decl->scope = VarScope;

	decl->bodyHead = NULL;
	
	return decl;
}

FuncDecl* DeclareExtern(const char* name)
{
	FuncDecl* decl = DeclareFunction(name, NumExterns++);
	decl->what = DECL_EXTERN;
	return decl;
}

FuncDecl* EnterFunction(const char* name)
{
	CurFunc = DeclareFunction(name, NumFunctions++);
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
		if(decl->what != DECL_EXTERN_ALIASED)
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

FuncDecl* ReferenceExternRaw(const char* name)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_EXTERN_ALIASED || decl->what == DECL_EXTERN)
		{
			if(strcmp(decl->name, name) == 0)
				return decl;
		}
	}
	
	return NULL;
}

VarDecl* DeclareVariable(const char* name)
{	
	VarDecl* decl = malloc(sizeof(VarDecl));
	assert(decl);
	
	decl->next = VarList;
	VarList = decl;
	
	decl->isGlobal = 0;
		
	strcpy(decl->name, name);
	if(!CurFunc)
	{	
		decl->index = NumGlobals++;
		decl->isGlobal = 1;
	}
	else
		decl->index = CurFunc->numLocals++;
	
	decl->scope = VarScope;
	decl->type = NULL;
	
	return decl;
}

VarDecl* DeclareArgument(const char* name)
{
	if(!CurFunc)
		ErrorExit("(INTERNAL) Must be inside function when registering arguments\n");
	
	VarDecl* decl = DeclareVariable(name);
	--CurFunc->numLocals;
	decl->index = -(++CurFunc->numArgs);
	
	return decl;
}

VarDecl* ReferenceVariable(const char* name)
{
	for(int i = VarScope; i >= 0; --i)
	{
		for(VarDecl* decl = VarList; decl != NULL; decl = decl->next)
		{
			if(decl->scope == i && strcmp(decl->name, name) == 0)
				return decl;
		}
	}
	
	return NULL;
}

// TODO: Idk man, this code looks pretty repetitive
FuncDecl* GetBinaryOverload(const TypeHint* a, const TypeHint* b, int op)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_OPERATOR)
		{
			if(decl->op == op && decl->type->func.numArgs == 2 && CompareTypes(decl->type->func.args[0], a) && CompareTypes(decl->type->func.args[1], b))
				return decl;
		}
	}
	
	return NULL;
}

FuncDecl* GetUnaryOverload(const TypeHint* a, int op)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_OPERATOR)
		{
			if(decl->op == op && decl->type->func.numArgs == 1 && CompareTypes(decl->type->func.args[0], a))
				return decl;
		}
	}
	
	return NULL;
}

FuncDecl* GetSpecialOverload(const TypeHint* a, const TypeHint* b, int op)
{
	for(FuncDecl* decl = Functions; decl != NULL; decl = decl->next)
	{
		if(decl->what == DECL_OPERATOR)
		{
			if(decl->op == op && CompareTypes(decl->type->func.args[0], a) && (!b || CompareTypes(decl->type->func.args[1], b))) 
				return decl;
		}
	}
	
	return NULL;
}
