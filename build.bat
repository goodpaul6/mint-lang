@echo off
gcc -o bin/lang lang.c -std=c99
gcc -o bin/sdlvm vm.c sdlvm.c -std=c99 -lmingw32 -lSDL2main -lSDL2
gcc -o bin/engvm vm.c engvm.c -std=c99 -lmingw32 -lSDL2main -lSDL2
