// lang.c -- a language which compiles to mint vm bytecode
/* 
 * TODO
 * - window
 * - multiple stacks:
 * 		- have a stack for storing numerical values
 * 		- a stack for reference objects (pointers, arrays, strings)	
 * 
 * - function overloading:
 * 		- syntax for function pointers:
 * 			function_name with number, string # NOTE: the amount of type parameters parsed is = the number of typed arguments to the function
 * 
 * - any setting operations on numbers should be copying (i.e obj->number = newObj->number) not assigning (obj = newObj)
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

	char compile = 0;
	Expr* exprHead = NULL;
	Expr* exprCurrent = NULL;

	for(int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-o") == 0)
			outPath = argv[++i];
		else if (strcmp(argv[i], "-c") == 0)
			compile = 1;
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
		else if(compile)
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
		else
		{
			if (argc >= 2)
			{
				FILE* bin = fopen(argv[1], "rb");
				if (!bin)
				{
					fprintf(stderr, "Failed to open file '%s' for execution\n", argv[1]);
					return 1;
				}

				char debugFlag = 0;
				for (int i = 2; i < argc; ++i)
				{
					if (strcmp(argv[i], "-g") == 0)
						debugFlag = 1;
				}
				VM* vm = NewVM();

				vm->debug = debugFlag;

				LoadBinaryFile(vm, bin);
				fclose(bin);

				HookStandardLibrary(vm);

				RunVM(vm);

				DeleteVM(vm);
			}
			else
			{
				fprintf(stderr, "Expected at least some command line arguments!\n");
				exit(1);
			}
		}
	}					
	
	if (compile)
	{
		// NOTE: setting this to zero so that any variables declared at compile time are global
		VarScope = 0;
		CurFunc = NULL;

		ResolveTypesExprList(exprHead);

        // Macros don't seem to be working so until that's figured out, no macros
		//ExecuteMacros(&exprHead);

		CompileExprList(exprHead);
		AppendCode(OP_HALT);

		FILE* out;

		if (!outPath)
			out = fopen("out.mb", "wb");
		else
			out = fopen(outPath, "wb");

		if (!out)
		{
			fprintf(stderr, "Failed to open '%s' for writing\n", outPath);
			exit(1);
		}

		OutputCode(out);

		fclose(out);
	}
	
	return 0;
}
