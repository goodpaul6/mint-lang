// stdvm.c -- standard virtual machine (nothing but the standard c functions)
#include <stdlib.h>
#include <string.h>

#include "vm.h"

int main(int argc, char* argv[])
{
	if(argc >= 2)
	{
		FILE* bin = fopen(argv[1], "rb");
		if(!bin)
		{
			fprintf(stderr, "Failed to open file '%s' for execution\n", argv[1]);
			return 1;
		}

		char debugFlag = 0;
		for(int i = 2; i < argc; ++i)
		{
			if(strcmp(argv[i], "-d") == 0)
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
	
	return 0;
}
