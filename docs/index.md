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

* TODO LAWRENCE: Register assignment, RISC-V generation, links to papers.

# RISC-V assembler

The assembler is written in C++. It's a straightforward two-pass recursive-descent
assembler from the official RISC-V mnemonics to RISC-V opcodes. It doesn't support
the pseudo-instructions one sees in some RISC-V listings or disassemblers
(e.g., `nop` for `addi x0, x0, 0`).

Its source is in `as.cpp`. Use the `-v` flag to dump a listing to the standard output.

    % ./as -v file.s > file.lst

# RISC-V emulator

* TODO BRAD: Write up.

# Multi-core RISC-V implementation in Verilog

* TODO BRAD: Write up.

# Results

* TODO BRAD: Numbers from spreadsheet.

