gcc -Wall -o bin/lang lang.c lexer.c parser.c symbols.c typer.c linker.c compiler.c codegen.c utils.c macro.c vm.c dict.c hash.c linkedlist.c -std=c99 -Iinclude -g
gcc -Wall -o bin/mint vm.c dict.c hash.c stdvm.c -std=c99 -g
