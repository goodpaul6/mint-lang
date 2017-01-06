@echo off
pushd "mint vm"
clang -fsanitize=address *.c -o ../Debug/mint -D_CRT_SECURE_NO_WARNINGS
popd