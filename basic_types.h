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

// A variable in memory, either global or within a function's frame.
struct Variable
{
    // Type of variable (not the pointer to it). Key into the "types" map.
    uint32_t type;

    // One of the values of SpvStorageClass (input, output, uniform, function, etc.).
    uint32_t storageClass;

    // XXX Not sure, or NO_INITIALIZER.
    uint32_t initializer;

    // Offset into the memory for the specific storage class.
    size_t offset;
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

// XXX A decoration tells us something about a type or variable, but I'm not sure what, maybe
// how to access it from outside the shader and the offset of fields within a struct.
struct Decoration
{
    // The ID of a type or variable.
    uint32_t decoration;

    // XXX Specific operands for this decoration.
    std::vector<uint32_t> operands;
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
};

struct Pointer
{
    uint32_t type;
    uint32_t storageClass;
    size_t offset; // offset into memory, not into Region
};

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
    virtual ~Instruction() {};

    virtual void step(Interpreter *interpreter) = 0;
    virtual void emit(Compiler *compiler);
    virtual std::string name() = 0;
};

#endif // BASIC_TYPES_H
