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

VarDecl* VarStack[MAX_SCOPES];
int VarStackDepth = 0;

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
	VarStack[++VarStackDepth] = NULL;
}

void PopScope()
{
	--VarStackDepth;
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
	
	decl->isAliased = 0;
	decl->isExtern = 0;
	decl->hasEllipsis = 0;
	
	strcpy(decl->name, name);
	decl->index = index;
	decl->numArgs = 0;
	decl->numLocals = 0;
	decl->pc = -1;
	
	decl->hasReturn = -1;

	decl->isLambda = 0;
	decl->prevDecl = NULL;
	decl->upvalues = NULL;
	decl->envDecl = NULL;
	decl->scope = VarStackDepth;

	decl->bodyHead = NULL;
	
	return decl;
}

FuncDecl* DeclareExtern(const char* name)
{
	FuncDecl* decl = DeclareFunction(name, NumExterns++);
	decl->isExtern = 1;
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
