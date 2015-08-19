// lang.c -- a language which compiles to mint vm bytecode
/* 
 * TODO:
 * - Have a CurFileIndex global which stores the constant index of the file being compiled currently (PERFORMANCE) 
 * - DO TYPE INFERENCES AT COMPILE TIME NOT PARSE TIME cause all symbols should be defined 
 * - FIX compiler structure, have a function to compile expressions which should have resulting values
 * 	 and a function to compile expressions which shouldn't
 * - (Possibly fixed, haven't tested new compile time dict creation) WEIRD BUG: Dict literals occasionally cause ffi_calls after their creation to fail? (Depends on # of elements in dict, idk, maybe dict creation pollutes the stack)
 * - the offset passed to getstruct/setstruct memmber should be a native object, not a number
 * - Finish meta api
 * - Safe extern calls (preserve stack size before and after, unless value is returned)
 * - Should function declarations require a 'return' modifier if they return values (so the programmer knows that they have to return values no matter what)
 *   or you could do compile-time checks to see if a function doesn't return (this would be difficult cause of control flow, etc)
 * - BUG - vm fails to load code if there is no _main function
 * - expand could leak values onto the stack if used incorrectly (but then again, that's not the job of the compiler to check, right?)
 * - MORE CONST OMG (CompileExpr doesn't [read: shouldn't] change the expressions)
 * - enums
 * 
 * REQUIRED FIXES:
 * - parenthesized function calls will pollute the stack if they aren't assigned (see FIX compiler structure for a possible fix)
 * 
 * IMPROVEMENTS:
 * - interpret intrinsic for compile time interpretation of simple expressions (addition, subtraction, etc)
 * - op_dict_*_rkey: maybe write a version of these which doesn't use a runtime key
 * 
 * PARTIALLY COMPLETE (I.E NOT FULLY SATISFIED WITH SOLUTION):
 * - extern aliasing (functionality is there but it's broken and doesn't compile)
 * - using compound binary operators causes an error
 * - include/require other scripts (copy bytecode into vm at runtime?): can only include things at compile time (with cmd line)
 */
#include "lang.h"

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
		{
			outPath = argv[++i];
		}
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
	VarStackDepth = 0;

	ResolveTypesExprList(exprHead);

	CompileExprList(exprHead);
	_AppendCode(OP_HALT, LineNumber, RegisterString(FileName)->index);
	
	FILE* out;
	
	if(!outPath)
		out = fopen("out.mb", "wb");
	else
		out = fopen(outPath, "wb");
	
	OutputCode(out);
	
	fclose(out);
	
	return 0;
}
