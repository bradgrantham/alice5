# Or, GLSLANG_BINARY_DIR = $(WHERE_PACKAGE_UNPACKED)/bin
GLSLANG_BINARY_DIR     :=       $(HOME)/trees/glslang/build/StandAlone
GLSLANG_SOURCE_DIR     :=       $(HOME)/trees/glslang
SPIRV_TOOLS_SOURCE_DIR     :=       $(HOME)/trees/glslang/External/spirv-tools

GLSLANG_CC_OPTIONS     :=      -g -DAMD_EXTENSIONS -DENABLE_HLSL -DENABLE_OPT=1 -DGLSLANG_OSINCLUDE_UNIX -DNV_EXTENSIONS -DSPIRV_CHECK_CONTEXT -DSPIRV_COLOR_TERMINAL -DSPIRV_MAC -DSPIRV_TOOLS_IMPLEMENTATION -DSPIRV_TOOLS_SHAREDLIB -DSPIRV_Tools_shared_EXPORTS

CXXFLAGS        :=      -I$(SPIRV_TOOLS_SOURCE_DIR)/include -I$(GLSLANG_SOURCE_DIR) $(GLSLANG_CC_OPTIONS) --std=c++17

LDFLAGS         :=      $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/SPIRV/libSPIRV.a $(GLSLANG_SOURCE_DIR)/build/SPIRV/libSPVRemapper.a $(GLSLANG_SOURCE_DIR)/build/StandAlone/libglslang-default-resource-limits.a -lpthread $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/OGLCompilersDLL/libOGLCompiler.a $(GLSLANG_SOURCE_DIR)/build/glslang/OSDependent/Unix/libOSDependent.a $(GLSLANG_SOURCE_DIR)/build/hlsl/libHLSL.a $(GLSLANG_SOURCE_DIR)/build/External/spirv-tools/source/opt/libSPIRV-Tools-opt.a $(GLSLANG_SOURCE_DIR)/build/External/spirv-tools/source/libSPIRV-Tools.a

# $(GLSLANG_SOURCE_DIR)/build/glslang/libglslang.a $(GLSLANG_SOURCE_DIR)/build/glslang/OSDependent/Unix/libOSDependent.a

simple.spv: simple.frag
	(cat preamble.frag simple.frag) | $(GLSLANG_BINARY_DIR)/glslangValidator -H -V100 -d -o simple.spv --stdin -S frag

.PHONY: clean
clean:
	if [ -f simple.spv ]; then rm simple.spv; fi
