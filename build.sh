gcc lang.c -Wall -o bin/lang -std=c99
gcc vm.c dict.c hash.c stdvm.c -Wall -o bin/mint -std=c99 -lffi -ldl -lm -g
