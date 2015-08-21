gcc lang.c typer.c lexer.c parser.c compiler.c codegen.c symbols.c utils.c -Wall -o bin/lang -std=c99 -ggdb
gcc vm.c dict.c hash.c stdvm.c -Wall -o bin/mint -std=c99 -lffi -ldl -lm -g
