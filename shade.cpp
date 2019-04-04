#include <iostream>
#include <cstdio>
#include <fstream>
#include <variant>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <iomanip>

#include <StandAlone/ResourceLimits.h>
#include <glslang/MachineIndependent/localintermediate.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.h>
#include "spirv.h"
#include "GLSL.std.450.h"
#include "basic_types.h"
#include "interpreter.h"

const int imageWidth = 640; // ShaderToy default is 640
const int imageHeight = 360; // ShaderToy default is 360
unsigned char imageBuffer[imageHeight][imageWidth][3];

std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

std::map<uint32_t, std::string> GLSLstd450OpcodeToString = {
#include "GLSLstd450_opcode_to_string.h"
};

#include "opcode_structs.h"

const uint32_t NO_MEMORY_ACCESS_SEMANTIC = 0xFFFFFFFF;

const uint32_t SOURCE_NO_FILE = 0xFFFFFFFF;
const uint32_t NO_INITIALIZER = 0xFFFFFFFF;
const uint32_t NO_ACCESS_QUALIFIER = 0xFFFFFFFF;
const uint32_t EXECUTION_ENDED = 0xFFFFFFFF;
const uint32_t NO_RETURN_REGISTER = 0xFFFFFFFF;
const uint32_t NO_BLOCK_ID = 0xFFFFFFFF;

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

template <class T>
T& objectAt(unsigned char* data)
{
    return *reinterpret_cast<T*>(data);
}

// The static state of the program.
struct Program 
{
    bool throwOnUnimplemented;
    bool hasUnimplemented;
    bool verbose;
    std::set<uint32_t> capabilities;

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
    std::vector<Instruction *> code;
    std::vector<uint32_t> blockId; // Parallel to "code".

    Function* currentFunction;
    // Map from label ID to index into code vector.
    std::map<uint32_t, uint32_t> labels;
    Function* mainFunction; 

    size_t memorySize;

    std::map<uint32_t, MemoryRegion> memoryRegions;

    std::map<uint32_t, uint32_t> resultsCreated;
    std::map<uint32_t, Register> constants;
    Register& allocConstantObject(uint32_t id, uint32_t type)
    {
        Register r {type, typeSizes[type]};
        constants[id] = r;
        return constants[id];
    }

    Program(bool throwOnUnimplemented_, bool verbose_) :
        throwOnUnimplemented(throwOnUnimplemented_),
        hasUnimplemented(false),
        verbose(verbose_),
        currentFunction(nullptr)
    {
        memorySize = 0;
        auto anotherRegion = [this](size_t size){MemoryRegion r(memorySize, size); memorySize += size; return r;};
        memoryRegions[SpvStorageClassUniformConstant] = anotherRegion(1024);
        memoryRegions[SpvStorageClassInput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassUniform] = anotherRegion(1024);
        memoryRegions[SpvStorageClassOutput] = anotherRegion(1024);
        memoryRegions[SpvStorageClassWorkgroup] = anotherRegion(0);
        memoryRegions[SpvStorageClassCrossWorkgroup] = anotherRegion(0);
        memoryRegions[SpvStorageClassPrivate] = anotherRegion(65536); // XXX still needed?
        memoryRegions[SpvStorageClassFunction] = anotherRegion(16384);
        memoryRegions[SpvStorageClassGeneric] = anotherRegion(0);
        memoryRegions[SpvStorageClassPushConstant] = anotherRegion(0);
        memoryRegions[SpvStorageClassAtomicCounter] = anotherRegion(0);
        memoryRegions[SpvStorageClassImage] = anotherRegion(0);
        memoryRegions[SpvStorageClassStorageBuffer] = anotherRegion(0);
    }

    ~Program()
    {
        for (auto insn : code) {
            delete insn;
        }
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

    // Returns the type of the member of "t" at index "i". For vectors,
    // "i" is ignored and the type of any element is returned. For matrices,
    // "i" is ignored and the type of any column is returned. For structs,
    // the type of field "i" (0-indexed) is returned.
    uint32_t getConstituentType(uint32_t t, int i) const
    {
        const Type& type = types.at(t);
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

    void dumpTypeAt(const Type& type, unsigned char *ptr) const
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
                    dumpTypeAt(types.at(type.type), ptr);
                    ptr += typeSizes.at(type.type);
                    if(i < type.count - 1)
                        std::cout << ", ";
                }
                std::cout << ">";
            },
            [&](const TypeMatrix& type) {
                std::cout << "<";
                for(int i = 0; i < type.columnCount; i++) {
                    dumpTypeAt(types.at(type.columnType), ptr);
                    ptr += typeSizes.at(type.columnType);
                    if(i < type.columnCount - 1)
                        std::cout << ", ";
                }
                std::cout << ">";
            },
            [&](const TypeStruct& type) {
                std::cout << "{";
                for(int i = 0; i < type.memberTypes.size(); i++) {
                    dumpTypeAt(types.at(type.memberTypes[i]), ptr);
                    ptr += typeSizes.at(type.memberTypes[i]);
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
        // auto pgm = static_cast<Program*>(user_data);
        return SPV_SUCCESS;
    }

    static spv_result_t handleInstruction(void* user_data, const spv_parsed_instruction_t* insn)
    {
        auto pgm = static_cast<Program*>(user_data);

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
                pgm->capabilities.insert(cap);
                if(pgm->verbose) {
                    std::cout << "OpCapability " << cap << " \n";
                }
                break;
            }

            case SpvOpExtInstImport: {
                // XXX result id
                uint32_t id = nextu();
                const char *name = nexts();
                if(strcmp(name, "GLSL.std.450") == 0) {
                    pgm->ExtInstGLSL_std_450_id = id;
                } else {
                    throw std::runtime_error("unimplemented extension instruction set \"" + std::string(name) + "\"");
                }
                pgm->extInstSets[id] = name;
                if(pgm->verbose) {
                    std::cout << "OpExtInstImport " << insn->words[opds[0].offset] << " " << name << "\n";
                }
                break;
            }

            case SpvOpMemoryModel: {
                pgm->addressingModel = nextu();
                pgm->memoryModel = nextu();
                assert(pgm->addressingModel == SpvAddressingModelLogical);
                assert(pgm->memoryModel == SpvMemoryModelGLSL450);
                if(pgm->verbose) {
                    std::cout << "OpMemoryModel " << pgm->addressingModel << " " << pgm->memoryModel << "\n";
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
                pgm->entryPoints[id] = {executionModel, name, interfaceIds};
                if(pgm->verbose) {
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
                pgm->entryPoints[entryPointId].executionModesToOperands[executionMode] = operands;

                if(pgm->verbose) {
                    std::cout << "OpExecutionMode " << entryPointId << " " << executionMode;
                    for(auto& i: pgm->entryPoints[entryPointId].executionModesToOperands[executionMode])
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpString: {
                // XXX result id
                uint32_t id = nextu();
                std::string name = nexts();
                pgm->strings[id] = name;
                if(pgm->verbose) {
                    std::cout << "OpString " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpName: {
                uint32_t id = nextu();
                std::string name = nexts();
                pgm->names[id] = name;
                if(pgm->verbose) {
                    std::cout << "OpName " << id << " " << name << "\n";
                }
                break;
            }

            case SpvOpSource: {
                uint32_t language = nextu();
                uint32_t version = nextu();
                uint32_t file = nextu(SOURCE_NO_FILE);
                std::string source = (insn->num_operands > 3) ? nexts() : "";
                pgm->sources.push_back({language, version, file, source});
                if(pgm->verbose) {
                    std::cout << "OpSource " << language << " " << version << " " << file << " " << ((source.size() > 0) ? "with source" : "without source") << "\n";
                }
                break;
            }

            case SpvOpMemberName: {
                uint32_t type = nextu();
                uint32_t member = nextu();
                std::string name = nexts();
                pgm->memberNames[type][member] = name;
                if(pgm->verbose) {
                    std::cout << "OpMemberName " << type << " " << member << " " << name << "\n";
                }
                break;
            }

            case SpvOpModuleProcessed: {
                std::string process = nexts();
                pgm->processes.push_back(process);
                if(pgm->verbose) {
                    std::cout << "OpModulesProcessed " << process << "\n";
                }
                break;
            }

            case SpvOpDecorate: {
                uint32_t id = nextu();
                uint32_t decoration = nextu();
                std::vector<uint32_t> operands = nextv();
                pgm->decorations[id] = {decoration, operands};
                if(pgm->verbose) {
                    std::cout << "OpDecorate " << id << " " << decoration;
                    for(auto& i: pgm->decorations[id].operands)
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
                pgm->memberDecorations[id][member] = {decoration, operands};
                if(pgm->verbose) {
                    std::cout << "OpMemberDecorate " << id << " " << member << " " << decoration;
                    for(auto& i: pgm->memberDecorations[id][member].operands)
                        std::cout << " " << i;
                    std::cout << "\n";
                }
                break;
            }

            case SpvOpTypeVoid: {
                // XXX result id
                uint32_t id = nextu();
                pgm->types[id] = TypeVoid {};
                pgm->typeSizes[id] = 0;
                if(pgm->verbose) {
                    std::cout << "TypeVoid " << id << "\n";
                }
                break;
            }

            case SpvOpTypeBool: {
                // XXX result id
                uint32_t id = nextu();
                pgm->types[id] = TypeBool {};
                pgm->typeSizes[id] = sizeof(bool);
                if(pgm->verbose) {
                    std::cout << "TypeBool " << id << "\n";
                }
                break;
            }

            case SpvOpTypeFloat: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t width = nextu();
                assert(width <= 32); // XXX deal with larger later
                pgm->types[id] = TypeFloat {width};
                pgm->typeSizes[id] = ((width + 31) / 32) * 4;
                if(pgm->verbose) {
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
                pgm->types[id] = TypeInt {width, signedness};
                pgm->typeSizes[id] = ((width + 31) / 32) * 4;
                if(pgm->verbose) {
                    std::cout << "TypeInt " << id << " width " << width << " signedness " << signedness << "\n";
                }
                break;
            }

            case SpvOpTypeFunction: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t returnType = nextu();
                std::vector<uint32_t> params = restv();
                pgm->types[id] = TypeFunction {returnType, params};
                pgm->typeSizes[id] = 4; // XXX ?!?!?
                if(pgm->verbose) {
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
                pgm->types[id] = TypeVector {type, count};
                pgm->typeSizes[id] = pgm->typeSizes[type] * count;
                if(pgm->verbose) {
                    std::cout << "TypeVector " << id << " of " << type << " count " << count << "\n";
                }
                break;
            }

            case SpvOpTypeMatrix: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t columnType = nextu();
                uint32_t columnCount = nextu();
                pgm->types[id] = TypeMatrix {columnType, columnCount};
                pgm->typeSizes[id] = pgm->typeSizes[columnType] * columnCount;
                if(pgm->verbose) {
                    std::cout << "TypeMatrix " << id << " of " << columnType << " count " << columnCount << "\n";
                }
                break;
            }

            case SpvOpTypePointer: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t storageClass = nextu();
                uint32_t type = nextu();
                pgm->types[id] = TypePointer {type, storageClass};
                pgm->typeSizes[id] = sizeof(uint32_t);
                if(pgm->verbose) {
                    std::cout << "TypePointer " << id << " class " << storageClass << " type " << type << "\n";
                }
                break;
            }

            case SpvOpTypeStruct: {
                // XXX result id
                uint32_t id = nextu();
                std::vector<uint32_t> memberTypes = restv();
                pgm->types[id] = TypeStruct {memberTypes};
                size_t size = 0;
                for(auto& t: memberTypes) {
                    size += pgm->typeSizes[t];
                }
                pgm->typeSizes[id] = size;
                if(pgm->verbose) {
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
                uint32_t pointedType = std::get<TypePointer>(pgm->types[type]).type;
                size_t offset = pgm->allocate(storageClass, pointedType);
                pgm->variables[id] = {pointedType, storageClass, initializer, offset};
                if(pgm->verbose) {
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
                Register& r = pgm->allocConstantObject(id, typeId);
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + pgm->typeSizes[typeId], r.data);
                if(pgm->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantTrue: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                Register& r = pgm->allocConstantObject(id, typeId);
                bool value = true;
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + pgm->typeSizes[typeId], r.data);
                if(pgm->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantFalse: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                Register& r = pgm->allocConstantObject(id, typeId);
                bool value = false;
                const unsigned char *data = reinterpret_cast<const unsigned char*>(&value);
                std::copy(data, data + pgm->typeSizes[typeId], r.data);
                if(pgm->verbose) {
                    std::cout << "Constant " << id << " type " << typeId << " value " << value << "\n";
                }
                break;
            }

            case SpvOpConstantComposite: {
                // XXX result id
                uint32_t typeId = nextu();
                uint32_t id = nextu();
                std::vector<uint32_t> operands = restv();
                Register& r = pgm->allocConstantObject(id, typeId);
                uint32_t offset = 0;
                for(uint32_t operand : operands) {
                    // Copy each operand from a constant into our new composite constant.
                    const Register &src = pgm->constants[operand];
                    uint32_t size = pgm->typeSizes[src.type];
                    std::copy(src.data, src.data + size, r.data + offset);
                    offset += size;
                }
                assert(offset = pgm->typeSizes[typeId]);
                break;
            }

            case SpvOpTypeSampledImage: {
                // XXX result id
                uint32_t id = nextu();
                uint32_t imageType = nextu();
                pgm->types[id] = TypeSampledImage { imageType };
                if(pgm->verbose) {
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
                pgm->types[id] = TypeImage { sampledType, dim, depth, arrayed, ms, sampled, imageFormat, accessQualifier };
                if(pgm->verbose) {
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
                uint32_t start = pgm->code.size();
                pgm->functions[id] = Function {id, resultType, functionControl, functionType, start };
                pgm->currentFunction = &pgm->functions[id];
                if(pgm->verbose) {
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
                pgm->labels[id] = pgm->code.size();
                if(pgm->verbose) {
                    std::cout << "Label " << id
                        << " at " << pgm->labels[id]
                        << "\n";
                }
                break;
            }

            case SpvOpFunctionEnd: {
                pgm->currentFunction = NULL;
                if(pgm->verbose) {
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
                if(pgm->throwOnUnimplemented) {
                    throw std::runtime_error("unimplemented opcode " + OpcodeToString[insn->opcode] + " (" + std::to_string(insn->opcode) + ")");
                } else {
                    std::cout << "unimplemented opcode " << OpcodeToString[insn->opcode] << " (" << insn->opcode << ")\n";
                    pgm->hasUnimplemented = true;
                }
                break;
            }
        }

        return SPV_SUCCESS;
    }

    // Post-parsing work.
    void postParse() {
        // Find the main function.
        mainFunction = nullptr;
        for(auto& e: entryPoints) {
            if(e.second.name == "main") {
                mainFunction = &functions[e.first];
            }
        }

        // Make a parallel array to "code" recording the block ID for each
        // instruction. First create a list of <index,labelId> pairs. These
        // are the same pairs as in the "labels" map but with the two elements
        // switched.
        typedef std::pair<uint32_t,uint32_t> IndexLabelId;
        std::vector<IndexLabelId> labels_in_order;
        for (auto label : labels) {
            labels_in_order.push_back(IndexLabelId{label.second, label.first});
        }
        // Add pseudo-entry for end of code.
        labels_in_order.push_back(IndexLabelId{code.size(), NO_BLOCK_ID});
        // Sort by code index.
        std::sort(labels_in_order.begin(), labels_in_order.end());

        // Create parallel array. Each entry in the "labels_in_order" vector is
        // a range of code indices.
        blockId.clear();
        for (int i = 0; i < labels_in_order.size() - 1; i++) {
            for (int j = labels_in_order[i].first; j < labels_in_order[i + 1].first; j++) {
                blockId.push_back(labels_in_order[i].second);
            }
        }
    }
};

Interpreter::Interpreter(const Program *pgm)
    : pgm(pgm)
{
    memory = new unsigned char[pgm->memorySize];

    // So we can catch errors.
    std::fill(memory, memory + pgm->memorySize, 0xFF);

    // Allocate registers so they aren't allocated during run()
    for(auto r: pgm->resultsCreated) {
        allocRegister(r.first, r.second);
    }
}

Register& Interpreter::allocRegister(uint32_t id, uint32_t type)
{
    Register r {type, pgm->typeSizes.at(type)};
    registers[id] = r;
    return registers[id];
}

void Interpreter::copy(uint32_t type, size_t src, size_t dst)
{
    std::copy(memory + src, memory + src + pgm->typeSizes.at(type), memory + dst);
}

template <class T>
T& Interpreter::objectInClassAt(SpvStorageClass clss, size_t offset)
{
    return *reinterpret_cast<T*>(memory + pgm->memoryRegions.at(clss).base + offset);
}

template <class T>
T& Interpreter::registerAs(int id)
{
    return *reinterpret_cast<T*>(registers[id].data);
}

void Interpreter::clearPrivateVariables()
{
    // Global variables are cleared for each run.
    const MemoryRegion &mr = pgm->memoryRegions.at(SpvStorageClassPrivate);
    std::fill(memory + mr.base, memory + mr.top, 0x00);
}

void Interpreter::stepLoad(const InsnLoad& insn)
{
    Pointer& ptr = pointers.at(insn.pointerId);
    Register& obj = registers[insn.resultId];
    std::copy(memory + ptr.offset, memory + ptr.offset + pgm->typeSizes.at(insn.type), obj.data);
    if(false) {
        std::cout << "load result is";
        pgm->dumpTypeAt(pgm->types.at(insn.type), obj.data);
        std::cout << "\n";
    }
}

void Interpreter::stepStore(const InsnStore& insn)
{
    Pointer& ptr = pointers.at(insn.pointerId);
    Register& obj = registers[insn.objectId];
    std::copy(obj.data, obj.data + obj.size, memory + ptr.offset);
}

void Interpreter::stepCompositeExtract(const InsnCompositeExtract& insn)
{
    Register& obj = registers[insn.resultId];
    Register& src = registers[insn.compositeId];
    /* use indexes to walk blob */
    uint32_t type = src.type;
    size_t offset = 0;
    for(auto& j: insn.indexesId) {
        for(int i = 0; i < j; i++) {
            offset += pgm->typeSizes.at(pgm->getConstituentType(type, i));
        }
        type = pgm->getConstituentType(type, j);
    }
    std::copy(src.data + offset, src.data + offset + pgm->typeSizes.at(obj.type), obj.data);
    if(false) {
        std::cout << "extracted from ";
        pgm->dumpTypeAt(pgm->types.at(src.type), src.data);
        std::cout << " result is ";
        pgm->dumpTypeAt(pgm->types.at(insn.type), obj.data);
        std::cout << "\n";
    }
}

void Interpreter::stepCompositeConstruct(const InsnCompositeConstruct& insn)
{
    Register& obj = registers[insn.resultId];
    size_t offset = 0;
    for(auto& j: insn.constituentsId) {
        Register& src = registers[j];
        std::copy(src.data, src.data + pgm->typeSizes.at(src.type), obj.data + offset);
        offset += pgm->typeSizes.at(src.type);
    }
    if(false) {
        std::cout << "constructed ";
        pgm->dumpTypeAt(pgm->types.at(obj.type), obj.data);
        std::cout << "\n";
    }
}

void Interpreter::stepIAdd(const InsnIAdd& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFAdd(const InsnFAdd& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFSub(const InsnFSub& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFMul(const InsnFMul& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFDiv(const InsnFDiv& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFMod(const InsnFMod& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFOrdLessThan(const InsnFOrdLessThan& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepFOrdGreaterThan(const InsnFOrdGreaterThan& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepFOrdLessThanEqual(const InsnFOrdLessThanEqual& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepFOrdEqual(const InsnFOrdEqual& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepFNegate(const InsnFNegate& insn)
{
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
    }, pgm->types.at(insn.type));
}

// Computes the dot product of two vectors.
float dotProduct(float *a, float *b, int count)
{
    float dot = 0.0;

    for (int i = 0; i < count; i++) {
        dot += a[i]*b[i];
    }

    return dot;
}

void Interpreter::stepDot(const InsnDot& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        const Register &r1 = registers[insn.vector1Id];
        const TypeVector &t1 = std::get<TypeVector>(pgm->types.at(r1.type));

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float* vector1 = &registerAs<float>(insn.vector1Id);
            float* vector2 = &registerAs<float>(insn.vector2Id);
            registerAs<float>(insn.resultId) = dotProduct(vector1, vector2, t1.count);

        } else {

            std::cout << "Unknown type for Dot\n";

        }
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFOrdGreaterThanEqual(const InsnFOrdGreaterThanEqual& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepSLessThan(const InsnSLessThan& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepSDiv(const InsnSDiv& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            int32_t operand1 = registerAs<int32_t>(insn.operand1Id);
            int32_t operand2 = registerAs<int32_t>(insn.operand2Id);
            int32_t result = operand1 / operand2;
            registerAs<int32_t>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            int32_t* operand1 = &registerAs<int32_t>(insn.operand1Id);
            int32_t* operand2 = &registerAs<int32_t>(insn.operand2Id);
            int32_t* result = &registerAs<int32_t>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] / operand2[i];
            }

        } else {

            std::cout << "Unknown type for SDiv\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepIEqual(const InsnIEqual& insn)
{
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
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepLogicalNot(const InsnLogicalNot& insn)
{
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
    }, pgm->types.at(registers[insn.operandId].type));
}

void Interpreter::stepLogicalOr(const InsnLogicalOr& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeBool>) {

            bool operand1 = registerAs<bool>(insn.operand1Id);
            bool operand2 = registerAs<bool>(insn.operand2Id);
            registerAs<bool>(insn.resultId) = operand1 || operand2;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            bool* operand1 = &registerAs<bool>(insn.operand1Id);
            bool* operand2 = &registerAs<bool>(insn.operand2Id);
            bool* result = &registerAs<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] || operand2[i];
            }

        } else {

            std::cout << "Unknown type for LogicalOr\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepSelect(const InsnSelect& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepVectorTimesScalar(const InsnVectorTimesScalar& insn)
{
    float* vector = &registerAs<float>(insn.vectorId);
    float scalar = registerAs<float>(insn.scalarId);
    float* result = &registerAs<float>(insn.resultId);

    const TypeVector &type = std::get<TypeVector>(pgm->types.at(insn.type));

    for(int i = 0; i < type.count; i++) {
        result[i] = vector[i] * scalar;
    }
}

void Interpreter::stepMatrixTimesVector(const InsnMatrixTimesVector& insn)
{
    float* matrix = &registerAs<float>(insn.matrixId);
    float* vector = &registerAs<float>(insn.vectorId);
    float* result = &registerAs<float>(insn.resultId);

    const Register &vectorReg = registers[insn.vectorId];

    const TypeVector &resultType = std::get<TypeVector>(pgm->types.at(insn.type));
    const TypeVector &vectorType = std::get<TypeVector>(pgm->types.at(vectorReg.type));

    int rn = resultType.count;
    int vn = vectorType.count;

    for(int i = 0; i < rn; i++) {
        float dot = 0.0;

        for(int j = 0; j < vn; j++) {
            dot += matrix[i + j*rn]*vector[j];
        }

        result[i] = dot;
    }
}

void Interpreter::stepVectorTimesMatrix(const InsnVectorTimesMatrix& insn)
{
    float* vector = &registerAs<float>(insn.vectorId);
    float* matrix = &registerAs<float>(insn.matrixId);
    float* result = &registerAs<float>(insn.resultId);

    const Register &vectorReg = registers[insn.vectorId];

    const TypeVector &resultType = std::get<TypeVector>(pgm->types.at(insn.type));
    const TypeVector &vectorType = std::get<TypeVector>(pgm->types.at(vectorReg.type));

    int rn = resultType.count;
    int vn = vectorType.count;

    for(int i = 0; i < rn; i++) {
        result[i] = dotProduct(vector, matrix + vn*i, vn);
    }
}

void Interpreter::stepVectorShuffle(const InsnVectorShuffle& insn)
{
    Register& obj = registers[insn.resultId];
    const Register &r1 = registers[insn.vector1Id];
    const Register &r2 = registers[insn.vector2Id];
    const TypeVector &t1 = std::get<TypeVector>(pgm->types.at(r1.type));
    uint32_t n1 = t1.count;
    uint32_t elementSize = pgm->typeSizes.at(t1.type);

    for(int i = 0; i < insn.componentsId.size(); i++) {
        uint32_t component = insn.componentsId[i];
        unsigned char *src = component < n1
            ? r1.data + component*elementSize
            : r2.data + (component - n1)*elementSize;
        std::copy(src, src + elementSize, obj.data + i*elementSize);
    }
}

void Interpreter::stepConvertSToF(const InsnConvertSToF& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepConvertFToS(const InsnConvertFToS& insn)
{
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
    }, pgm->types.at(insn.type));
}

void Interpreter::stepAccessChain(const InsnAccessChain& insn)
{
    Pointer& basePointer = pointers.at(insn.baseId);
    uint32_t type = basePointer.type;
    size_t offset = basePointer.offset;
    for(auto& id: insn.indexesId) {
        int32_t j = registerAs<int32_t>(id);
        for(int i = 0; i < j; i++) {
            offset += pgm->typeSizes.at(pgm->getConstituentType(type, i));
        }
        type = pgm->getConstituentType(type, j);
    }
    if(false) {
        std::cout << "accesschain of " << basePointer.offset << " yielded " << offset << "\n";
    }
    uint32_t pointedType = std::get<TypePointer>(pgm->types.at(insn.type)).type;
    pointers[insn.resultId] = Pointer { pointedType, basePointer.storageClass, offset };
}

void Interpreter::stepFunctionParameter(const InsnFunctionParameter& insn)
{
    uint32_t sourceId = callstack.back(); callstack.pop_back();
    // XXX is this ever a register?
    pointers[insn.resultId] = pointers[sourceId];
    if(false) std::cout << "function parameter " << insn.resultId << " receives " << sourceId << "\n";
}

void Interpreter::stepReturn(const InsnReturn& insn)
{
    callstack.pop_back(); // return parameter not used.
    pc = callstack.back(); callstack.pop_back();
}

void Interpreter::stepReturnValue(const InsnReturnValue& insn)
{
    // Return value.
    uint32_t returnId = callstack.back(); callstack.pop_back();
    registers[returnId] = registers[insn.valueId];

    pc = callstack.back(); callstack.pop_back();
}

void Interpreter::stepFunctionCall(const InsnFunctionCall& insn)
{
    const Function& function = pgm->functions.at(insn.functionId);

    callstack.push_back(pc);
    callstack.push_back(insn.resultId);
    for(int i = insn.operandId.size() - 1; i >= 0; i--) {
        callstack.push_back(insn.operandId[i]);
    }
    pc = function.start;
}

void Interpreter::stepGLSLstd450Distance(const InsnGLSLstd450Distance& insn)
{
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
    }, pgm->types.at(registers[insn.p0Id].type));
}

void Interpreter::stepGLSLstd450Length(const InsnGLSLstd450Length& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FMax(const InsnGLSLstd450FMax& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            float y = registerAs<float>(insn.yId);
            registerAs<float>(insn.resultId) = fmaxf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* y = &registerAs<float>(insn.yId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = fmaxf(x[i], y[i]);
            }

        } else {

            std::cout << "Unknown type for FMax\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FMin(const InsnGLSLstd450FMin& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            float y = registerAs<float>(insn.yId);
            registerAs<float>(insn.resultId) = fminf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* y = &registerAs<float>(insn.yId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = fminf(x[i], y[i]);
            }

        } else {

            std::cout << "Unknown type for FMin\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Pow(const InsnGLSLstd450Pow& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            float y = registerAs<float>(insn.yId);
            registerAs<float>(insn.resultId) = powf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* y = &registerAs<float>(insn.yId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = powf(x[i], y[i]);
            }

        } else {

            std::cout << "Unknown type for Pow\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Normalize(const InsnGLSLstd450Normalize& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Sin(const InsnGLSLstd450Sin& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Cos(const InsnGLSLstd450Cos& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Atan(const InsnGLSLstd450Atan& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float y_over_x = registerAs<float>(insn.y_over_xId);
            registerAs<float>(insn.resultId) = atanf(y_over_x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* y_over_x = &registerAs<float>(insn.y_over_xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = atanf(y_over_x[i]);
            }

        } else {

            std::cout << "Unknown type for Atan\n";

        }
    }, pgm->types.at(registers[insn.y_over_xId].type));
}

void Interpreter::stepGLSLstd450Atan2(const InsnGLSLstd450Atan2& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float y = registerAs<float>(insn.yId);
            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = atan2f(y, x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* y = &registerAs<float>(insn.yId);
            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = atan2f(y[i], x[i]);
            }

        } else {

            std::cout << "Unknown type for Atan2\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FAbs(const InsnGLSLstd450FAbs& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Exp(const InsnGLSLstd450Exp& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = expf(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = expf(x[i]);
            }

        } else {

            std::cout << "Unknown type for Exp\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Exp2(const InsnGLSLstd450Exp2& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = exp2f(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = exp2f(x[i]);
            }

        } else {

            std::cout << "Unknown type for Exp2\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Floor(const InsnGLSLstd450Floor& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Fract(const InsnGLSLstd450Fract& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = x - floor(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = x[i] - floor(x[i]);
            }

        } else {

            std::cout << "Unknown type for Fract\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

// Returns the value x clamped to the range minVal to maxVal per the GLSL docs.
static float fclamp(float x, float minVal, float maxVal)
{
    return fminf(fmaxf(x, minVal), maxVal);
}

// Compute the smoothstep function per the GLSL docs. They say that the
// results are undefined if edge0 >= edge1, but we allow edge0 > edge1.
static float smoothstep(float edge0, float edge1, float x)
{
    if (edge0 == edge1) {
        return 0;
    }

    float t = fclamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);

    return t*t*(3 - 2*t);
}

// Mixes between x and y according to a.
static float fmix(float x, float y, float a)
{
    return x*(1.0 - a) + y*a;
}

void Interpreter::stepGLSLstd450FClamp(const InsnGLSLstd450FClamp& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            float minVal = registerAs<float>(insn.minValId);
            float maxVal = registerAs<float>(insn.maxValId);
            registerAs<float>(insn.resultId) = fclamp(x, minVal, maxVal);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* minVal = &registerAs<float>(insn.minValId);
            float* maxVal = &registerAs<float>(insn.maxValId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = fclamp(x[i], minVal[i], maxVal[i]);
            }

        } else {

            std::cout << "Unknown type for FClamp\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FMix(const InsnGLSLstd450FMix& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = registerAs<float>(insn.xId);
            float y = registerAs<float>(insn.yId);
            float a = registerAs<float>(insn.aId);
            registerAs<float>(insn.resultId) = fmix(x, y, a);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* x = &registerAs<float>(insn.xId);
            float* y = &registerAs<float>(insn.yId);
            float* a = &registerAs<float>(insn.aId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = fmix(x[i], y[i], a[i]);
            }

        } else {

            std::cout << "Unknown type for FMix\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450SmoothStep(const InsnGLSLstd450SmoothStep& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float edge0 = registerAs<float>(insn.edge0Id);
            float edge1 = registerAs<float>(insn.edge1Id);
            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = smoothstep(edge0, edge1, x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* edge0 = &registerAs<float>(insn.edge0Id);
            float* edge1 = &registerAs<float>(insn.edge1Id);
            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = smoothstep(edge0[i], edge1[i], x[i]);
            }

        } else {

            std::cout << "Unknown type for SmoothStep\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Step(const InsnGLSLstd450Step& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float edge = registerAs<float>(insn.edgeId);
            float x = registerAs<float>(insn.xId);
            registerAs<float>(insn.resultId) = x < edge ? 0.0 : 1.0;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* edge = &registerAs<float>(insn.edgeId);
            float* x = &registerAs<float>(insn.xId);
            float* result = &registerAs<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = x[i] < edge[i] ? 0.0 : 1.0;
            }

        } else {

            std::cout << "Unknown type for Step\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Cross(const InsnGLSLstd450Cross& insn)
{
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
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Reflect(const InsnGLSLstd450Reflect& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float i = registerAs<float>(insn.iId);
            float n = registerAs<float>(insn.nId);

            float dot = n*i;

            registerAs<float>(insn.resultId) = i - 2.0*dot*n;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            float* i = &registerAs<float>(insn.iId);
            float* n = &registerAs<float>(insn.nId);
            float* result = &registerAs<float>(insn.resultId);

            float dot = dotProduct(n, i, type.count);

            for (int k = 0; k < type.count; k++) {
                result[k] = i[k] - 2.0*dot*n[k];
            }

        } else {

            std::cout << "Unknown type for Reflect\n";

        }
    }, pgm->types.at(registers[insn.iId].type));
}

void Interpreter::stepBranch(const InsnBranch& insn)
{
    pc = pgm->labels.at(insn.targetLabelId);
}

void Interpreter::stepBranchConditional(const InsnBranchConditional& insn)
{
    bool condition = registerAs<bool>(insn.conditionId);
    pc = pgm->labels.at(condition ? insn.trueLabelId : insn.falseLabelId);
}

void Interpreter::stepPhi(const InsnPhi& insn)
{
    Register& obj = registers[insn.resultId];
    uint32_t size = pgm->typeSizes.at(obj.type);

    bool found = false;
    for(int i = 0; !found && i < insn.operandId.size(); i += 2) {
        uint32_t srcId = insn.operandId[i];
        uint32_t parentId = insn.operandId[i + 1];

        if (parentId == previousBlockId) {
            const Register &src = registers[srcId];
            std::copy(src.data, src.data + size, obj.data);
            found = true;
        }
    }

    if (!found) {
        std::cout << "Error: Phi didn't find any label, previous " << previousBlockId
            << ", current " << currentBlockId << "\n";
        for(int i = 0; i < insn.operandId.size(); i += 2) {
            std::cout << "    " << insn.operandId[i + 1] << "\n";
        }
    }
}

void Interpreter::step()
{
    if(false) std::cout << "address " << pc << "\n";

    // Update our idea of what block we're in. If we just switched blocks,
    // remember the previous one (for Phi).
    if (pgm->blockId.at(pc) != currentBlockId) {
        // I'm not sure this is fully correct. For example, when returning
        // from a function this will set previousBlockId to a block in
        // the called function, but that's not right, it should always
        // point to a block within the current function. I don't think that
        // matters in practice because of the restricted locations where Phi
        // is placed.
        if(false) {
            std::cout << "Previous " << previousBlockId << ", current "
                << currentBlockId << ", new " << pgm->blockId.at(pc) << "\n";
        }
        previousBlockId = currentBlockId;
        currentBlockId = pgm->blockId.at(pc);
    }

    Instruction *insn = pgm->code.at(pc++);

    insn->step(this);
}

template <class T>
void Interpreter::set(SpvStorageClass clss, size_t offset, const T& v)
{
    objectInClassAt<T>(clss, offset) = v;
}

template <class T>
void Interpreter::get(SpvStorageClass clss, size_t offset, T& v)
{
    v = objectInClassAt<T>(clss, offset);
}

void Interpreter::run()
{
    currentBlockId = NO_BLOCK_ID;
    previousBlockId = NO_BLOCK_ID;

    // Copy constants to memory. They're treated like variables.
    for(auto c: pgm->constants) {
        registers[c.first] = c.second;
    }

    // init Function variables with initializers before each invocation
    // XXX also need to initialize within function calls?
    for(auto v: pgm->variables) {
        const Variable& var = v.second;
        pointers[v.first] = Pointer { var.type, var.storageClass, var.offset };
        if(v.second.storageClass == SpvStorageClassFunction) {
            assert(v.second.initializer == NO_INITIALIZER); // XXX will do initializers later
        }
    }

    callstack.push_back(EXECUTION_ENDED); // caller PC
    callstack.push_back(NO_RETURN_REGISTER); // return register
    pc = pgm->mainFunction->start;

    do {
        step();
    } while(pc != EXECUTION_ENDED);
}

// -----------------------------------------------------------------------------------

void eval(Interpreter &interpreter, float x, float y, v4float& color)
{
    interpreter.clearPrivateVariables();
    interpreter.set(SpvStorageClassInput, 0, v2float {x, y}); // fragCoord is in #0 in preamble
    interpreter.run();
    interpreter.get(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
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
    printf("\t-f S E    Render frames S through and including E\n");
    printf("\t-v        Print opcodes as they are parsed\n");
    printf("\t-g        Generate debugging information\n");
    printf("\t-O        Run optimizing passes\n");
    printf("\t-t        Throw an exception on first unimplemented opcode\n");
    printf("\t-n        Compile and load shader, but do not shade an image\n");
}

// Number of rows still left to shade (for progress report).
static std::atomic_int rowsLeft;

// Render rows starting at "startRow" every "skip".
void render(const Program *pgm, int startRow, int skip, float when)
{
    Interpreter interpreter(pgm);

    // iResolution is uniform @0 in preamble
    interpreter.set(SpvStorageClassUniform, 0, v2float {imageWidth, imageHeight});

    // iTime is uniform @8 in preamble
    interpreter.set(SpvStorageClassUniform, 8, when);

    // iMouse is uniform @16 in preamble, but we don't align properly, so ours is at 12.
    interpreter.set(SpvStorageClassUniform, 12, v4float {0, 0, 0, 0});

    for(int y = startRow; y < imageHeight; y += skip) {
        for(int x = 0; x < imageWidth; x++) {
            v4float color;
            eval(interpreter, x + 0.5f, y + 0.5f, color);
            for(int c = 0; c < 3; c++) {
                // ShaderToy clamps the color.
                int byteColor = color[c]*255.99;
                if (byteColor < 0) byteColor = 0;
                if (byteColor > 255) byteColor = 255;
                imageBuffer[imageHeight - 1 - y][x][c] = byteColor;
            }
        }

        rowsLeft--;
    }
}

// Thread to show progress to the user.
void showProgress(int totalRows, std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    while(true) {
        int left = rowsLeft;
        if (left == 0) {
            break;
        }

        std::cout << left << " rows left of " << totalRows;

        // Estimate time left.
        if (left != totalRows) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedTime = now - startTime;
            auto elapsedSeconds = double(elapsedTime.count())*
                std::chrono::steady_clock::period::num/
                std::chrono::steady_clock::period::den;
            auto secondsLeft = elapsedSeconds*left/(totalRows - left);

            std::cout << " (" << int(secondsLeft) << " seconds left)   ";
        }
        std::cout << "\r";
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool optimize = false;
    bool beVerbose = false;
    bool throwOnUnimplemented = false;
    bool doNotShade = false;
    int imageStart = 0, imageEnd = 0;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-g") == 0) {

            debug = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-f") == 0) {

            if(argc < 3) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            imageStart = atoi(argv[1]);
            imageEnd = atoi(argv[2]);
            argv += 3; argc -= 3;

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

    Program pgm(throwOnUnimplemented, beVerbose);
    spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_3);
    spvBinaryParse(context, &pgm, spirv.data(), spirv.size(), Program::handleHeader, Program::handleInstruction, nullptr);

    if (pgm.hasUnimplemented) {
        exit(1);
    }

    pgm.postParse();

    if (doNotShade) {
        exit(EXIT_SUCCESS);
    }

    int threadCount = std::thread::hardware_concurrency();
    std::cout << "Using " << threadCount << " threads.\n";

    for(int imageNumber = imageStart; imageNumber <= imageEnd; imageNumber++) {

        auto startTime = std::chrono::steady_clock::now();

        // Workers decrement rowsLeft at the end of each row.
        rowsLeft = imageHeight;

        std::vector<std::thread *> thread;

        // Generate the rows on multiple threads.
        for (int t = 0; t < threadCount; t++) {
            thread.push_back(new std::thread(render, &pgm, t, threadCount, imageNumber / 60.0));
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Progress information.
        thread.push_back(new std::thread(showProgress, imageHeight, startTime));

        // Wait for worker threads to quit.
        for (int t = 0; t < thread.size(); t++) {
            std::thread* td = thread.back();
            thread.pop_back();
            td->join();
        }

        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = endTime - startTime;
        auto elapsedSeconds = double(elapsedTime.count())*
            std::chrono::steady_clock::period::num/
            std::chrono::steady_clock::period::den;
        std::cout << "Shading took " << elapsedSeconds << " seconds ("
            << long(imageWidth*imageHeight/elapsedSeconds) << " pixels per second)\n";

        std::ostringstream ss;
        ss << "image" << std::setfill('0') << std::setw(4) << imageNumber << std::setw(0) << ".ppm";
        std::ofstream imageFile(ss.str(), std::ios::out | std::ios::binary);
        imageFile << "P6 " << imageWidth << " " << imageHeight << " 255\n";
        imageFile.write(reinterpret_cast<const char *>(imageBuffer), sizeof(imageBuffer));
        imageFile.close();
    }

    exit(EXIT_SUCCESS);
}
