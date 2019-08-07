
VERILATOR_ROOT=$(HOME)

# System test
MAIN=Main
MAIN_CPP=sim_main

.PHONY: run
run: build
	./obj_dir/V$(MAIN)

.PHONY: build
build: verilate
	# Add OPT=-DVL_DEBUG to display debug output.
	cd obj_dir && make -j -f V$(MAIN).mk V$(MAIN)

.PHONY: verilate
verilate:
	verilator -Wall --cc $(MAIN).v --exe $(MAIN_CPP).cpp

