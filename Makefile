GLSLANG_DIR     :=       $(HOME)/Downloads/glslang

simple.spv: simple.frag
	$(GLSLANG_DIR)/bin/glslangValidator -V100 -d -o simple.spv simple.frag
