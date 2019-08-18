
VERILATOR_ROOT=$(HOME)

MAIN=Main
MAIN_CPP=sim_main
OBJ_DIR=obj_dir

# Display debug output.
# V_BUILD_OPTS=OPT=-DVL_DEBUG

.PHONY: run
run: build
	./$(OBJ_DIR)/V$(MAIN)


.PHONY: build
build: verilate
	make $(V_BUILD_OPTS) -C $(OBJ_DIR) -j -f V$(MAIN).mk V$(MAIN)

.PHONY: verilate
verilate:
	if [ -d $(OBJ_DIR) ]; then rm -r $(OBJ_DIR); fi
	verilator -y ../fpu -v ../fpu/primitives.v -Wall -Wno-fatal -CFLAGS "-I../../.. --std=c++17" --cc $(MAIN).v --exe $(MAIN_CPP).cpp

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
