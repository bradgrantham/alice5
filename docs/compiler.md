# Notes on the compiler back end

Lawrence's notes while researching SSA register assignment.

## Terminology

* **Static Single Assignment (SSA) form**: Every variable is assigned to exactly once.
* **Phi instruction**: A pseudo-instruction that copies its parameters to its
  result depending on where control came from. The copies are conceptually done
  in parallel.
* **Phi congruence class**: Set of variables involved in a phi instructions, and
  its transitive closure.
* **Conventional SSA (CSSA)**: SSA where variables in phi classes don't interfere.
* **Transformed SSA (TSSA)**: SSA after some optimizations where variables in phi
  classes do interfere.
* **Critical edge**: Edge between a block with multiple successors and a block
  with multiple predecessors.

## Resources

### [Efficiently Computing SSA Form and the Control Dependence Graph](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.100.6361&rep=rep1&type=pdf) (Cytron et al, 1991)

Original paper that showed that SSA could be computed efficiently. Introduces
the dominance frontier. For getting out of SSA (before register assignment)
they just add move instructions at the end of the previous blocks, and do some
simple optimizations to remove unnecessary moves. Nothing on spilling that I
could find.

### Translating Out of Static Single Assignment Form (Sreedhar et al, 1999)

Introduces concepts of CSSA, TSSA, and phi congruence class. Explains how to
get from TSSA to CSSA.

### Register Allocation for Programs in SSA Form (Sebastian Hack, 2007, [PDF](https://publikationen.bibliothek.kit.edu/1000007166/6532))

Thesis that proved that doing graph coloring in SSA form was possible.

4.2 Spilling. Define memory variables, similar to ordinary (virtual) variables,
but not assigned to physical registers. Instead assigned to memory locations,
and we assume there are plenty of them. We can copy from ordinary variable to
memory (spill) and vice-versa (reload). Also the phi operation can select from
memory locations, copying to other memory locations.

### The Design and Implementation of a SSA-based Register Allocator ([Pereira](https://scholar.google.com/citations?hl=en&user=xZfJARoAAAAJ&view_op=list_works&sortby=pubdate), 2007, [PDF](http://compilers.cs.ucla.edu/fernando/projects/soc/reports/long_tech.pdf), [PDF](https://pdfs.semanticscholar.org/ffd7/6c45a235cd9bde19d394e76afbf0dc24158a.pdf) (better?), [PDF](http://compilers.cs.ucla.edu/fernando/projects/soc/reports/short_tech.pdf))

Presents the entire pipeline for allocating registers.

4.1 Compute the intervals of virtual registers. I don't know why this is done
here first and not after PhiLifting.

4.2 Presents an algorithm (PhiLifting) to convert a TSSA program to CSSA.

I don't see the advantage of creating vi in the PhiLifting algorithm. (Also its
location is wrong, it should be placed after the phi instruction.) Maybe if the
vi loops around it might interfere with its own parameters? (See how v9 loops
through block 3.) Or it might cause itself to become part of another phi
equivalence class for another phi? I don't see how it could once we've added
the vij copies. Anyway creating vi will be free if we use the push/pop method
of keeping track of free physical registers.

Why is Interval Analysis done first? Doesn't it have to be done again after
PhiLifting is applied, since variables were created?

4.3 Creates the phi equivalence classes. After PhiLifting each phi has its own
class, so this is trivial.

4.4 He presents PhiMemCoalesce, an optional algorithm that removes copies
inserted by PhiLifting so long as they keep the program CSSA. I don't
understand this algorithm. He uses the expression Qi ∩ Qj, which implies
intersection of variables in equivalence classes, but that can't be right, the
intersection will always be empty. And the middle paragraph in section 4.4
talks about live ranges. So I think that expression means “none of the live
ranges of the variables in the equivalence classes overlap”, which is the
definition of CSSA, so that would make sense.

4.5 He assigns registers. This is done by traversing the idom tree in pre-order
and assigning registers greedily. Some details:

4.5.i For move instructions where the source doesn't outlive the instruction,
both virtuals are assigned to the same physical registers. I think this is free
if we use a stack for free physical registers.

4.5.ii He presents an algorithm (ComputePreference) to increase the number of
phi-participating variables that map to the same register. When coloring a
register v, ComputePreference is called to find what physical registers have
been used in related phi instructions. If v is the destination of a phi, look
at its parameters. If it's anywhere a parameter to a phi, look at its
destination (but not other parameters?). Here I'd expect to use the phi
equivalence classes.

4.5.iii He presents an algorithm (ComputeWeight) to help figure out which
variables to spill.

4.5.iv To handle pre-colored registers, he first assigns registers to variables
that must be pre-colored, then makes sure those registers aren't assigned to
virtuals that overlap. This requires looking at the interference graph, since
when assigning to a variable you'll need to know if its future range interferes
with a variable that's already been assigned. I don't know if we'll have
pre-colored virtual registers.

4.6 Insert the phi parallel copies at the end of blocks that jump to other
blocks that start with phi. The copies are parallel, so first do any copy whose
destination isn't needed later. Anything left is a permutation that can be
handled with exchange instructions, three xors, or push/pop for a temporary. In
fact a free register may be available, but isn't guaranteed.

4.7 Insert code to spill variables. When you decide to spill v, you keep v as a
variable and must assign a register to it. But you also assign a memory
location to it. Where v is written-to you keep v, but for every read of v you
create a new fresh register v', whose physical register is allocated
independently but whose memory location matches that of v. This will force the
register allocator to see the lifetime of v as very short (and insert a store
there) and the lifetime of each v' as short too, and insert loads there.

### Register Allocation by Puzzle Solving (Pereira and Palsberg, 2008)

The benefit here is for register files that can be configured in various ways,
for example as 16 32-bit floats or eight 64-bit floats. Or the x86 AX = AH +
AL. It doesn't seem to provide value for normal register files like the kind
we'll deal with.

They transform the program into elementary programs, which are programs in SSA
form, static single information (SSI) form, and with parallel copies between
every pair of instructions that rename the variables live at that point.

SSI has pi instructions at the end of each basic block that split variables
into different variables depending on which block they're going to. It's the
inverse of phi blocks. The idea is that a branch generates information about
variables (e.g., whether it's odd), and these can be captured in new variable
names. SSI comes from Scott Anamian's master's thesis, _[The static single
information form](https://pdfs.semanticscholar.org/d68e/b3e274bfaf6c194fb2bdf44a1d64c7b89f8c.pdf)_. The original paper calls these sigma instructions.

### SSA Elimination after Register Assignment (Pereira, and Palsberg, 2009)

Assumes no critical edges and that the architecture can swap registers. If
variables in an equivalent class interfere, then a bunch of spilling must be
inserted. Instead, convert to CSSA form first (by adding variables that split
the range of variables). They define spartan parallel copies as: if cycle, then
all registers; else must be a path and only first and last are permitted to be
memory. In other words, source can only go to one destination, and the same
memory address cannot both be read and written to. They give an algorithm for
generating code for these spartan copies. I don't see where they show how to
make the copies spartan, or that CSSA will necessarily make spartan copies.

### [Register Spilling and Live-Range Splitting for SSA-Form Programs](https://sci-hub.se/https://doi.org/10.1007/978-3-642-00722-4_13) (Matthias Braun and Sebastian Hack, 2009)

Describes the Min algorithm (spills variables used furthest in the future),
extends it to the CFG (normally Min is only within a block), describes how to
add spills and reloads, and how to add phi instructions. They only seem to add
spill/reload on CFG edges, but what if a block has too high register pressure
internally?

### [Spilling](https://www.capsl.udel.edu//conferences/pemws-2/doc/Brisk.pdf) (Philip Brisk, 2009)

Lecture notes on spilling. Good overview of local (within block) and global
(within function) spilling. The third section on SSA spilling is good, look for
“SSA-based GRA” (page 27). Good bibliography at the end. Scary, says that
spilling requires reconstructing the SSA form, since a spilled variable and its
original value might have to be merged with a phi node. I don't remember seeing
that with other papers.

### [Lecture Notes on Register Allocation](https://www.cs.cmu.edu/~fp/courses/15411-f14/lectures/03-regalloc.pdf) (Frank Pfenning, Andre Platzer, 2014)

(not read)

## Notes

* Parameters to phi instructions are live at the end of the block they came
  from. We can do this by doing a full liveness analysis, then removing the
  parameters just from the phi instructions.
* Results of phi instructions are live at the top of the block where they are
  defined.
* The parameters to a phi instruction don't live past that phi instruction, so
  we can assign register in their block without knowing the assignments to
  those parameters. This explains why we can color nodes using the idom tree
  even if that means that merge blocks are done before their then/else blocks.
* Phi equivalence classes can only be an optimization. They can't solve the
  problem because even after PhiLifting you might not be able to assign all
  virtual registers in a class to the same physical register.

# Questions

* In Pereira 2009, he shows that spartan parallel copies can be efficiently
  done, but they don't show how or when we might have spartan parallel copies.
* I still don't understand how spilling is done.
* In Pereira 2007 he calculates phi equivalence classes, and spends a lot of
  time coalescing them, but then never uses them. Should they be used in
  ComputePreference? That algorithm just uses phi instructions.
* I commented out the call to computePhiClassMap() because I thought we'd do it
  differently (inserting copies when scalarizing), but in fact the copies can't
  be there since that takes it out of SSA, and the whole point is to come out
  of SSA after register assignment. In any case this is an optimization, so can
  be done later.

## Structure of compiler

* parse binary.
* post-parse:
  * find main function.
  * assign memory locations to variables.
* prepare for compile:
  * replace SPIR-V phi with ours.
  * compute successor and predecessor blocks.
  * phiLifting.
  * compute dom tree.
    * compute idom and its tree.
  * expand vectors.
  * For each function:
    * compute livein/liveout for each line.
    * if livein exceeds number of registers for an instruction:
      * spill one of the livein variables.
* compile:
  * convert SPIR-V instructions to RISC-V instructions.
    * Very simple right now, only creates Addi instructions.
  * translate out of SSA.
    * Does nothing, but could compute phi congruence classes.
  * assign registers.
  * emit instructions.

