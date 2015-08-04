gcc -Wall -o bin/lang lang.c -std=c99 -Iinclude
gcc -Wall -o bin/mint vm.c dict.c hash.c stdvm.c -std=c99 -lffi -g
