# Alice 5 - SPIR-V fragment shader GPU core based on RISC-V

Our goal with Alice 5 is to create our own shader core and supporting hardware and software to download and display ShaderToy "toys". We hope to explore and understand to various degrees:
1. ShaderToy shaders and structures
1. Shader parsing using glslang to SPIR-V
1. Shader compilation using software written from scratch
1. RISC-V instruction set as a GPU shader core framework
1. GPU hardware architecture and optimization

We have #1 and #2 working, #3 and #4 without texturing or multipass and are working on those.  We're in the process of first synthesis of our Verilog to an Altera Cyclone V.  After that, we'll look into #5 and texturing and multipass in hardware.
