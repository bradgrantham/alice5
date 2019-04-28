#ifndef PROGRAM_H
#define PROGRAM_H

#include <string>
#include <map>
#include <set>
#include <memory>
#include <vector>

#include <StandAlone/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>
#include <spirv-tools/libspirv.h>
#include <spirv-tools/optimizer.hpp>
#include "spirv.h"
#include "GLSL.std.450.h"

#include "basic_types.h"
#include "image.h"
#include "timer.h"

// List of shared instruction pointers.
using InstructionList = std::vector<std::shared_ptr<Instruction>>;
#include "opcode_structs.h"

const uint32_t NO_MEMORY_ACCESS_SEMANTIC = 0xFFFFFFFF;

const uint32_t SOURCE_NO_FILE = 0xFFFFFFFF;
const uint32_t NO_INITIALIZER = 0xFFFFFFFF;
const uint32_t NO_ACCESS_QUALIFIER = 0xFFFFFFFF;
const uint32_t EXECUTION_ENDED = 0xFFFFFFFF;
const uint32_t NO_RETURN_REGISTER = 0xFFFFFFFF;

extern std::map<uint32_t, std::string> OpcodeToString;

struct VariableInfo
{
    uint32_t address;       // The address of this variable within memory
    size_t size;            // Size in bytes of this element for type checking
};

// Section of memory for a specific use.
struct MemoryRegion
{
    // Offset within the "memory" array.
    size_t base;

    // Size of the region.
    size_t size;

    // Offset within the "memory" array of next allocation.
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

// The static state of the program.
struct Program 
{
    bool throwOnUnimplemented;
    bool hasUnimplemented;
    bool verbose;
    std::set<uint32_t> capabilities;

    LineInfo currentLine;

    // main id-to-thingie map containing extinstsets, types, variables, etc
    // secondary maps of entryPoint, decorations, names, etc

    std::map<uint32_t, std::string> extInstSets;
    uint32_t ExtInstGLSL_std_450_id;
    std::map<uint32_t, std::string> strings;
    uint32_t memoryModel;
    uint32_t addressingModel;
    std::map<uint32_t, EntryPoint> entryPoints;

    std::map<uint32_t, std::string> names;
    std::map<uint32_t, std::map<uint32_t, std::string> > memberNames;

    // decorations is indexed by id, then decoration type, yielding operands.
    // e.g. offset = decorations[variable_fragCoord][SpvDecorationLocation][0];
    std::map<uint32_t, Decorations > decorations;

    // memberDecorations is indexed by id, then member number,
    // then decoration type, yielding operands.
    // e.g. offset = decorations[var_params][mem_iTime][SpvDecorationOffset][0];
    std::map<uint32_t, std::map<uint32_t, Decorations> > memberDecorations;

    std::vector<Source> sources;
    std::vector<std::string> processes;
    std::map<uint32_t, std::shared_ptr<Type>> types;
    std::map<uint32_t, size_t> typeSizes; // XXX put into Type
    std::map<uint32_t, Variable> variables;
    std::map<uint32_t, Function> functions;
    InstructionList instructions;
    // Map from label ID to block object.
    std::map<uint32_t, std::unique_ptr<Block>> blocks;
    // Map from virtual register ID to its type. Only used for results of instructions.
    std::map<uint32_t, uint32_t> resultTypes;
    std::map<uint32_t, Register> constants;
    std::map<std::string, VariableInfo> namedVariables;

    // For expanding vectors to scalars:
    uint32_t nextReg = 1000; // XXX make sure this doesn't conflict with actual registers.
    using RegIndex = std::pair<uint32_t,int>;
    std::map<RegIndex,uint32_t> vec2scalar;

    SampledImage sampledImages[16];

    Function* currentFunction;
    // Map from label ID to index into instructions vector.
    std::map<uint32_t, uint32_t> labels;
    Function* mainFunction; 

    size_t memorySize;

    std::map<uint32_t, MemoryRegion> memoryRegions;

    // Returns the type as the specific subtype. Does not check to see
    // whether the object is of the specific subtype.
    template <class T>
    const T *type(uint32_t typeId) const {
        return dynamic_cast<const T *>(types.at(typeId).get());
    }

    Register& allocConstantObject(uint32_t id, uint32_t type)
    {
        constants[id] = Register {type, typeSizes[type]};
        constants[id].initialized = true;
        return constants[id];
    }

    Program(bool throwOnUnimplemented_, bool verbose_);

    size_t allocate(SpvStorageClass clss, uint32_t type)
    {
        MemoryRegion& reg = memoryRegions[clss];
        if(false) {
            std::cout << "allocate from " << clss << " type " << type << "\n";
            std::cout << "region at " << reg.base << " size " << reg.size << " top " << reg.top << "\n";
            std::cout << "object is size " << typeSizes[type] << "\n";
        }
        assert(reg.top + typeSizes[type] <= reg.base + reg.size);
        size_t address = reg.top;
        reg.top += typeSizes[type];
        return address;
    }
    size_t allocate(uint32_t clss, uint32_t type)
    {
        return allocate(static_cast<SpvStorageClass>(clss), type);
    }

    // Returns the type of and byte offset to the member of "t" at
    // index "i".
    // For vectors, "i" is ignored and the type of any
    // element is returned and the offset by type is returned.
    // For matrices, "i" is ignored and the type of any column is
    // returned and the offset by column type is returned.
    // For structs, the type of field "i" (0-indexed) is returned
    // and the offset is looked up in offset decorations.
    ConstituentInfo getConstituentInfo(uint32_t t, int i) const
    {
        const Type *type = types.at(t).get();
        return type->getConstituentInfo(i);
        /*
        if(std::holds_alternative<TypeVector>(type)) {
            uint32_t elementType = std::get<TypeVector>(type).type;
            return std::make_tuple(elementType, typeSizes.at(elementType) * i);
        } else if(std::holds_alternative<TypeArray>(type)) {
            uint32_t elementType = std::get<TypeArray>(type).type;
            return std::make_tuple(elementType, typeSizes.at(elementType) * i);
        } else if(std::holds_alternative<TypeMatrix>(type)) {
            uint32_t columnType = std::get<TypeMatrix>(type).columnType;
            return std::make_tuple(columnType, typeSizes.at(columnType) * i);
        } else if (std::holds_alternative<TypeStruct>(type)) {
            uint32_t memberType = std::get<TypeStruct>(type).memberTypes[i];
            return std::make_tuple(memberType, memberDecorations.at(t).at(i).at(SpvDecorationOffset)[0]);
        } else {
            std::cout << type.index() << "\n";
            assert(false && "getConstituentType of invalid type?!");
        }
        return std::make_tuple(0, 0); // not reached
        */
    }

    // If the type ID refers to a TypeVector, returns it. Otherwise returns null.
    const TypeVector *getTypeAsVector(int typeId) const {
        const Type *type = types.at(typeId).get();
        return type->op() == SpvOpTypeVector ? dynamic_cast<const TypeVector *>(type) : nullptr;
    }

    // Returns true if a type is a float, false if it's an integer or pointer,
    // otherwise asserts.
    bool isTypeFloat(uint32_t typeId) const {
        const Type *type = types.at(typeId).get();
        if (type->op() == SpvOpTypeInt || type->op() == SpvOpTypePointer) {
            return false;
        } else if (type->op() == SpvOpTypeVector) {
            return true;
        } else {
            std::cerr << "Error: Type " << typeId << " is neither int nor float.\n";
            assert(false);
        }
    }

    static spv_result_t handleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema);

    static spv_result_t handleInstruction(void* user_data, const spv_parsed_instruction_t* insn);

    void storeNamedVariableInfo(const std::string& name, uint32_t typeId, uint32_t address)
    {
        const Type *type = types[typeId].get();

        switch (type->op()) {
            case SpvOpTypeStruct: {
                const TypeStruct *typeStruct = dynamic_cast<const TypeStruct *>(type);

                for(int i = 0; i < typeStruct->memberTypes.size(); i++) {
                    uint32_t membertype = typeStruct->memberTypeIds[i];
                    std::string fullname = ((name == "") ? "" : (name + ".")) + memberNames[typeId][i];
                    storeNamedVariableInfo(fullname, membertype, address + memberDecorations[typeId][i][SpvDecorationOffset][0]);
                }
                break;
            }

            case SpvOpTypeArray: {
                const TypeArray *typeArray = dynamic_cast<const TypeArray *>(type);

                for(int i = 0; i < typeArray->count; i++) {
                    uint32_t membertype = typeArray->type;
                    std::string fullname = name + "[" + std::to_string(i) + "]";
                    storeNamedVariableInfo(fullname, membertype, address + i * typeSizes.at(membertype));
                }
                break;
            }

            case SpvOpTypeVector:
            case SpvOpTypeFloat:
            case SpvOpTypeInt:
            case SpvOpTypeSampledImage:
            case SpvOpTypeBool:
            case SpvOpTypeMatrix: {
                namedVariables[name] = {address, typeSizes.at(typeId)};
                break;
            }

            default:
                std::cout << "Unhandled type for finding uniform variable offset and size\n";
                break;
        }
    }

    // Post-parsing work.
    void postParse(bool scalarize);

    // Return the scalar for the vector register's index.
    uint32_t scalarize(uint32_t vreg, int i, uint32_t subtype, uint32_t scalarReg = 0);

    // Transform vector instructions to scalar instructions. inList and outList
    // may be the same list.
    void expandVectors(const InstructionList &inList, InstructionList &outList);

    // Expand a binary operator, such as FMul, from vector to scalar if necessary.
    template <class T>
    void expandVectorsBinOp(Instruction *instruction, InstructionList &newList, bool &replaced) {
        T *insn = dynamic_cast<T *>(instruction);
        const TypeVector *typeVector = getTypeAsVector(resultTypes.at(insn->resultId));
        if (typeVector != nullptr) {
            for (int i = 0; i < typeVector->count; i++) {
                auto [subtype, offset] = getConstituentInfo(insn->type, i);
                newList.push_back(std::make_shared<T>(insn->lineInfo,
                            subtype,
                            scalarize(insn->resultId, i, subtype),
                            scalarize(insn->operand1Id, i, subtype),
                            scalarize(insn->operand2Id, i, subtype)));
            }
            replaced = true;
        }
    }

    // Take "mainImage(vf4;vf2;" and return "mainImage$v4f$vf2".
    std::string cleanUpFunctionName(int nameId) const {
        std::string name = names.at(nameId);

        // Replace "mainImage(vf4;vf2;" with "mainImage$v4f$vf2$"
        for (int i = 0; i < name.length(); i++) {
            if (name[i] == '(' || name[i] == ';') {
                name[i] = '$';
            }
        }

        // Strip trailing dollar sign.
        if (name.length() > 0 && name[name.length() - 1] == '$') {
            name = name.substr(0, name.length() - 1);
        }

        return name;
    }
};

#endif /* PROGRAM_H */
