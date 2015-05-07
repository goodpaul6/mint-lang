@echo off
gcc -o bin/lang lang.c -std=c99
gcc -o bin/sdlvm vm.c sdlvm.c dict.c -std=c99 -lmingw32 -lSDL2main -lSDL2
