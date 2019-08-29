
VERILATOR_ROOT=$(HOME)

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
	verilator -y ../fpu -v ../fpu/primitives.v -y ../shadercore -Wall -Wno-fatal -CFLAGS "-I../../.. --std=c++17" --cc $(MAIN).v --exe ../../riscv-disas.c $(MAIN_CPP).cpp

simple.o: simple.s
	../../as -v -o simple.o simple.s > simple.lst

.PHONY: simple
simple: build simple.o
	./$(OBJ_DIR)/V$(MAIN) simple.o

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
