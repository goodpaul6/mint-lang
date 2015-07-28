@echo off
gcc -o bin/lang lang.c -std=c99 -Iinclude
gcc -o bin/mint vm.c dict.c hash.c stdvm.c -std=c99 -Iinclude -Llib -ldyncall_s -ldyncallback_s -ldynload_s
