#include <iostream>
#include <cstdio>
#include <fstream>
#include <variant>
#include <chrono>

#include <StandAlone/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.h>
#include "spirv.h"
#include "GLSL.std.450.h"

const int imageWidth = 256;
const int imageHeight = 256;
unsigned char imageBuffer[imageHeight][imageWidth][3];

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

std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

std::map<uint32_t, std::string> GLSLstd450OpcodeToString = {
#include "GLSLstd450_opcode_to_string.h"
};

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

#include "opcode_structs.h"

const uint32_t NO_MEMORY_ACCESS_SEMANTIC = 0xFFFFFFFF;

typedef std::variant<
#include "opcode_variant.h"
> Instruction;

struct Function
{
    uint32_t id;
    uint32_t resultType;
    uint32_t functionControl;
    uint32_t functionType;

    // Index into the "code" array.
    uint32_t start;
};

struct RegisterPointer
{
    uint32_t type;
    uint32_t storageClass;
    size_t offset; // offset into memory, not into Region
};

struct RegisterObject
{
    uint32_t type;
    size_t size;
    unsigned char *data;
    
    RegisterObject(const RegisterObject &other) 
    {
        if(false)std::cout << "move ctor allocated to " << this << " as " << &data << "\n";
        type = other.type;
        size = other.size;
        data = new unsigned char[size];
        std::copy(other.data, other.data + size, data);
    }

    RegisterObject(uint32_t type_, size_t size_) :
        type(type_),
        size(size_),
        data(new unsigned char[size_])
    {
        if(false)std::cout << "ctor allocated to " << this << " as " << &data << "\n";
    }

    RegisterObject() :
        type(0xFFFFFFFF),
        size(0),
        data(nullptr)
    {
        if(false)std::cout << "ctor empty RegisterObject " << this << " \n";
    }

    ~RegisterObject()
    {
        if(false)std::cout << "dtor deleting from " << this << " as " << &data << "\n";
        delete[] data;
    }

    RegisterObject& operator=(const RegisterObject &other)
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

typedef std::variant<RegisterPointer, RegisterObject> Register;

const uint32_t SOURCE_NO_FILE = 0xFFFFFFFF;
const uint32_t NO_INITIALIZER = 0xFFFFFFFF;
const uint32_t NO_ACCESS_QUALIFIER = 0xFFFFFFFF;
const uint32_t EXECUTION_ENDED = 0xFFFFFFFF;
const uint32_t NO_RETURN_REGISTER = 0xFFFFFFFF;

struct MemoryRegion
{
    size_t base;
    size_t size;
    size_t top;
    MemoryRegion() :
        base(0),
        size(0),
        top(0)
    {}
    MemoryRegion(size_t base_, size_t size_) :
        base(base_),
        size(size_),
        top(base_)
    {}
};

// helper type for visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct Interpreter 
{
    bool throwOnUnimplemented;
    bool hasUnimplemented;
    bool verbose;
    std::set<uint32_t> capabilities;

    // These values are label IDs identifying blocks within a function. The current block
    // is the block we're executing. The previous block was the block we came from.
    // These are 0xFFFFFFFF if not yet set.
    uint32_t currentBlockId;
    uint32_t previousBlockId;

    // main id-to-thingie map containing extinstsets, types, variables, etc
    // secondary maps of entryPoint, decorations, names, etc

    std::map<uint32_t, std::string> extInstSets;
    uint32_t ExtInstGLSL_std_450_id;
    std::map<uint32_t, std::string> strings;
    uint32_t memoryModel;
    uint32_t addressingModel;
    std::map<uint32_t, EntryPoint> entryPoints;
    std::map<uint32_t, Decoration> decorations;
    std::map<uint32_t, std::string> names;
    std::vector<Source> sources;
    std::vector<std::string> processes;
    std::map<uint32_t, std::map<uint32_t, std::string> > memberNames;
    std::map<uint32_t, std::map<uint32_t, Decoration> > memberDecorations;
    std::map<uint32_t, Type> types;
    std::map<uint32_t, size_t> typeSizes; // XXX put into Type
    std::map<uint32_t, Variable> variables;
    std::map<uint32_t, Function> functions;
    std::vector<Instruction> code;
    std::vector<uint32_t> blockId; // Parallel to "code".

    Function* currentFunction;
    // Map from label ID to index into code vector.
    std::map<uint32_t, uint32_t> labels;
    Function* mainFunction; 

    std::map<uint32_t, MemoryRegion> memoryRegions;
    unsigned char *memory;

    std::map<uint32_t, Register> registers;
    RegisterObject& allocRegisterObject(uint32_t id, uint32_t type)
    {
        RegisterObject r {type, typeSizes[type]};
        registers[id] = r;
        return std::get<RegisterObject>(registers[id]);
    }

    std::map<uint32_t, Register> constants;
    RegisterObject& allocConstantObject(uint32_t id, uint32_t type)
    {
        RegisterObject r {type, typeSizes[type]};
        constants[id] = r;
        return std::get<RegisterObject>(constants[id]);
    }

    Interpreter(bool throwOnUnimplemented_, bool verbose_) :
        throwOnUnimplemented(throwOnUnimplemented_),
        hasUnimplemented(false),
        verbose(verbose_),
        currentFunction(nullptr),
        currentBlockId(0xFFFFFFFF),
        previousBlockId(0xFFFFFFFF)
    {
        size_t base = 0;
        auto anotherRegion = [&base](size_t size){MemoryRegion r(base, size); base += size; return r;};
        memoryRegions[SpvStorageClassUniformConstant] = anotherRegion(1024);
        memoryRegions[SpvStorageClassInput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassUniform] = anotherRegion(1024);
        memoryRegions[SpvStorageClassOutput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassWorkgroup] = anotherRegion(0);
        memoryRegions[SpvStorageClassCrossWorkgroup] = anotherRegion(0);
            // I'll put intermediates in "Private" storage:
        memoryRegions[SpvStorageClassPrivate] = anotherRegion(65536);
        memoryRegions[SpvStorageClassFunction] = anotherRegion(16384);
        memoryRegions[SpvStorageClassGeneric] = anotherRegion(0);
        memoryRegions[SpvStorageClassPushConstant] = anotherRegion(0);
        memoryRegions[SpvStorageClassAtomicCounter] = anotherRegion(0);
        memoryRegions[SpvStorageClassImage] = anotherRegion(0);
        memoryRegions[SpvStorageClassStorageBuffer] = anotherRegion(0);
        memory = new unsigned char[base];
    }

    ~Interpreter()
    {
        delete[] memory;
    }

    size_t allocate(SpvStorageClass clss, uint32_t type)
    {
        MemoryRegion& reg = memoryRegions[clss];
        if(false) {
            std::cout << "allocate from " << clss << " type " << type << "\n";
            std::cout << "region at " << reg.base << " size " << reg.size << " top " << reg.top << "\n";
            std::cout << "object is size " << typeSizes[type] << "\n";
        }
        assert(reg.top + typeSizes[type] <= reg.base + reg.size);
        size_t offset = reg.top;
        reg.top += typeSizes[type];
        return offset;
    }
    size_t allocate(uint32_t clss, uint32_t type)
    {
        return allocate(static_cast<SpvStorageClass>(clss), type);
    }

    void copy(uint32_t type, size_t src, size_t dst)
    {
        std::copy(memory + src, memory + src + typeSizes[type], memory + dst);
    }

    template <class T>
    T& objectInClassAt(SpvStorageClass clss, size_t offset)
    {
        return *reinterpret_cast<T*>(memory + memoryRegions[clss].base + offset);
    }

    template <class T>
    T& objectAt(size_t offset)
    {
        return *reinterpret_cast<T*>(memory + offset);
    }

    template <class T>
    T& objectAt(unsigned char* data)
    {
        return *reinterpret_cast<T*>(data);
    }

    template <class T>
    T& registerAs(int id)
    {
        return *reinterpret_cast<T*>(std::get<RegisterObject>(registers[id]).data);
    }

    // Returns the type of the member of "t" at index "i". For vectors,
    // "i" is ignored and the type of any element is returned. For matrices,
    // "i" is ignored and the type of any column is returned. For structs,
    // the type of field "i" (0-indexed) is returned.
    uint32_t getConstituentType(uint32_t t, int i)
    {
        const Type& type = types[t];
        if(std::holds_alternative<TypeVector>(type)) {
            return std::get<TypeVector>(type).type;
        } else if(std::holds_alternative<TypeMatrix>(type)) {
            return std::get<TypeMatrix>(type).columnType;
        } else if (std::holds_alternative<TypeStruct>(type)) {
            return std::get<TypeStruct>(type).memberTypes[i];
        // XXX } else if (std::holds_alternative<TypeArray>(type)) {
        } else {
            std::cout << type.index() << "\n";
            assert(false && "getConstituentType of invalid type?!");
        }
        return 0; // not reached
    }

    void dumpTypeAt(const Type& type, unsigned char *ptr)
    {
        std::visit(overloaded {
            [&](const TypeVoid& type) { std::cout << "{}"; },
            [&](const TypeBool& type) { std::cout << objectAt<bool>(ptr); },
            [&](const TypeFloat& type) { std::cout << objectAt<float>(ptr); },
            [&](const TypeInt& type) {
                if(type.signedness) {
                    std::cout << objectAt<int32_t>(ptr);
                } else {
                    std::cout << objectAt<uint32_t>(ptr);
                }
            },
            [&](const TypePointer& type) { std::cout << "(ptr)" << objectAt<uint32_t>(ptr); },
            [&](const TypeFunction& type) { std::cout << "function"; },
            [&](const TypeImage& type) { std::cout << "image"; },
            [&](const TypeSampledImage& type) { std::cout << "sampledimage"; },
            [&](const TypeVector& type) {
                std::cout << "<";
                for(int i = 0; i < type.count; i++) {
                    dumpTypeAt(types[type.type], ptr);
                    ptr += typeSizes[type.type];
                    if(i < type.count - 1)
                        std::cout << ", ";
                }
                std::cout << ">";
            },
            [&](const TypeMatrix& type) {
                std::cout << "<";
                for(int i = 0; i < type.columnCount; i++) {
                    dumpTypeAt(types[type.columnType], ptr);
                    ptr += typeSizes[type.columnType];
                    if(i < type.columnCount - 1)
                        std::cout << ", ";
                }
                std::cout << ">";
            },
            [&](const TypeStruct& type) {
                std::cout << "{";
                for(int i = 0; i < type.memberTypes.size(); i++) {
                    dumpTypeAt(types[type.memberTypes[i]], ptr);
                    ptr += typeSizes[type.memberTypes[i]];
                    if(i < type.memberTypes.size() - 1)
                        std::cout << ", ";
                }
                std::cout << "}";
            },
        }, type);
    }

    static spv_result_t handleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema)
    {
        // auto ip = static_cast<Interpreter*>(user_data);
        return SPV_SUCCESS;
    }

    static spv_result_t handleInstruction(void* user_data, const spv_parsed_instruction_t* insn)
    {
        auto ip = static_cast<Interpreter*>(user_data);

        auto opds = insn->operands;

        int which = 0;

        // Read the next uint32_t.
        auto nextu = [insn, opds, &which](uint32_t deflt = 0xFFFFFFFF) {
            return (which < insn->num_operands) ? insn->words[opds[which++].offset] : deflt;
        };

        // Read the next string.
        auto nexts = [insn, opds, &which]() {
            const char *s = reinterpret_cast<const char *>(&insn->words[opds[which].offset]);
            which++;
            return s;
        };

        // Read the next vector.
        auto nextv = [insn, opds, &which]() {
            std::vector<uint32_t> v;
            if(which < insn->num_operands) {
                v = std::vector<uint32_t> {&insn->words[opds[which].offset], &insn->words[opds[which].offset] + opds[which].num_words};
            } else {
                v = {};
            }
            which++; // XXX advance by opds[which].num_words? And move up into the "if"?
            return v;
        };

        // Read the rest of the operands as uint32_t.
        auto restv = [insn, opds, &which]() {
            std::vector<uint32_t> v;
            while(which < insn->num_operands) {
                v.push_back(insn->words[opds[which++].offset]);
            }
            return v;
        };

        switch(insn->opcode) {

            case SpvOpCapability: {
                uint32_t cap = nextu();
                assert(cap == SpvCapabilityShader);
                ip->capabilities.insert(cap);
                if(ip->verbose) {
                    std::cout << "OpCapability " << cap << " \n";
                }
                break;
            }

            case SpvOpExtInstImport: {
                // XXX result id
                uint32_t id = nextu();
                const char *name = nexts();
                if(strcmp(name, "GLSL.std.450") == 0) {
                    ip->ExtInstGLSL_std_450_id = id;
                } else {
                    throw std::runtime_error("unimplemented extension instruction set \"" + std::string(name) + "\"");
                }
                ip->extInstSets[id] = name;
                if(ip->verbose) {
                    std::cout << "OpExtInstImport " << insn->words[opds[0].offset] << " " << name << "\n";
                }
                break;
            }

            case SpvOpMemoryModel: {
                ip->addressingModel = nextu();
                ip->memoryModel = nextu();
                assert(ip->addressingModel == SpvAddressingModelLogical);
                assert(ip->memoryModel == SpvMemoryModelGLSL450);
                if(ip->verbose) {
                    std::cout << "OpMemoryModel " << ip->addressingModel << " " << ip->memoryModel << "\n";
                }
                break;
            }

            case SpvOpEntryPoint: {
                // XXX not result id but id must eventually be Function result id
                uint32_t executionModel = nextu();
                uint32_t id = nextu();
                std::string name = nexts();
                std::vector<uint32_t> interfaceIds = restv();
                assert(executionModel == SpvExecutionModelFragment);
                ip->entryPoints[id] = {executionModel, name, interfaceIds};
                if(ip->verbose) {
                    std::cout << "OpEntryPoint " << executionModel << " " << id << " " << name;
                    for(auto& i: interfaceIds)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpExecutionMode: {
                uint32_t entryPointId = nextu();
                uint32_t executionMode = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->entryPoints[entryPointId].executionModesToOperands[executionMode] = operands;

                if(ip->verbose) {
                    std::cout << "OpExecutionMode " << entryPointId << " " << executionMode;
                    for(auto& i: ip->entryPoints[entryPointId].executionModesToOperands[executionMode])
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpString: {
                // XXX result id
                uint32_t id = nextu();
                std::string name = nexts();
                ip->strings[id] = name;
                if(ip->verbose) {
                    std::cout << "OpString " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpName: {
                uint32_t id = nextu();
                std::string name = nexts();
                ip->names[id] = name;
                if(ip->verbose) {
                    std::cout << "OpName " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpSource: {
                uint32_t language = nextu();
                uint32_t version = nextu();
                uint32_t file = nextu(SOURCE_NO_FILE);
                std::string source = (insn->num_operands > 3) ? nexts() : "";
                ip->sources.push_back({language, version, file, source});
                if(ip->verbose) {
                    std::cout << "OpSource " << language << " " << version << " " << file << " " << ((source.size() > 0) ? "with source" : "without source") << "\n";
                }
                break;
            }

            case SpvOpMemberName: {
                uint32_t type = nextu();
                uint32_t member = nextu();
                std::string name = nexts();
                ip->memberNames[type][member] = name;
                if(ip->verbose) {
                    std::cout << "OpMemberName " << type << " " << member << " " << name << "\n";
                }
                break;
            }

            case SpvOpModuleProcessed: {
                std::string process = nexts();
                ip->processes.push_back(process);
                if(ip->verbose) {
                    std::cout << "OpModulesProcessed " << process << "\n";
                }
                break;
            }

            case SpvOpDecorate: {
                uint32_t id = nextu();
                uint32_t decoration = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->decorations[id] = {decoration, operands};
                if(ip->verbose) {
                    std::cout << "OpDecorate " << id << " " << decoration;
                    for(auto& i: ip->decorations[id].operands)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpMemberDecorate: {
                uint32_t id = nextu();
                uint32_t member = nextu();
                uint32_t decoration = nextu();
                std::vector<uint32_t> operands = nextv();
                ip->memberDecorations[id][member] = {decoration, operands};
                if(ip->verbose) {
                    std::cout << "OpMemberDecorate " << id << " " << member << " " << decoration;
                    for(auto& i: ip->memberDecorations[id][member].operands)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpTypeVoid: {
                // XXX result id
                uint32_t id = nextu();
                ip->types[id] = TypeVoid {};
                ip->typeSizes[id] = 0;
                if(ip->verbose) {
                    std::cout << "TypeVoid " << id << "\n";
                }
                break;
            }

            case SpvOpTypeBool: {
                // XXX result id
                uint32_t id = nextu();
                ip->types[id] = TypeBool {};
                ip->typeSizes[id] = sizeof(bool);
                if(ip->verbose) {
                    std::cout << "TypeBool " << id << "\n";
                }
                break;
            }

            case SpvOpTypeFloat: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t width = nextu();
                assert(width <= 32); // XXX deal with larger later
                ip->types[id] = TypeFloat {width};
                ip->typeSizes[id] = ((width + 31) / 32) * 4;
                if(ip->verbose) {
                    std::cout << "TypeFloat " << id << " " << width << "\n";
                }
                break;
            }

            case SpvOpTypeInt: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t width = nextu();
                uint32_t signedness = nextu();
                assert(width <= 32); // XXX deal with larger later
                ip->types[id] = TypeInt {width, signedness};
                ip->typeSizes[id] = ((width + 31) / 32) * 4;
                if(ip->verbose) {
                    std::cout << "TypeInt " << id << " width " << width << " signedness " << signedness << "\n";
                }
                break;
            }

            case SpvOpTypeFunction: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t returnType = nextu();
                std::vector<uint32_t> params = restv();
                ip->types[id] = TypeFunction {returnType, params};
                ip->typeSizes[id] = 4; // XXX ?!?!?
                if(ip->verbose) {
                    std::cout << "TypeFunction " << id << " returning " << returnType;
                    if(params.size() > 1) {
                        std::cout << " with parameter types"; 
                        for(int i = 0; i < params.size(); i++)
                            std::cout << " " << params[i];
                    }
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpTypeVector: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t type = nextu();
                uint32_t count = nextu();
                ip->types[id] = TypeVector {type, count};
                ip->typeSizes[id] = ip->typeSizes[type] * count;
                if(ip->verbose) {
                    std::cout << "TypeVector " << id << " of " << type << " count " << count << "\n";
                }
                break;
            }

            case SpvOpTypeMatrix: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t columnType = nextu();
                uint32_t columnCount = nextu();
                ip->types[id] = TypeMatrix {columnType, columnCount};
                ip->typeSizes[id] = ip->typeSizes[columnType] * columnCount;
                if(ip->verbose) {
                    std::cout << "TypeMatrix " << id << " of " << columnType << " count " << columnCount << "\n";
                }
                break;
            }

            case SpvOpTypePointer: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t storageClass = nextu();
                uint32_t type = nextu();
                ip->types[id] = TypePointer {type, storageClass};
                ip->typeSizes[id] = sizeof(uint32_t);
                if(ip->verbose) {
                    std::cout << "TypePointer " << id << " class " << storageClass << " type " << type << "\n";
                }
                break;
            }

            case SpvOpTypeStruct: {
                // XXX result id
                uint32_t id = nextu();
                std::vector<uint32_t> memberTypes = restv();
                ip->types[id] = TypeStruct {memberTypes};
                size_t size = 0;
                for(auto& t: memberTypes) {
                    size += ip->typeSizes[t];
                }
                ip->typeSizes[id] = size;
                if(ip->verbose) {
                    std::cout << "TypeStruct " << id;
                    if(memberTypes.size() > 0) {
                        std::cout << " members"; 
                        for(auto& i: memberTypes)
                            std::cout << " " << i;
                    }
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpVariable: {
                // XXX result id
                uint32_t type = nextu();
                uint32_t id = nextu();
                uint32_t storageClass = nextu();
                uint32_t initializer = nextu(NO_INITIALIZER);
                uint32_t pointedType = std::get<TypePointer>(ip->types[type]).type;
                size_t offset = ip->allocate(storageClass, pointedType);
                ip->variables[id] = {pointedType, storageClass, initializer, offset};
                if(ip->verbose) {
                    std::cout << "Variable " << id << " type " << type << " to type " << pointedType << " storageClass " << storageClass << " offset " << offset;
                    if(initializer != NO_INITIALIZER)
                        std::cout << " initializer " << initializer;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpConstant: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                assert(opds[2].num_words == 1); // XXX limit to 32 bits for now
                uint32_t value = nextu();
                RegisterObject& r = ip->allocConstantObject(id, typeId);
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + ip->typeSizes[typeId], r.data);
                if(ip->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantTrue: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                RegisterObject& r = ip->allocConstantObject(id, typeId);
                bool value = true;
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + ip->typeSizes[typeId], r.data);
                if(ip->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantFalse: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                RegisterObject& r = ip->allocConstantObject(id, typeId);
                bool value = false;
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + ip->typeSizes[typeId], r.data);
                if(ip->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantComposite: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                std::vector<uint32_t> operands = restv();
                RegisterObject& r = ip->allocConstantObject(id, typeId);
                uint32_t offset = 0;
                for(uint32_t operand : operands) {
                    // Copy each operand from a constant into our new composite constant.
                    const RegisterObject &src = std::get<RegisterObject>(ip->constants[operand]);
                    uint32_t size = ip->typeSizes[src.type];
                    std::copy(src.data, src.data + size, r.data + offset);
                    offset += size;
                }
                assert(offset = ip->typeSizes[typeId]);
                break;
            }

            case SpvOpTypeSampledImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t imageType = nextu();
                ip->types[id] = TypeSampledImage { imageType };
                if(ip->verbose) {
                    std::cout << "TypeSampledImage " << id
                        << " imageType " << imageType
                        << "\n";
                }
                break;
            }

            case SpvOpTypeImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t sampledType = nextu();
                uint32_t dim = nextu();
                uint32_t depth = nextu();
                uint32_t arrayed = nextu();
                uint32_t ms = nextu();
                uint32_t sampled = nextu();
                uint32_t imageFormat = nextu();
                uint32_t accessQualifier = nextu(NO_ACCESS_QUALIFIER);
                ip->types[id] = TypeImage { sampledType, dim, depth, arrayed, ms, sampled, imageFormat, accessQualifier };
                if(ip->verbose) {
                    std::cout << "TypeImage " << id
                        << " sampledType " << sampledType
                        << " dim " << dim
                        << " depth " << depth
                        << " arrayed " << arrayed
                        << " ms " << ms
                        << " sampled " << sampled
                        << " imageFormat " << imageFormat
                        << " accessQualifier " << accessQualifier
                        << "\n";
                }
                break;
            }

            case SpvOpFunction: {
                uint32_t resultType = nextu();
                uint32_t id = nextu();
                uint32_t functionControl = nextu();
                uint32_t functionType = nextu();
                uint32_t start = ip->code.size();
                ip->functions[id] = Function {id, resultType, functionControl, functionType, start };
                ip->currentFunction = &ip->functions[id];
                if(ip->verbose) {
                    std::cout << "Function " << id
                        << " resultType " << resultType
                        << " functionControl " << functionControl
                        << " functionType " << functionType
                        << "\n";
                }
                break;
            }

            case SpvOpLabel: {
                uint32_t id = nextu();
                ip->labels[id] = ip->code.size();
                if(ip->verbose) {
                    std::cout << "Label " << id
                        << " at " << ip->labels[id]
                        << "\n";
                }
                break;
            }

            case SpvOpFunctionEnd: {
                ip->currentFunction = NULL;
                if(ip->verbose) {
                    std::cout << "FunctionEnd\n";
                }
                break;
            }

            case SpvOpSelectionMerge: {
                // We don't do anything with this information.
                // https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#StructuredControlFlow
                break;
            }

            case SpvOpLoopMerge: {
                // We don't do anything with this information.
                // https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#StructuredControlFlow
                break;
            }

            // Decode the instructions.
#include "opcode_decode.h"
            
            default: {
                if(ip->throwOnUnimplemented) {
                    throw std::runtime_error("unimplemented opcode " + OpcodeToString[insn->opcode] + " (" + std::to_string(insn->opcode) + ")");
                } else {
                    std::cout << "unimplemented opcode " << OpcodeToString[insn->opcode] << " (" << insn->opcode << ")\n";
                    ip->hasUnimplemented = true;
                }
                break;
            }
        }

        return SPV_SUCCESS;
    }

    void prepare()
    {
        mainFunction = nullptr;
        for(auto& e: entryPoints) {
            if(e.second.name == "main") {
                mainFunction = &functions[e.first];
            }
        }
    }

    uint32_t pc;
    std::vector<size_t> callstack;

    void stepLoad(const InsnLoad& insn)
    {
        RegisterPointer& ptr = std::get<RegisterPointer>(registers[insn.pointerId]);
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::copy(memory + ptr.offset, memory + ptr.offset + typeSizes[insn.type], obj.data);
        if(false) {
            std::cout << "load result is";
            dumpTypeAt(types[insn.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepStore(const InsnStore& insn)
    {
        RegisterPointer& ptr = std::get<RegisterPointer>(registers[insn.pointerId]);
        RegisterObject& obj = std::get<RegisterObject>(registers[insn.objectId]);
        std::copy(obj.data, obj.data + obj.size, memory + ptr.offset);
    }

    void stepCompositeExtract(const InsnCompositeExtract& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        RegisterObject& src = std::get<RegisterObject>(registers[insn.compositeId]);
        /* use indexes to walk blob */
        uint32_t type = src.type;
        size_t offset = 0;
        for(auto& j: insn.indexesId) {
            for(int i = 0; i < j; i++) {
                offset += typeSizes[getConstituentType(type, i)];
            }
            type = getConstituentType(type, j);
        }
        std::copy(src.data + offset, src.data + offset + typeSizes[obj.type], obj.data);
        if(false) {
            std::cout << "extracted from ";
            dumpTypeAt(types[src.type], src.data);
            std::cout << " result is ";
            dumpTypeAt(types[insn.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepCompositeConstruct(const InsnCompositeConstruct& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        size_t offset = 0;
        for(auto& j: insn.constituentsId) {
            RegisterObject& src = std::get<RegisterObject>(registers[j]);
            std::copy(src.data, src.data + typeSizes[src.type], obj.data + offset);
            offset += typeSizes[src.type];
        }
        if(false) {
            std::cout << "constructed ";
            dumpTypeAt(types[obj.type], obj.data);
            std::cout << "\n";
        }
    }

    void stepIAdd(const InsnIAdd& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeInt>) {

                uint32_t operand1 = registerAs<uint32_t>(insn.operand1Id);
                uint32_t operand2 = registerAs<uint32_t>(insn.operand2Id);
                uint32_t result = operand1 + operand2;
                registerAs<uint32_t>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                uint32_t* operand1 = &registerAs<uint32_t>(insn.operand1Id);
                uint32_t* operand2 = &registerAs<uint32_t>(insn.operand2Id);
                uint32_t* result = &registerAs<uint32_t>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] + operand2[i];
                }

            } else {

                std::cout << "Unknown type for IAdd\n";

            }
        }, types[insn.type]);
    }

    void stepFAdd(const InsnFAdd& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float result = operand1 + operand2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] + operand2[i];
                }

            } else {

                std::cout << "Unknown type for FAdd\n";

            }
        }, types[insn.type]);
    }

    void stepFSub(const InsnFSub& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float result = operand1 - operand2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] - operand2[i];
                }

            } else {

                std::cout << "Unknown type for FSub\n";

            }
        }, types[insn.type]);
    }

    void stepFMul(const InsnFMul& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float result = operand1 * operand2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] * operand2[i];
                }

            } else {

                std::cout << "Unknown type for FMul\n";

            }
        }, types[insn.type]);
    }

    void stepFDiv(const InsnFDiv& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float result = operand1 / operand2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] / operand2[i];
                }

            } else {

                std::cout << "Unknown type for FDiv\n";

            }
        }, types[insn.type]);
    }

    void stepFMod(const InsnFMod& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                float result = operand1 - floor(operand1/operand2)*operand2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] - floor(operand1[i]/operand2[i])*operand2[i];
                }

            } else {

                std::cout << "Unknown type for FMod\n";

            }
        }, types[insn.type]);
    }

    void stepFOrdLessThan(const InsnFOrdLessThan& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                bool result = operand1 < operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] < operand2[i];
                }

            } else {

                std::cout << "Unknown type for FOrdLessThan\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepFOrdGreaterThan(const InsnFOrdGreaterThan& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                bool result = operand1 > operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] > operand2[i];
                }

            } else {

                std::cout << "Unknown type for FOrdGreaterThan\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepFOrdLessThanEqual(const InsnFOrdLessThanEqual& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                bool result = operand1 <= operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] <= operand2[i];
                }

            } else {

                std::cout << "Unknown type for FOrdLessThanEqual\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepFOrdEqual(const InsnFOrdEqual& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                // XXX I don't know the difference between ordered and equal
                // vs. unordered and equal, so I don't know which this is.
                bool result = operand1 == operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] == operand2[i];
                }

            } else {

                std::cout << "Unknown type for FOrdEqual\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepFNegate(const InsnFNegate& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                registerAs<float>(insn.resultId) = -registerAs<float>(insn.operandId);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand = &registerAs<float>(insn.operandId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = -operand[i];
                }

            } else {

                // Doesn't seem necessary to do matrices, the assembly
                // extracts the vectors and negates them and contructs
                // a new matrix.

                std::cout << "Unknown type for FNegate\n";

            }
        }, types[insn.type]);
    }

    void stepDot(const InsnDot& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            const RegisterObject &r1 = std::get<RegisterObject>(registers[insn.vector1Id]);
            const TypeVector &t1 = std::get<TypeVector>(types[r1.type]);

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float* vector1 = &registerAs<float>(insn.vector1Id);
                float* vector2 = &registerAs<float>(insn.vector2Id);
                float sum = 0;
                for(int i = 0; i < t1.count; i++) {
                    sum += vector1[i] * vector2[i];
                }
                registerAs<float>(insn.resultId) = sum;

            } else {

                std::cout << "Unknown type for Dot\n";

            }
        }, types[insn.type]);
    }

    void stepFOrdGreaterThanEqual(const InsnFOrdGreaterThanEqual& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float operand1 = registerAs<float>(insn.operand1Id);
                float operand2 = registerAs<float>(insn.operand2Id);
                bool result = operand1 >= operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* operand1 = &registerAs<float>(insn.operand1Id);
                float* operand2 = &registerAs<float>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] >= operand2[i];
                }

            } else {

                std::cout << "Unknown type for FOrdGreaterThanEqual\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepSLessThan(const InsnSLessThan& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeInt>) {

                int32_t operand1 = registerAs<int32_t>(insn.operand1Id);
                int32_t operand2 = registerAs<int32_t>(insn.operand2Id);
                bool result = operand1 < operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                int32_t* operand1 = &registerAs<int32_t>(insn.operand1Id);
                int32_t* operand2 = &registerAs<int32_t>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] < operand2[i];
                }

            } else {

                std::cout << "Unknown type for SLessThan\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepIEqual(const InsnIEqual& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeInt>) {

                uint32_t operand1 = registerAs<uint32_t>(insn.operand1Id);
                uint32_t operand2 = registerAs<uint32_t>(insn.operand2Id);
                bool result = operand1 == operand2;
                registerAs<bool>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                uint32_t* operand1 = &registerAs<uint32_t>(insn.operand1Id);
                uint32_t* operand2 = &registerAs<uint32_t>(insn.operand2Id);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = operand1[i] == operand2[i];
                }

            } else {

                std::cout << "Unknown type for IEqual\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operand1Id]).type]);
    }

    void stepLogicalNot(const InsnLogicalNot& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeBool>) {

                registerAs<bool>(insn.resultId) = !registerAs<bool>(insn.operandId);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                bool* operand = &registerAs<bool>(insn.operandId);
                bool* result = &registerAs<bool>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = !operand[i];
                }

            } else {

                std::cout << "Unknown type for LogicalNot\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.operandId]).type]);
    }

    void stepSelect(const InsnSelect& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                bool condition = registerAs<bool>(insn.conditionId);
                float object1 = registerAs<float>(insn.object1Id);
                float object2 = registerAs<float>(insn.object2Id);
                float result = condition ? object1 : object2;
                registerAs<float>(insn.resultId) = result;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                bool* condition = &registerAs<bool>(insn.conditionId);
                // XXX shouldn't assume floats here. Any data is valid.
                float* object1 = &registerAs<float>(insn.object1Id);
                float* object2 = &registerAs<float>(insn.object2Id);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = condition[i] ? object1[i] : object2[i];
                }

            } else {

                std::cout << "Unknown type for stepSelect\n";

            }
        }, types[insn.type]);
    }

    void stepVectorTimesScalar(const InsnVectorTimesScalar& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);

        // XXX assumes floats.
        float* vector = &registerAs<float>(insn.vectorId);
        float scalar = registerAs<float>(insn.scalarId);
        float* result = &registerAs<float>(insn.resultId);

        const TypeVector &type = std::get<TypeVector>(types[insn.type]);

        for(int i = 0; i < type.count; i++) {
            result[i] = vector[i] * scalar;
        }
    }

    void stepVectorShuffle(const InsnVectorShuffle& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        const RegisterObject &r1 = std::get<RegisterObject>(registers[insn.vector1Id]);
        const RegisterObject &r2 = std::get<RegisterObject>(registers[insn.vector2Id]);
        const TypeVector &t1 = std::get<TypeVector>(types[r1.type]);
        uint32_t n1 = t1.count;
        uint32_t elementSize = typeSizes[t1.type];

        for(int i = 0; i < insn.componentsId.size(); i++) {
            uint32_t component = insn.componentsId[i];
            unsigned char *src = component < n1
                ? r1.data + component*elementSize
                : r2.data + (component - n1)*elementSize;
            std::copy(src, src + elementSize, obj.data + i*elementSize);
        }
    }

    void stepConvertSToF(const InsnConvertSToF& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                int32_t src = registerAs<int32_t>(insn.signedValueId);
                registerAs<float>(insn.resultId) = src;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                int32_t* src = &registerAs<int32_t>(insn.signedValueId);
                float* dst = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    dst[i] = src[i];
                }

            } else {

                std::cout << "Unknown type for ConvertSToF\n";

            }
        }, types[insn.type]);
    }

    void stepConvertFToS(const InsnConvertFToS& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeInt>) {

                float src = registerAs<float>(insn.floatValueId);
                registerAs<uint32_t>(insn.resultId) = src;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* src = &registerAs<float>(insn.floatValueId);
                uint32_t* dst = &registerAs<uint32_t>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    dst[i] = src[i];
                }

            } else {

                std::cout << "Unknown type for ConvertFToS\n";

            }
        }, types[insn.type]);
    }

    void stepAccessChain(const InsnAccessChain& insn)
    {
        RegisterPointer& basePointer = std::get<RegisterPointer>(registers[insn.baseId]);
        uint32_t type = basePointer.type;
        size_t offset = basePointer.offset;
        for(auto& id: insn.indexesId) {
            int32_t j = registerAs<int32_t>(id);
            for(int i = 0; i < j; i++) {
                offset += typeSizes[getConstituentType(type, i)];
            }
            type = getConstituentType(type, j);
        }
        if(false) {
            std::cout << "accesschain of " << basePointer.offset << " yielded " << offset << "\n";
        }
        uint32_t pointedType = std::get<TypePointer>(types[insn.type]).type;
        registers[insn.resultId] = RegisterPointer { pointedType, basePointer.storageClass, offset };
    }

    void stepFunctionParameter(const InsnFunctionParameter& insn)
    {
        uint32_t sourceId = callstack.back(); callstack.pop_back();
        registers[insn.resultId] = registers[sourceId];
        if(false) std::cout << "function parameter " << insn.resultId << " receives " << sourceId << "\n";
    }

    void stepReturn(const InsnReturn& insn)
    {
        callstack.pop_back(); // return parameter not used.
        pc = callstack.back(); callstack.pop_back();
    }

    void stepReturnValue(const InsnReturnValue& insn)
    {
        // Return value.
        uint32_t returnId = callstack.back(); callstack.pop_back();
        registers[returnId] = registers[insn.valueId];

        pc = callstack.back(); callstack.pop_back();
    }

    void stepFunctionCall(const InsnFunctionCall& insn)
    {
        const Function& function = functions[insn.functionId];

        callstack.push_back(pc);
        callstack.push_back(insn.resultId);
        for(int i = insn.operandId.size() - 1; i >= 0; i--) {
            uint32_t argument = insn.operandId[i];
            assert(std::holds_alternative<RegisterPointer>(registers[argument]));
            callstack.push_back(argument);
        }
        pc = function.start;
    }

    void stepGLSLstd450Distance(const InsnGLSLstd450Distance& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float p0 = registerAs<float>(insn.p0Id);
                float p1 = registerAs<float>(insn.p1Id);
                float radicand = (p1 - p0) * (p1 - p0);
                registerAs<float>(insn.resultId) = sqrtf(radicand);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* p0 = &registerAs<float>(insn.p0Id);
                float* p1 = &registerAs<float>(insn.p1Id);
                float radicand = 0;
                for(int i = 0; i < type.count; i++) {
                    radicand += (p1[i] - p0[i]) * (p1[i] - p0[i]);
                }
                registerAs<float>(insn.resultId) = sqrtf(radicand);

            } else {

                std::cout << "Unknown type for Distance\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.p0Id]).type]);
    }

    void stepGLSLstd450Length(const InsnGLSLstd450Length& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = fabsf(x);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float length = 0;
                for(int i = 0; i < type.count; i++) {
                    length += x[i]*x[i];
                }
                registerAs<float>(insn.resultId) = sqrtf(length);

            } else {

                std::cout << "Unknown type for Length\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450FMax(const InsnGLSLstd450FMax& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                float y = registerAs<float>(insn.yId);
                registerAs<float>(insn.resultId) = x < y ? y : x;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* y = &registerAs<float>(insn.yId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = x[i] < y[i] ? y[i] : x[i];
                }

            } else {

                std::cout << "Unknown type for FMax\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450FMin(const InsnGLSLstd450FMin& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                float y = registerAs<float>(insn.yId);
                registerAs<float>(insn.resultId) = x > y ? y : x;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* y = &registerAs<float>(insn.yId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = x[i] > y[i] ? y[i] : x[i];
                }

            } else {

                std::cout << "Unknown type for FMin\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450Normalize(const InsnGLSLstd450Normalize& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = x < 0 ? -1 : 1;

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float length = 0;
                for(int i = 0; i < type.count; i++) {
                    length += x[i]*x[i];
                }
                length = sqrtf(length);

                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = length == 0 ? 0 : x[i]/length;
                }

            } else {

                std::cout << "Unknown type for Normalize\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450Sin(const InsnGLSLstd450Sin& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = sin(x);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = sin(x[i]);
                }

            } else {

                std::cout << "Unknown type for Sin\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450Cos(const InsnGLSLstd450Cos& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = cos(x);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = cos(x[i]);
                }

            } else {

                std::cout << "Unknown type for Cos\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450FAbs(const InsnGLSLstd450FAbs& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = fabsf(x);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = fabsf(x[i]);
                }

            } else {

                std::cout << "Unknown type for FAbs\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450Floor(const InsnGLSLstd450Floor& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeFloat>) {

                float x = registerAs<float>(insn.xId);
                registerAs<float>(insn.resultId) = floor(x);

            } else if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* result = &registerAs<float>(insn.resultId);
                for(int i = 0; i < type.count; i++) {
                    result[i] = floor(x[i]);
                }

            } else {

                std::cout << "Unknown type for Floor\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepGLSLstd450Cross(const InsnGLSLstd450Cross& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        std::visit([this, &insn](auto&& type) {

            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, TypeVector>) {

                float* x = &registerAs<float>(insn.xId);
                float* y = &registerAs<float>(insn.yId);
                float* result = &registerAs<float>(insn.resultId);

                assert(type.count == 3);

                result[0] = x[1]*y[2] - y[1]*x[2];
                result[1] = x[2]*y[0] - y[2]*x[0];
                result[2] = x[0]*y[1] - y[0]*x[1];

            } else {

                std::cout << "Unknown type for Cross\n";

            }
        }, types[std::get<RegisterObject>(registers[insn.xId]).type]);
    }

    void stepBranch(const InsnBranch& insn)
    {
        pc = labels[insn.targetLabelId];
    }

    void stepBranchConditional(const InsnBranchConditional& insn)
    {
        bool condition = registerAs<bool>(insn.conditionId);
        pc = labels[condition ? insn.trueLabelId : insn.falseLabelId];
    }

    void stepPhi(const InsnPhi& insn)
    {
        RegisterObject& obj = allocRegisterObject(insn.resultId, insn.type);
        uint32_t size = typeSizes[obj.type];

        bool found = false;
        for(int i = 0; !found && i < insn.operandId.size(); i += 2) {
            uint32_t srcId = insn.operandId[i];
            uint32_t parentId = insn.operandId[i + 1];

            if (parentId == previousBlockId) {
                const RegisterObject &src = std::get<RegisterObject>(registers[srcId]);
                std::copy(src.data, src.data + size, obj.data);
                found = true;
            }
        }

        if (!found) {
            std::cout << "Error: Phi didn't find any label " << previousBlockId << "\n";
        }
    }

    // Unimplemented instructions. To implement one, move the function from this
    // header into this file just above this comment.
#include "opcode_impl.h"

    void step()
    {
        if(false) std::cout << "address " << pc << "\n";

        // Update our idea of what block we're in. If we just switched blocks,
        // remember the previous one (for Phi).
        if (blockId[pc] != currentBlockId) {
            // I'm not sure this is fully correct. For example, when returning
            // from a function this will set previousBlockId to a block in
            // the called function, but that's not right, it should always
            // point to a block within the current function. I don't think that
            // matters in practice because of the restricted locations where Phi
            // is placed.
            previousBlockId = currentBlockId;
            currentBlockId = blockId[pc];
        }

        Instruction insn = code[pc++];

        std::visit(overloaded {
#include "opcode_dispatch.h"
        }, insn);
    }

    template <class T>
    void set(SpvStorageClass clss, size_t offset, const T& v)
    {
        objectInClassAt<T>(clss, offset) = v;
    }

    template <class T>
    void get(SpvStorageClass clss, size_t offset, T& v)
    {
        v = objectInClassAt<T>(clss, offset);
    }

    // Post-parsing work.
    void postParse() {
        // Make a parallel array to "code" recording the block ID for each
        // instructions.
        blockId.clear();
        for (int pc = 0; pc < code.size(); pc++) {
            uint32_t id = 0xFFFFFFFF;

            for (auto label : labels) {
                if (pc >= label.second) {
                    id = label.first;
                }
            }

            if (id == 0xFFFFFFFF) {
                std::cout << "Can't find block for PC " << pc << "\n";
                exit(EXIT_FAILURE);
            }

            blockId.push_back(id);
        }
    }

    void run()
    {
        currentBlockId = 0xFFFFFFFF;
        previousBlockId = 0xFFFFFFFF;

        // Copy constants to memory. They're treated like variables.
        registers = constants;

        // init Function variables with initializers before each invocation
        // XXX also need to initialize within function calls?
        for(auto v: variables) {
            const Variable& var = v.second;
            registers[v.first] = RegisterPointer { var.type, var.storageClass, var.offset };
            if(v.second.storageClass == SpvStorageClassFunction) {
                assert(v.second.initializer == NO_INITIALIZER); // XXX will do initializers later
            }
        }

        size_t oldTop = memoryRegions[SpvStorageClassPrivate].top;
        callstack.push_back(EXECUTION_ENDED); // caller PC
        callstack.push_back(NO_RETURN_REGISTER); // return register
        pc = mainFunction->start;

        do {
            step();
        } while(pc != EXECUTION_ENDED);

        memoryRegions[SpvStorageClassPrivate].top = oldTop;
    }
};

void eval(Interpreter& ip, float x, float y, v4float& color)
{
    if(0) {
        color[0] = x / imageWidth;
        color[1] = y / imageHeight;
        color[2] = 0.5f;
        color[3] = 1.0f;
    } else {
        ip.set(SpvStorageClassInput, 0, v2float {x, y}); // fragCoord is in #0 in preamble
        ip.run();
        ip.get(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
    }
}


std::string readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream::pos_type size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    shaderFile.read(&text[0], size);

    return text;
}

std::string readStdin()
{
    std::istreambuf_iterator<char> begin(std::cin), end;
    return std::string(begin, end);
}

void usage(const char* progname)
{
    printf("usage: %s [options] shader.frag\n", progname);
    printf("provide \"-\" as a filename to read from stdin\n");
    printf("options:\n");
    printf("\t-v    Print opcodes as they are parsed\n");
    printf("\t-g    Generate debugging information\n");
    printf("\t-O    Run optimizing passes\n");
    printf("\t-t    Throw an exception on first unimplemented opcode\n");
    printf("\t-n    Compile and load shader, but do not shade an image\n");
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool optimize = false;
    bool beVerbose = false;
    bool throwOnUnimplemented = false;
    bool doNotShade = false;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-g") == 0) {

            debug = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-v") == 0) {

            beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-t") == 0) {

            throwOnUnimplemented = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-O") == 0) {

            optimize = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-n") == 0) {

            doNotShade = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else if(strcmp(argv[0], "-") == 0) {

            // Read from stdin.
            break;

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }


    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    std::string preambleFilename = "preamble.frag";
    std::string preamble = readFileContents(preambleFilename.c_str());
    std::string epilogueFilename = "epilogue.frag";
    std::string epilogue = readFileContents(epilogueFilename.c_str());

    std::string filename;
    std::string text;
    if(strcmp(argv[0], "-") == 0) {
        filename = "stdin";
        text = readStdin();
    } else {
        filename = argv[0];
        text = readFileContents(filename.c_str());
    }

    glslang::TShader *shader = new glslang::TShader(EShLangFragment);

    {
        const char* strings[3] = { preamble.c_str(), text.c_str(), epilogue.c_str() };
        const char* names[3] = { preambleFilename.c_str(), filename.c_str(), epilogueFilename.c_str() };
        shader->setStringsWithLengthsAndNames(strings, NULL, names, 3);
    }

    shader->setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);

    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

    glslang::TShader::ForbidIncluder includer;
    TBuiltInResource resources;

    resources = glslang::DefaultTBuiltInResource;

    ShInitialize();

    if (!shader->parse(&resources, 110, false, messages, includer)) {
        std::cerr << "compile failed\n";
        std::cerr << shader->getInfoLog();
        exit(EXIT_FAILURE);
    }

    std::vector<unsigned int> spirv;
    std::string warningsErrors;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.generateDebugInfo = debug;
    options.disableOptimizer = !optimize;
    options.optimizeSize = false;
    glslang::TIntermediate *shaderInterm = shader->getIntermediate();
    glslang::GlslangToSpv(*shaderInterm, spirv, &logger, &options);

    if(false) {
        FILE *fp = fopen("spirv", "wb");
        fwrite(spirv.data(), 1, spirv.size() * 4, fp);
        fclose(fp);
    }

    Interpreter ip(throwOnUnimplemented, beVerbose);
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spvBinaryParse(context, &ip, spirv.data(), spirv.size(), Interpreter::handleHeader, Interpreter::handleInstruction, nullptr);

    if (ip.hasUnimplemented) {
        exit(1);
    }

    ip.postParse();

    if (doNotShade) {
        exit(EXIT_SUCCESS);
    }

    auto start_time = std::chrono::steady_clock::now();

    ip.prepare();
    ip.set(SpvStorageClassUniform, 0, v2int {imageWidth, imageHeight}); // iResolution is uniform @0 in preamble
    ip.set(SpvStorageClassUniform, 8, 0.0f); // iTime is uniform @8 in preamble
    for(int y = 0; y < imageHeight; y++)
        for(int x = 0; x < imageWidth; x++) {
            v4float color;
            eval(ip, x + 0.5f, y + 0.5f, color);
            for(int c = 0; c < 3; c++) {
                // ShaderToy clamps the color.
                int byteColor = color[c]*255.99;
                if (byteColor < 0) byteColor = 0;
                if (byteColor > 255) byteColor = 255;
                imageBuffer[imageHeight - 1 - y][x][c] = byteColor;
            }
        }

    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = end_time - start_time;
    auto elapsed_seconds = double(elapsed_time.count())*
        std::chrono::steady_clock::period::num/
        std::chrono::steady_clock::period::den;
    std::cout << "Shading took " << elapsed_seconds << " seconds ("
        << long(imageWidth*imageHeight/elapsed_seconds) << " pixels per second)\n";

    std::ofstream imageFile("shaded.ppm", std::ios::out | std::ios::binary);
    imageFile << "P6 " << imageWidth << " " << imageHeight << " 255\n";
    imageFile.write(reinterpret_cast<const char *>(imageBuffer), sizeof(imageBuffer));
    imageFile.close();

    exit(EXIT_SUCCESS);
}
