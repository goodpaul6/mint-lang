// lang.c -- a language which compiles to mint vm bytecode
/* 
 * TODO
 * - Fix lambdas
 * - DECL_LAMBDA unnecessary (just use DECL_NORMAL or create a DECL_ANON or something)
 * - token names in error messages
 * - store function names as indices into the stringConstants array in the vm
 * - macro system sorta working except declaring variables in macros breaks the damn thing
 * - certain top level code breaks binaries
 * - Adjustable vm stack and indirection stack size
 * - enums (could be implemented using macros, if the macro system actually ends up working)
 * 
 * IMPROVEMENTS:
 * - store length of string in object (also, make strings immutable)
 * - allocate number constants on an "operand stack" to speed up the operations
 * - native lambdas (i.e implement them directly in the vm)
 * - op_dict_*_rkey: maybe write a version of these which doesn't use a runtime key
 * 
 * PARTIALLY COMPLETE (I.E NOT FULLY SATISFIED WITH SOLUTION):
 * - using compound binary operators causes an error
 * - include/require other scripts (copy bytecode into vm at runtime?): can only include things at compile time (with cmd line)
 */
#include "lang.h"

char ProduceDebugInfo = 0;

const char* StandardSourceSearchPath = "C:\\Mint\\src\\";
const char* StandardLibSearchPath = "C:\\Mint\\lib\\";

int main(int argc, char* argv[])
{
	if(argc == 1)
		ErrorExit("no input files detected!\n");
	
	const char* outPath = NULL;

	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;

	for(int i = 1; i < argc; ++i)
	{
		if(strcmp(argv[i], "-o") == 0)
			outPath = argv[++i];
		else if(strcmp(argv[i], "-g") == 0)
			ProduceDebugInfo = 1;
		else if(strcmp(argv[i], "-l") == 0)
		{		
			/*FILE* in = fopen(argv[++i], "rb");
			if(!in)
			{
				char buf[256];
				strcpy(buf, StandardLibSearchPath);
				strcat(buf, argv[i]);
				in = fopen(buf, "rb");
				if(!in)
					ErrorExit("Cannot open file '%s' for reading\n", argv[i]);
			}
			
			if(!exprHead)
			{
				exprHead = ParseBinaryFile(in, argv[i]);
				exprCurrent = exprHead;
			}
			else
			{
				exprCurrent->next = ParseBinaryFile(in, argv[i]);
				exprCurrent = exprCurrent->next;
			}
			
			fclose(in);*/
			
			// alright, deprecating binary file linking for now
			printf("cannot link binary files...\n");
		}
		else
		{
			FILE* in = fopen(argv[i], "r");
			if(!in)
			{
				char buf[256];
				strcpy(buf, StandardSourceSearchPath);
				strcat(buf, argv[i]);
				in = fopen(buf, "r");
				
				if(!in)
					ErrorExit("Cannot open file '%s' for reading\n", argv[i]);
			}
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
	
	// NOTE: setting this to zero so that any variables declared at compile time are global
	VarScope = 0;
	CurFunc = NULL;
	
	ResolveTypesExprList(exprHead);
	
	ExecuteMacros(&exprHead);

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
