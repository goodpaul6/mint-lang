@echo off
gcc -o bin/lang lang.c -std=c99
gcc -o bin/sdlvm vm.c sdlvm.c dict.c hash.c -std=c99 -lmingw32 -lSDL2main -lSDL2
gcc -o bin/sfvm vm.c dict.c hash.c sfvm.c -std=c99 -Iinclude -Llib -lcsfml-window -lcsfml-graphics -lcsfml-system
gcc -o bin/mint vm.c dict.c hash.c stdvm.c -std=c99
