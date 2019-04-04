# Or, GLSLANG_BINARY_DIR = $(WHERE_PACKAGE_UNPACKED)/bin
TREES := $(HOME)/trees
GLSLANG_SOURCE_DIR     :=       $(TREES)/glslang
GLSLANG_BINARY_DIR     :=       $(GLSLANG_SOURCE_DIR)/build/StandAlone
SPIRV_TOOLS_SOURCE_DIR     :=       $(GLSLANG_SOURCE_DIR)/External/spirv-tools

OPT     := -O2 -g

GLSLANG_CC_OPTIONS     :=      -DAMD_EXTENSIONS -DENABLE_HLSL -DENABLE_OPT=1 -DGLSLANG_OSINCLUDE_UNIX -DNV_EXTENSIONS -DSPIRV_CHECK_CONTEXT -DSPIRV_COLOR_TERMINAL -DSPIRV_MAC -DSPIRV_TOOLS_IMPLEMENTATION -DSPIRV_TOOLS_SHAREDLIB -DSPIRV_Tools_shared_EXPORTS

CXXFLAGS        :=      $(OPT) -Wall -I$(SPIRV_TOOLS_SOURCE_DIR)/include -I$(GLSLANG_SOURCE_DIR) $(GLSLANG_CC_OPTIONS) --std=c++17

LDFLAGS         :=      $(OPT) $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/SPIRV/libSPIRV.a $(GLSLANG_SOURCE_DIR)/build/SPIRV/libSPVRemapper.a $(GLSLANG_SOURCE_DIR)/build/StandAlone/libglslang-default-resource-limits.a -lpthread $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/OGLCompilersDLL/libOGLCompiler.a $(GLSLANG_SOURCE_DIR)/build/glslang/OSDependent/Unix/libOSDependent.a $(GLSLANG_SOURCE_DIR)/build/hlsl/libHLSL.a $(GLSLANG_SOURCE_DIR)/build/External/spirv-tools/source/opt/libSPIRV-Tools-opt.a $(GLSLANG_SOURCE_DIR)/build/External/spirv-tools/source/libSPIRV-Tools.a

# $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/glslang/OSDependent/Unix/libOSDependent.a

default: shade

shade: shade.cpp GLSL.std.450.h GLSLstd450_opcode_to_string.h basic_types.h interpreter.h opcode_decl.h opcode_decode.h opcode_impl.h opcode_struct_decl.h opcode_structs.h opcode_to_string.h spirv.h json.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) shade.cpp -o $@ $(LDLIBS)

simple.spv: simple.frag
	cat preamble.frag simple.frag epilogue.frag | $(GLSLANG_BINARY_DIR)/glslangValidator -H -V100 -d -o simple.spv --stdin -S frag

.PHONY: generate
generate:
	python3 generate_ops.py $(GLSLANG_SOURCE_DIR)/External/spirv-tools/external/spirv-headers/include/spirv/1.2/spirv.core.grammar.json $(GLSLANG_SOURCE_DIR)/External/spirv-tools/external/spirv-headers/include/spirv/1.2/extinst.glsl.std.450.grammar.json

.PHONY: clean
clean:
	if [ -f simple.spv ]; then rm simple.spv; fi
	if [ -f shade ]; then rm shade; fi

.PHONY: dis
dis: simple.spv
	$(GLSLANG_SOURCE_DIR)/build/External/spirv-tools/tools/spirv-dis simple.spv | cat
