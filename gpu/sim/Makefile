
VERILATOR_ROOT=$(HOME)

# System test
MAIN=Main
MAIN_CPP=sim_main

.PHONY: build
build: verilate
	# Add OPT=-DVL_DEBUG to display debug output.
	cd obj_dir && make -j -f V$(MAIN).mk V$(MAIN)

.PHONY: verilate
verilate:
	verilator -Wall -Wno-fatal -CFLAGS "-I../../.. --std=c++17" --cc $(MAIN).v --exe $(MAIN_CPP).cpp

simple.o: simple.s
	../../as -o simple.o simple.s

.PHONY: simple
simple: build simple.o
	./obj_dir/V$(MAIN) simple.o
