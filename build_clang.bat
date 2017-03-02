@echo off
pushd "mint vm"
clang *.c -o ../Debug/mint.exe -D_CRT_SECURE_NO_WARNINGS
popd