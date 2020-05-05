# Overview

The Alice V project aimed to build a tablet that ran
[ShaderToy](https://www.shadertoy.com/) shaders using a custom-built
FPGA fragment shader processor. It was a continuation of the first
four [Alice projects](https://lkesteloot.github.io/alice/). The
project did not achieve its goals, running several orders of magnitude
too slowly.

Project members were [Brad Grantham](http://plunk.org/~grantham/) and
[Lawrence Kesteloot](https://www.teamten.com/lawrence/).

The project included the following components:

* A SPIR-V compiler front-end (not written by us).
* A SPIR-V compiler back-end, to RISC-V assembly, by Lawrence.
* A RISC-V assembler, by Lawrence.
* A RISC-V emulator, by Brad.
* A multi-core RISC-V implementation in Verilog, by Brad.

# SPIR-V compiler front-end

* TODO BRAD: Library we used.
* TODO LAWRENCE: IR interpreter.

# SPIR-V compiler back-end

The compiler front-end generates what's known as Static Single Assignment (SSA)
form. This is a register-based instruction set that only ever writes to each
register once. To get around this limitation, they use two tricks: infinite
registers and a phi instruction (for merging results from different paths).

The main job of a back-end is to assign the infinite registers to the finite
registers of a real architecture (in our case, RISC-V). There are various
papers describing how to do this, but in practice few real-world compilers
use this technique, mostly because SSA algorithms were invented after all
mainstream compilers were written.

Our back-end does a reasonable job, but still has too much "spilling" of
registers (temporarily saving register contents to memory).

If you're interested in writing a back-end based on SSA,
[here are Lawrence's notes](compiler.md).

# RISC-V assembler

The assembler is written in C++. It's a straightforward two-pass recursive-descent
assembler from the official RISC-V mnemonics to RISC-V opcodes. It doesn't support
the pseudo-instructions one sees in some RISC-V listings or disassemblers
(e.g., `nop` for `addi x0, x0, 0`).

Its source is in `as.cpp`. Use the `-v` flag to dump a listing to the standard output.

    % ./as -v file.s > file.lst

The assembler outputs a simple binary format containing the
instructions for the "instruction RAM" and initialization data for
the "data RAM".  We called these both RAM although there's no way
for our RISC-V implementation to alter its own instructions, so
effectively it's an instruction ROM from the shader program's point
of view.

# RISC-V architecture variant

Our assembler's target is RISC-V IMF (integer, multiply, and float
instruction sets) with separate instruction and data memory, also
known as a Harvard architecture.

Our GPU design assumes one or more of these cores operating
simultaneously.

During execution, the segment of "data memory" from 0x0 of some
predefined size (32K in our current configuration) is private to
each shader core.  GPU cores also share a segment of memory at
`0x80000000` into which the GPU cores write their rasterized pixels.

We use the `EBREAK` instruction to indicate completion of the shader program.
On `EBREAK`, execution halts and a bit is set to indicate completion.

# RISC-V emulator

The [emulator](../emu.h) consists of a header-only implementation
of our Harvard architecture IMF RISC-V and a C++ driver.  The driver
loads a shader program and initializer data from our simplified
binary format into separate instruction RAM and data RAM for each
core, and maintains an emulated shared RAM into which every core
writes its results.

The driver makes the assumption that the shader is structured to
rasterize a row of pixels.  For each row, the driver program sets
per-row pixel variables including the row address, then invokes the
shader.  In this case, a shader halting means that a row has been
shaded and written to shared RAM.

The emulator creates multiple threads, both to test concurrency of
cores but also to reduce our testing time.

On completion, the emulator writes the resulting image to a file
and optionally to the screen using the iTerm image escape codes.

# Multi-core RISC-V implementation in Verilog

* TODO BRAD: Write up.

# Results

* TODO BRAD: Numbers from spreadsheet.

