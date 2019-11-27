
CFG_CXXFLAGS_STD_NEWEST=-std=c++17
CXXFLAGS=-I../.. -std=c++17

.PHONY: all
all: drive

drive: drive.cpp ../../risc-v.h ../../timer.h ../../disassemble.h ../../objectfile.h

simple.o: simple.s
	../../as -v -o simple.o simple.s > simple.lst

red_green.s: ../../shaders/red_green.frag
	(cd ../.. && ./shade -c -O -o gpu/sim/red_green.s -v -S shaders/red_green.frag > out)

red_green.o: red_green.s
	../../as -v -o red_green.o red_green.s > red_green.lst

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
