#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

typedef std::array<float,1> v1float;
typedef std::array<uint32_t,1> v1uint;
typedef std::array<int32_t,1> v1int;
typedef std::array<float,2> v2float;
typedef std::array<uint32_t,2> v2uint;
typedef std::array<int32_t,2> v2int;
typedef std::array<float,3> v3float;
typedef std::array<uint32_t,3> v3uint;
typedef std::array<int32_t,3> v3int;
typedef std::array<float,4> v4float;
typedef std::array<uint32_t,4> v4uint;
typedef std::array<int32_t,4> v4int;

const uint32_t NO_REGISTER = 0xFFFFFFFF;
const uint32_t NO_BLOCK_ID = 0xFFFFFFFF;

// A variable in memory, either global or within a function's frame.
struct Variable
{
    // Type of variable (not the pointer to it). Key into the "types" map.
    uint32_t type;

    // One of the values of SpvStorageClass (input, output, uniform, function, etc.).
    uint32_t storageClass;

    // XXX Not sure, or NO_INITIALIZER.
    uint32_t initializer;

    // Address in memory array
    size_t address;
};

// Entry point for the shader.
struct EntryPoint
{
    // What kind of shader. Must be SpvExecutionModelFragment.
    uint32_t executionModel;

    // Name of the function to call. This is typically "main".
    std::string name;

    // XXX Don't know.
    std::vector<uint32_t> interfaceIds;

    // XXX Don't know what an execution mode is.
    std::map<uint32_t, std::vector<uint32_t>> executionModesToOperands;
};

// Information about the source code for the shader.
struct Source
{
    // What language the shader was written in.
    uint32_t language;

    // XXX Version of the shading language.
    uint32_t version;

    // XXX Dunno.
    uint32_t file;

    // XXX Filename of the shader source?
    std::string source;
};

// A decoration tells us something about a type or variable, like 
// how to access it from outside the shader (OpDecorationBinding),
// the offset of fields within a struct (OpDecorationOffset), or
// the stride of elements with in an array (OpDecorationArrayStride,
// or whether a scalar can have relaxed precision (OpDecorateRelaxedPrecision).
// See the SPIR-V spec for "OpDecorate" for more details on the
// available decorations.
//
// This type represents a list of decoration ids (e.g. OpDecorationLocation)
// mapped to the decoration's operands.  At the time of SPIR-V 1.2, all
// decorations have 0 or 1 operands except OpDecorationLinkageAttributes,
// which has 2.  So the operands vector almost always has 0 or 1 operands.
typedef std::map<uint32_t, std::vector<uint32_t> > Decorations;

// XXX Unused.
struct MemberName
{
    uint32_t type;
    uint32_t member;
    std::string name;
};

// Represents a "void" type.
struct TypeVoid
{
    // XXX Not sure why we need this. Not allowed to have an empty struct?
    int foo;
};

// Represents an "bool" type.
struct TypeBool
{
    // No fields. It's a freakin' bool.
};

// Represents an "int" type.
struct TypeInt
{
    // Number of bits in the int.
    uint32_t width;

    // Whether signed.
    uint32_t signedness;
};

// Represents a "float" type.
struct TypeFloat
{
    // Number of bits in the float.
    uint32_t width;
};

// Represents a vector type.
struct TypeVector
{
    // Type of the element. This is a key in the "types" map.
    uint32_t type;

    // Number of elements.
    uint32_t count;
};

// Represents a matrix type. Each matrix is a sequence of column vectors.
// Matrix data is stored starting at the upper-left, going down the first
// column, then the next column, etc.
struct TypeMatrix
{
    // Type of the column vector. This is a key in the "types" map.
    uint32_t columnType;

    // Number of columns.
    uint32_t columnCount;
};

// Represents a function type.
struct TypeFunction
{
    // Type of the return value. This is a key in the "types" map.
    uint32_t returnType;

    // Type of each parameter, in order from left to right. These are keys in the "types" map.
    std::vector<uint32_t> parameterTypes;
};

// Represents a struct type.
struct TypeStruct
{
    // Type of each element, in order. These are keys in the "types" map.
    std::vector<uint32_t> memberTypes;
};

// Represents a pointer type.
struct TypePointer
{
    // Type of the data being pointed to. This is a key in the "types" map.
    uint32_t type;

    // Storage class of the data. One of the values of SpvStorageClass (input,
    // output, uniform, function, etc.).
    uint32_t storageClass;
};

// Represents an image type.
struct TypeImage
{
    // XXX document these.
    uint32_t sampledType;
    uint32_t dim;
    uint32_t depth;
    uint32_t arrayed;
    uint32_t ms;
    uint32_t sampled;
    uint32_t imageFormat;
    uint32_t accessQualifier;
};

// XXX Dunno.
struct TypeSampledImage
{
    // Type of the image. This is a key in the "types" map. XXX yes?
    uint32_t imageType;
};

// Union of all known types.
typedef std::variant<TypeVoid, TypeBool, TypeFloat, TypePointer, TypeFunction, TypeVector, TypeMatrix, TypeInt, TypeStruct, TypeImage, TypeSampledImage> Type;

// A function parameter. This is on the receiving side, to set aside an SSA slot
// for the value received from the caller.
struct FunctionParameter
{
    // Type of the parameter. This is a key in the "types" map.
    uint32_t type;

    // Name of the parameter. This is a key in the "names" array.
    uint32_t id;
};

struct Function
{
    uint32_t id;
    uint32_t resultType;
    uint32_t functionControl;
    uint32_t functionType;

    // Index into the "code" array.
    uint32_t start;

    // ID of the start label. Don't jump here, this is for block dominance calculations.
    // This is NO_BLOCK_ID if it's not yet been set.
    uint32_t labelId;
};

struct Pointer
{
    uint32_t type;
    uint32_t storageClass;
    size_t address; // address in memory
};

// SSA (virtual) register.
struct Register
{
    uint32_t type;
    size_t size;
    unsigned char *data;

    Register(const Register &other)
    {
        if(false)std::cout << "move ctor allocated to " << this << " as " << &data << "\n";
        type = other.type;
        size = other.size;
        data = new unsigned char[size];
        std::copy(other.data, other.data + size, data);
    }

    Register(uint32_t type_, size_t size_) :
        type(type_),
        size(size_),
        data(new unsigned char[size_])
    {
        if(false)std::cout << "ctor allocated to " << this << " as " << &data << "\n";
    }

    Register() :
        type(0xFFFFFFFF),
        size(0),
        data(nullptr)
    {
        if(false)std::cout << "ctor empty Register " << this << " \n";
    }

    ~Register()
    {
        if(false)std::cout << "dtor deleting from " << this << " as " << &data << "\n";
        delete[] data;
    }

    Register& operator=(const Register &other)
    {
        if(false)std::cout << "operator=\n";
        if(this != &other) {
            if(false)std::cout << "op= deleting from " << this << " as " << &data << "\n";
            delete[] data;
            type = other.type;
            size = other.size;
            data = new unsigned char[size];
            if(false)std::cout << "op= allocated to " << this << " as " << &data << "\n";
            std::copy(other.data, other.data + size, data);
        }
        return *this;
    }
};

struct Interpreter;
struct Compiler;

struct Instruction {
    Instruction(uint32_t resId);
    virtual ~Instruction() {};

    // Which block this instruction is in.
    uint32_t blockId;

    // Register affected by instruction, or NO_REGISTER if no register is affected.
    uint32_t resId;

    // Set of registers that are inputs to the instruction.
    std::set<uint32_t> argIds;

    // Label IDs we might branch to.
    std::set<uint32_t> targetLabelIds;

    // Predecessor instructions. This is only empty for the first instruction in each function.
    std::set<uint32_t> pred;

    // Successor instructions.
    std::set<uint32_t> succ;

    // Registers that are live going into this instruction.
    std::set<uint32_t> livein;

    // Registers that are live leaving this instruction.
    std::set<uint32_t> liveout;

    // Step the interpreter forward one instruction.
    virtual void step(Interpreter *interpreter) = 0;

    // Emit compiler output for this instruction.
    virtual void emit(Compiler *compiler);

    // The opcode of this instruction (e.g., SpvOpFMul).
    virtual uint32_t opcode() const = 0;

    // The name of this instruction (e.g., "OpFMul").
    virtual std::string name() const = 0;

    // Whether this is a branch instruction (OpBranch, OpBranchConditional,
    // OpSwitch, OpReturn, or OpReturnValue).
    virtual bool isBranch() const { return false; }

    // Whether this is a termination instruction (branch instruction, OpKill,
    // OpUnreachable).
    virtual bool isTermination() const { return false; }
};

// A block is a sequence of instructions that has one entry point
// (the first instruction) and one exit point (the last instruction).
// The last instruction must be a variant of a branch.
struct Block {
    Block(uint32_t labelId, uint32_t begin, uint32_t end)
        : labelId(labelId), begin(begin), end(end), idom(NO_BLOCK_ID) {
        // Nothing.
    }

    // ID of label that points to first instruction.
    uint32_t labelId;

    // Index into "instructions" array of first instruction.
    uint32_t begin;

    // Index into "instructions" array of one past last instruction.
    uint32_t end;

    // Predecessor blocks. This is only empty for the first block in each function.
    std::set<uint32_t> pred;

    // Successor blocks.
    std::set<uint32_t> succ;

    // Block IDs that dominate this block.
    std::set<uint32_t> dom;

    // Immediate dominator of this block, or NO_BLOCK_ID if this is the start block.
    uint32_t idom;

    // Whether this block is dominated by the other block.
    bool isDominatedBy(uint32_t other) const {
        return dom.find(other) != dom.end();
    }
};

#endif // BASIC_TYPES_H
