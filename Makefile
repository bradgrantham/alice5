GLSLANG_DIR     :=       $(HOME)/Downloads/glslang

simple.spv: simple.frag
	$(GLSLANG_DIR)/bin/glslangValidator -H -V100 -d -o simple.spv simple.frag

.PHONY: clean
clean:
	if [ -f simple.spv ]; then rm simple.spv; fi
