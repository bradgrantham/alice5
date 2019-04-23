tests: tests.cpp riscv-disas.h riscv-disas.c ../gpuemu.h
	c++ -Wall -g --std=c++17 tests.cpp riscv-disas.c -o tests -I..

run: run.cpp riscv-disas.h riscv-disas.c ../gpuemu.h
	c++ -Wall -g --std=c++17 run.cpp riscv-disas.c -o run -I..
