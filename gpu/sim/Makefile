
VERILATOR_ROOT=$(HOME)
CFG_CXXFLAGS_STD_NEWEST=-std=c++17

MAIN=Main
MAIN_CPP=sim_main
OBJ_DIR=obj_dir

# Display debug output.
# V_BUILD_OPTS=OPT=-DVL_DEBUG

.PHONY: build
build: verilate
	make $(V_BUILD_OPTS) -C $(OBJ_DIR) -j -f V$(MAIN).mk V$(MAIN)

.PHONY: verilate
verilate:
	if [ -d $(OBJ_DIR) ]; then rm -r $(OBJ_DIR); fi
	verilator --threads 1 -y ../fpu -v ../fpu/primitives.v -y ../shadercore -Wall -Wno-fatal -CFLAGS "-I../../.. -O2 --std=c++17" --cc $(MAIN).v --exe ../../riscv-disas.c $(MAIN_CPP).cpp

simple.o: simple.s
	../../as -v -o simple.o simple.s > simple.lst

.PHONY: simple
simple: build simple.o
	./$(OBJ_DIR)/V$(MAIN) simple.o

red_green.s: ../../shaders/red_green.frag
	(cd ../.. && ./shade -c -O -o gpu/sim/red_green.s -v -S shaders/red_green.frag > out)

red_green.o: red_green.s
	../../as -v -o red_green.o red_green.s > red_green.lst

.PHONY: red_green
red_green: build red_green.o
	./$(OBJ_DIR)/V$(MAIN) -j 1 --term red_green.o

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
