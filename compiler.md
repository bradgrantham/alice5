
# Compiler

These are the stages that the program goes through to get compiled.

The program starts out as a text GLSL source file. It is compiled by the
glslang compiler (which we did not write) to SPIR-V intermediate representation (IR).
This IR is in static single assignment (SSA) form, which means that it pretends
that it has infinite registers, that these registers can hold any value (scalars,
vectors, matrices, or structs), and that each register can only be written to in one
place in the program.

If the `-O` flag is specified on the command line, this SPIR-V code is
optimized using glslang's optimizer (which we also didn't write).

We convert the SPIR-V code (which is in a packed binary format) to an in-memory
data structure consisting of a sequence of objects, one for each instruction. Each
one is an instance of a subclass of the `Instruction` class. For example
the SPIR-V instruction `OpFAdd` (which adds floating point scalars or
vectors) is converted to an instance of the `InsnFAdd` class and its parameters
are unpacked into fields. At the same time we record constants, labels,
functions, and other metadata. At this point we can interpret the code to
generate an image:

    ./shade --term -O shaders/wavy.frag

If the code is to be compiled, it then goes through further analysis
and transformations. The three main goals of these transformations are:

1. To expand complex registers to simple ones (e.g., vec3 to three floats).

2. To get rid of phi instructions (see below for explanation).

3. To assign physical registers.

The first pass expands vectors. For example, the SPIR-V `OpFAdd` instruction
can be used to add two vec3 registers, resulting in a new vec3 register. Our
target architecture has no vector registers or instructions, so we want to
assign three floating point registers to the source and destination registers
and replace the single `OpFAdd` instruction with three `OpFAdd` instructions
that operate on the individual scalar registers. These expanded registers are
still SSA (virtual) registers, not physical registers. This pass is done in
`Program::expandVectors()`.

The next few steps (all in `Program::postParse()`) deal with SSA _blocks_. SSA
code is broken into blocks of code, where each block starts with an `OpLabel`
and ends with a terminating instruction of some kind (`OpBranch`,
`OpBranchConditional`, `OpSwitch`, `OpReturn`, `OpReturnValue`, `OpKill`, or
`OpUnreachable`). It contains no other labels. In other words, each block runs
straight from its first to its last instruction. There's no "falling through"
from block to block. This means that the order of blocks is arbitrary. Each
function has a starting block that should be executed first.

To discover the blocks, we go through each `OpLabel` in the program and
look for the first subsequent terminating instruction. The block ID is
the ID of its starting label.

Then, for each block, we look at the labels it might jump to in its terminating
instruction (`OpBranch`, `OpBranchConditional`, etc.). This creates a successor
graph from block to block. We also compute the inverse (predecessor) graph. The
first block of each function has no predecessor block and blocks that terminate
with return instructions have no successor blocks. Some non-initial blocks may
have no predecessors, such as "continue" blocks of loops that are never called.

For each instruction we then record its block ID. We also compute its
predecessor and successor instructions. For most instructions this is just the
lexically previous or next instruction, but for instructions at the beginning
or end of each block we follow the block's predecessor or successor blocks.
This gives us a full _control flow graph_ (CFG).

We next need to figure out what (virtual) registers are _live_ (in use) at each
instruction. A register is live from the time it's defined until it's last
referenced. This is computed using an iterative algorithm that propagates
register usage through the CFG. As a result we also know which registers are
required to already be defined at the start of each block and which are still
alive at the end of each block.

We then compute the _dominance tree_. A block _dominates_ another if it is
guaranteed to run before the other. The first block of a function dominates all
others, for example. But in an if/else statement, neither the "then" nor the
"else" block dominates any other block, since neither is guaranteed to run.
This is computed using an iterative algorithm that propagates dominance forward
from each function's initial block. Once we have the full dominance _graph_, we
can compute the dominance _tree_, which is a simplified version where each
block's _immediate dominating block_ (idom) is computed. The idom of a block is
its dominating block that's "closest" to it. Each function has an idom tree
that is rooted at its initial block. The idom tree is used to process blocks in
an order that guarantees that we will see all register definitions before we
see their usage.

The next pass converts some SPIR-V instructions to instructions that are more
similar to our target architecture (which is RISC-V-like). For example, the
integer-add instruction (OpIAdd) might be adding two registers (which should
generate an `add` instruction) or a register and a constant that can fit in 12
bits (which should generate an `addi` instruction) or a register and a constant
that cannot fit in 12 bits (which should generate several instructions that
load the constant into a temporary register). Since these transformations may
use temporary registers, they must be done before register assignment. They
cannot be done at the end when the RISC-V code is being emitted. This pass is
done in `Compiler::transformInstructions()`.

Before we can assign physical registers, we must get rid of _phi instructions_.
If SSA truly only assigned each register once, then you couldn't make use of
any register assigned in a conditional block, since you weren't later guaranteed
that the block had been executed. To solve this problem, SSA form inserts phi
instructions immediately after optional blocks like those of if/then statements.
They are similar to ternary expressions in C:

    if (condition) {
        a = 5;
    } else {
        b = 6;
    }
    c = ... ? a : b;

The `...` above can't be expressed in C, but means "if we came from the first
branch". It's a magical check that knows where program flow just came from,
and is a way to merge data flow together again after a split in control flow.
The RISC-V instruction set doesn't have a phi instruction, so we must get rid
of it. The easiest solution is to remove it altogether by assigning all
of its arguments and results to the same virtual register:

    if (condition) {
        c = 5;
    } else {
        c = 6;
    }

It is a violation of SSA rules to assign to `c` in two places in the code, so
this process is sometimes called "translating out of SSA form". Since the
result of a phi instruction might be used as an argument to another phi
instruction, we transitively compute all virtual registers that participate in
phi instructions together so we can assign them all the same virtual (and
eventually physical) register. This is done in `Compiler::translateOutOfSsa()`.

The next pass assigns virtual registers to physical registers. We had
previously computed which virtual registers were live at each instruction. If
any instruction has more live registers than we have physical registers, then
some virtual registers must be spilled to memory. (We do not yet do this.)

The initial block of a function may have registers already live at its start.
These are constants that the SSA form puts into registers in the file's header;
they are not explicitly loaded into registers by instruction in the code. We
assign physical registers to these constants, and we will load them later at
the start of functions when generating code.

To assign physical registers to all other registers, we recurse down the
idom tree, assigning physical registers to virtual registers as they
are defined. At the end of a virtual register's live range, we free up
the physical register.

Finally, the assembly code is generated. The assembler `as` is used to assemble
this to RISC-V-compatible opcodes. The resulting binary can be simulated with
the `gpuemu` program or sent to the hardware for execution.

