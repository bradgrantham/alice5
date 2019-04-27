#include <iomanip>

#include "program.h"
#include "risc-v.h"


std::map<uint32_t, std::string> OpcodeToString = {
#include "opcode_to_string.h"
};

std::map<uint32_t, std::string> GLSLstd450OpcodeToString = {
#include "GLSLstd450_opcode_to_string.h"
};

Program::Program(bool throwOnUnimplemented_, bool verbose_) :
    throwOnUnimplemented(throwOnUnimplemented_),
    hasUnimplemented(false),
    verbose(verbose_),
    currentFunction(nullptr)
{
    memorySize = 0;
    auto anotherRegion = [this](size_t size){MemoryRegion r(memorySize, size); memorySize += size; return r;};
    memoryRegions[SpvStorageClassUniformConstant] = anotherRegion(1024);
    memoryRegions[SpvStorageClassInput] = anotherRegion(1024);
    memoryRegions[SpvStorageClassUniform] = anotherRegion(4096);
    memoryRegions[SpvStorageClassOutput] = anotherRegion(1024);
    memoryRegions[SpvStorageClassWorkgroup] = anotherRegion(0);
    memoryRegions[SpvStorageClassCrossWorkgroup] = anotherRegion(0);
    memoryRegions[SpvStorageClassPrivate] = anotherRegion(16384); // XXX still needed?
    memoryRegions[SpvStorageClassFunction] = anotherRegion(16384);
    memoryRegions[SpvStorageClassGeneric] = anotherRegion(0);
    memoryRegions[SpvStorageClassPushConstant] = anotherRegion(0);
    memoryRegions[SpvStorageClassAtomicCounter] = anotherRegion(0);
    memoryRegions[SpvStorageClassImage] = anotherRegion(0);
    memoryRegions[SpvStorageClassStorageBuffer] = anotherRegion(0);
}

spv_result_t Program::handleHeader(void* user_data, spv_endianness_t endian,
                               uint32_t /* magic */, uint32_t version,
                               uint32_t generator, uint32_t id_bound,
                               uint32_t schema)
{
    // auto pgm = static_cast<Program*>(user_data);
    return SPV_SUCCESS;
}

spv_result_t Program::handleInstruction(void* user_data, const spv_parsed_instruction_t* insn)
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
            pgm->decorations[id][decoration] = operands;
            if(pgm->verbose) {
                std::cout << "OpDecorate " << id << " " << decoration;
                for(auto& i: pgm->decorations[id][decoration])
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
            pgm->memberDecorations[id][member][decoration] = operands;
            if(pgm->verbose) {
                std::cout << "OpMemberDecorate " << id << " " << member << " " << decoration;
                for(auto& i: pgm->memberDecorations[id][member][decoration])
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

        case SpvOpLine: {
            // XXX result id
            uint32_t fileId = nextu();
            uint32_t line = nextu();
            uint32_t column = nextu();
            pgm->currentLine = LineInfo {fileId, line, column};
            if(pgm->verbose) {
                std::cout << "Line " << fileId << " (\"" << pgm->strings[fileId] << "\") line " << line << " column " << column << "\n";
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

        case SpvOpTypeArray: {
            // XXX result id
            uint32_t id = nextu();
            uint32_t type = nextu();
            uint32_t lengthId = nextu();
            const Register& length = pgm->constants.at(lengthId);
            uint32_t count = *reinterpret_cast<uint32_t*>(length.data);
            pgm->types[id] = TypeArray {type, count};
            pgm->typeSizes[id] = pgm->typeSizes[type] * count;
            if(pgm->verbose) {
                std::cout << "TypeArray " << id << " of " << type << " count " << count << " (from register " << lengthId << ")\n";
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
            pgm->variables[id] = {pointedType, storageClass, initializer, 0xFFFFFFFF};
            if(pgm->verbose) {
                std::cout << "Variable " << id << " type " << type << " to type " << pointedType << " storageClass " << storageClass;
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

        case SpvOpUndef: {
            uint32_t typeId = nextu();
            uint32_t id = nextu();
            // Treat like a constant.
            Register& r = pgm->allocConstantObject(id, typeId);
            std::fill(r.data, r.data + pgm->typeSizes[typeId], 0);
            // Arguably here we should set r.initialized to false, so we
            // can figure out if it's ever used, but just the fact that
            // it's defined here means it'll be used somewhere later.
            if(pgm->verbose) {
                std::cout << "Undef " << id << " type " << typeId << "\n";
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
            pgm->typeSizes[id] = sizeof(uint32_t); // binding
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
            pgm->typeSizes[id] = sizeof(uint32_t); // XXX ???
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
            uint32_t functionControl = nextu(); // bitfield hints for inlining, pure, etc.
            uint32_t functionType = nextu();
            uint32_t start = pgm->instructions.size();
            pgm->functions[id] = Function {id, resultType, functionControl, functionType, start , NO_BLOCK_ID};
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
            pgm->labels[id] = pgm->instructions.size();
            // The first label we run into after a function definition is its start block.
            if(pgm->currentFunction->labelId == NO_BLOCK_ID) {
                pgm->currentFunction->labelId = id;
            }
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

// Return the scalar for the vector register's index.
uint32_t Program::scalarize(uint32_t vreg, int i, uint32_t subtype, uint32_t scalarReg) {
    auto regIndex = RegIndex{vreg, i};
    auto itr = vec2scalar.find(regIndex);
    if (itr == vec2scalar.end()) {
        if (scalarReg == 0) {
            scalarReg = nextReg++;
        }
        vec2scalar[regIndex] = scalarReg;

        // See if this was a constant vector.
        auto itr = constants.find(vreg);
        if (itr != constants.end()) {
            // Extract new constant for this component.
            Register &r = allocConstantObject(scalarReg, subtype);
            auto [subtype2, offset] = getConstituentInfo(itr->second.type, i);
            uint32_t size = typeSizes[subtype];
            assert(subtype == subtype2);
            std::copy(itr->second.data + offset, itr->second.data + offset + size, r.data);
        } else {
            // Not a constant, must be a variable.
            this->resultTypes[scalarReg] = subtype;
        }
    } else {
        if (scalarReg != 0) {
            assert(scalarReg == itr->second);
        } else {
            scalarReg = itr->second;
        }
    }

    return scalarReg;
}

// Post-parsing work.
void Program::postParse() {
    // Convert vector instructions to scalar instructions.
    expandVectors(instructions, instructions);

    // Find the main function.
    mainFunction = nullptr;
    for(auto& e: entryPoints) {
        if(e.second.name == "main") {
            mainFunction = &functions[e.first];
        }
    }

    // Allocated variables 
    for(auto& [id, var]: variables) {
        if(var.storageClass == SpvStorageClassUniformConstant) {
            uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
            var.address = memoryRegions[SpvStorageClassUniformConstant].base + binding * 16; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);

        } else if(var.storageClass == SpvStorageClassUniform) {
            uint32_t binding = decorations.at(id).at(SpvDecorationBinding)[0];
            var.address = memoryRegions[SpvStorageClassUniform].base + binding * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else if(var.storageClass == SpvStorageClassOutput) {
            uint32_t location;
            if(decorations.at(id).find(SpvDecorationLocation) == decorations.at(id).end()) {
                throw std::runtime_error("no Location decoration available for output " + std::to_string(id));
            }

            location = decorations.at(id).at(SpvDecorationLocation)[0];
            var.address = memoryRegions[SpvStorageClassOutput].base + location * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else if(var.storageClass == SpvStorageClassInput) {
            uint32_t location;
            if(names[id] == "gl_FragCoord") {
                location = 0;
            } else if(names[id].find("gl_") == 0) {
                throw std::runtime_error("builtin input variable " + names[id] + " specified with no location available");
            } else {
                if(decorations.find(id) == decorations.end()) {
                    throw std::runtime_error("no decorations available for input " + std::to_string(id) + ", name \"" + names[id] + "\"");
                }
                if(decorations.at(id).find(SpvDecorationLocation) == decorations.at(id).end()) {
                    throw std::runtime_error("no Location decoration available for input " + std::to_string(id));
                }

                location = decorations.at(id).at(SpvDecorationLocation)[0];
            }
            var.address = memoryRegions[SpvStorageClassInput].base + location * 256; // XXX magic number
            storeNamedVariableInfo(names[id], var.type, var.address);
        } else {
            var.address = allocate(var.storageClass, var.type);
        }
    }
    if(verbose) {
        std::cout << "----------------------- Variable binding and location info\n";
        for(auto& [name, info]: namedVariables) {
            std::cout << "variable " << name << " is at " << info.address << '\n';
        }
    }

    // Figure out our basic blocks. These start on an OpLabel and end on
    // a terminating instruction.
    for (auto [labelId, codeIndex] : labels) {
        bool found = false;
        for (int i = codeIndex; i < instructions.size(); i++) {
            if (instructions[i]->isTermination()) {
                blocks[labelId] = std::make_unique<Block>(labelId, codeIndex, uint32_t(i + 1));
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "Error: Terminating instruction for label "
                << labelId << " not found.\n";
            exit(EXIT_FAILURE);
        }
    }

    // Compute successor blocks.
    for (auto& [labelId, block] : blocks) {
        Instruction *instruction = instructions[block->end - 1].get();
        assert(instruction->isTermination());
        block->succ = instruction->targetLabelIds;
        for (uint32_t labelId : block->succ) {
            blocks[labelId]->pred.insert(block->labelId);
        }
    }

    // Record the block ID for each instruction. Note a problem
    // here is that the OpFunctionParameter instruction gets put into the
    // block at the end of the previous function. I don't think this
    // matters in practice because there's never a Phi at the top of a
    // function.
    for (auto& [labelId, block] : blocks) {
        for (int pc = block->begin; pc < block->end; pc++) {
            instructions[pc]->blockId = block->labelId;
        }
    }

    // Compute predecessor and successor instructions for each instruction.
    // For most instructions this is just the previous or next line, except
    // for the ones at the beginning or end of each block.
    for (auto& [labelId, block] : blocks) {
        for (auto predBlockId : block->pred) {
            instructions[block->begin]->pred.insert(blocks[predBlockId]->end - 1);
        }
        if (block->begin != block->end - 1) {
            instructions[block->begin]->succ.insert(block->begin + 1);
        }
        for (int pc = block->begin + 1; pc < block->end - 1; pc++) {
            instructions[pc]->pred.insert(pc - 1);
            instructions[pc]->succ.insert(pc + 1);
        }
        if (block->begin != block->end - 1) {
            instructions[block->end - 1]->pred.insert(block->end - 2);
        }
        for (auto succBlockId : block->succ) {
            instructions[block->end - 1]->succ.insert(blocks[succBlockId]->begin);
        }
    }

    // Compute livein and liveout registers for each line.
    Timer timer;
    std::set<uint32_t> pc_worklist; // PCs left to work on.
    for (int pc = 0; pc < instructions.size(); pc++) {
        pc_worklist.insert(pc);
    }
    while (!pc_worklist.empty()) {
        // Pick any PC to start with. Starting at the end is a bit more efficient
        // since our predecessor depends on us.
        auto itr = pc_worklist.rbegin();
        uint32_t pc = *itr;
        pc_worklist.erase(*itr);

        Instruction *instruction = instructions[pc].get();
        std::set<uint32_t> oldLivein = instruction->livein;
        std::set<uint32_t> oldLiveout = instruction->liveout;

        instruction->liveout.clear();
        for (uint32_t succPc : instruction->succ) {
            Instruction *succInstruction = instructions[succPc].get();
            instruction->liveout.insert(succInstruction->livein.begin(),
                    succInstruction->livein.end());
        }

        instruction->livein = instruction->liveout;
        if (instruction->resId != NO_REGISTER) {
            instruction->livein.erase(instruction->resId);
        }
        for (uint32_t argId : instruction->argIds) {
            // Don't consider constants or variables, they're never in registers.
            if (/*constants.find(argId) == constants.end() &&*/
                    variables.find(argId) == variables.end()) {

                instruction->livein.insert(argId);
            }
        }

        if (oldLivein != instruction->livein || oldLiveout != instruction->liveout) {
            // Our predecessors depend on us.
            for (uint32_t predPc : instruction->pred) {
                pc_worklist.insert(predPc);
            }
        }
    }
    std::cerr << "Livein and liveout took " << timer.elapsed() << " seconds.\n";

    // Compute the dominance tree for blocks. Use a worklist. Do all blocks
    // simultaneously (across functions).
    timer.reset();
    std::vector<uint32_t> worklist; // block IDs.
    // Make set of all label IDs.
    std::set<uint32_t> allLabelIds;
    for (auto& [labelId, block] : blocks) {
        allLabelIds.insert(labelId);
    }
    std::set<uint32_t> unreached = allLabelIds;
    // Prepare every block.
    for (auto& [labelId, block] : blocks) {
        block->dom = allLabelIds;
    }
    // Insert every function's start block.
    for (auto& [_, function] : functions) {
        assert(function.labelId != NO_BLOCK_ID);
        worklist.push_back(function.labelId);
    }
    while (!worklist.empty()) {
        // Take any item from worklist.
        uint32_t labelId = worklist.back();
        worklist.pop_back();

        // We can reach this from the start node.
        unreached.erase(labelId);

        Block *block = blocks.at(labelId).get();

        // Intersection of all predecessor doms.
        std::set<uint32_t> dom;
        bool first = true;
        for (auto predBlockId : block->pred) {
            Block *pred = blocks.at(predBlockId).get();

            if (first) {
                dom = pred->dom;
                first = false;
            } else {
                // I love C++.
                std::set<uint32_t> intersection;
                std::set_intersection(dom.begin(), dom.end(),
                        pred->dom.begin(), pred->dom.end(),
                        std::inserter(intersection, intersection.begin()));
                std::swap(intersection, dom);
            }
        }
        // Add ourselves.
        dom.insert(labelId);

        // If the set changed, update it and put the
        // successors in the work queue.
        if (dom != block->dom) {
            block->dom = dom;
            worklist.insert(worklist.end(), block->succ.begin(), block->succ.end());
        }
    }
    std::cerr << "Dominance tree took " << timer.elapsed() << " seconds.\n";

    // Compute immediate dom for each block.
    for (auto& [labelId, block] : blocks) {
        block->idom = NO_BLOCK_ID;

        // Try each block in the block's dom.
        for (auto idomCandidate : block->dom) {
            bool valid = idomCandidate != labelId;

            // Can't be the idom if it's dominated by another dom.
            for (auto otherDom : block->dom) {
                if (otherDom != idomCandidate &&
                        otherDom != labelId &&
                        blocks[otherDom]->isDominatedBy(idomCandidate)) {

                    valid = false;
                    break;
                }
            }

            if (valid) {
                block->idom = idomCandidate;
                break;
            }
        }
    }

    // Dump block info.
    if (verbose) {
        std::cout << "----------------------- Block info\n";
        for (auto& [labelId, block] : blocks) {
            std::cout << "Block " << labelId << ", instructions "
                << block->begin << "-" << block->end << ":\n";
            std::cout << "    Pred:";
            for (auto labelId : block->pred) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            std::cout << "    Succ:";
            for (auto labelId : block->succ) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            std::cout << "    Dom:";
            for (auto labelId : block->dom) {
                std::cout << " " << labelId;
            }
            std::cout << "\n";
            if (block->idom != NO_BLOCK_ID) {
                std::cout << "    Immediate Dom: " << block->idom << "\n";
            }
        }
        std::cout << "-----------------------\n";
        // http://www.webgraphviz.com/
        std::cout << "digraph CFG {\n  rankdir=TB;\n";
        for (auto& [labelId, block] : blocks) {
            if (unreached.find(labelId) == unreached.end()) {
                for (auto pred : block->pred) {
                    // XXX this is laid out much better if the function's start
                    // block is mentioned first.
                    if (unreached.find(pred) == unreached.end()) {
                        std::cout << "  \"" << pred << "\" -> \"" << labelId << "\"";
                        std::cout << ";\n";
                    }
                }
                if (block->idom != NO_BLOCK_ID) {
                    std::cout << "  \"" << labelId << "\" -> \"" << block->idom << "\"";
                    std::cout << " [color=\"0.000, 0.999, 0.999\"]";
                    std::cout << ";\n";
                }
            }
        }
        std::cout << "}\n";
        std::cout << "-----------------------\n";
    }

    // Dump instruction info.
    if (verbose) {
        std::cout << "----------------------- Instruction info\n";
        for (int pc = 0; pc < instructions.size(); pc++) {
            Instruction *instruction = instructions[pc].get();

            for(auto &function : functions) {
                if(pc == function.second.start) {
                    std::string name = cleanUpFunctionName(function.first);
                    std::cout << name << ":\n";
                }
            }

            for(auto &label : labels) {
                if(pc == label.second) {
                    std::cout << label.first << ":\n";
                }
            }

            std::ios oldState(nullptr);
            oldState.copyfmt(std::cout);

            std::cout
                << std::setw(5) << pc;
            if (instruction->blockId == NO_BLOCK_ID) {
                std::cout << "  ---";
            } else {
                std::cout << std::setw(5) << instruction->blockId;
            }
            if (instruction->resId == NO_REGISTER) {
                std::cout << "        ";
            } else {
                std::cout << std::setw(5) << instruction->resId << " <-";
            }

            std::cout << std::setw(0) << " " << instruction->name();

            for (auto argId : instruction->argIds) {
                std::cout << " " << argId;
            }

            std::cout << " (pred";
            for (auto line : instruction->pred) {
                std::cout << " " << line;
            }
            std::cout << ", succ";
            for (auto line : instruction->succ) {
                std::cout << " " << line;
            }
            std::cout << ", live";
            for (auto regId : instruction->livein) {
                std::cout << " " << regId;
            }

            std::cout << ")\n";

            std::cout.copyfmt(oldState);
        }
        std::cout << "-----------------------\n";
    }

    for (auto& [labelId, block] : blocks) {
        // It's okay to have no idom as long as you're the first block in the function.
        if ((block->idom == NO_BLOCK_ID) != block->pred.empty()) {
            std::cout << "----\n"
                << labelId << "\n"
                << block->idom << "\n"
                << block->pred.size() << "\n";
        }
        assert((block->idom == NO_BLOCK_ID) == block->pred.empty());
    }
}

void Program::expandVectors(const InstructionList &inList, InstructionList &outList) {
    InstructionList newList;

    /* XXX delete
    // Expand variables.
    std::map<uint32_t, Variable> newVariables;
    for (auto &[id, var] : variables) {
        const TypeVector *typeVector = getTypeAsVector(var.type);
        if (typeVector != nullptr) {
            assert(var.initializer == NO_INITIALIZER);
            for (int i = 0; i < typeVector->count; i++) {
                auto [subtype, offset] = getConstituentInfo(var.type, i);
                uint32_t scalarReg = scalarize(id, i, subtype);
                newVariables[scalarReg] = {
                    subtype, var.storageClass, NO_INITIALIZER, 0xFFFFFFFF
                };
                auto itr = names.find(id);
                if (itr != names.end()) {
                    names[scalarReg] = itr->second;
                }
            }
        }
    }
    for (auto &[id, var] : newVariables) {
        variables[id] = var;
    }
    */

    for (uint32_t pc = 0; pc < inList.size(); pc++) {
        bool replaced = false;
        const std::shared_ptr<Instruction> &instructionPtr = inList[pc];
        Instruction *instruction = instructionPtr.get();

        switch (instruction->opcode()) {
            case SpvOpLoad: {
                InsnLoad *insn = dynamic_cast<InsnLoad *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(
                        resultTypes.at(insn->resultId));
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(insn->type, i);
                        newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                    subtype,
                                    scalarize(insn->resultId, i, subtype),
                                    insn->pointerId,
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVLoad>(insn->lineInfo,
                                insn->type,
                                insn->resultId,
                                insn->pointerId,
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpStore: {
                InsnStore *insn = dynamic_cast<InsnStore *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(
                        resultTypes.at(insn->objectId));
                if (typeVector != nullptr) {
                    for (int i = 0; i < typeVector->count; i++) {
                        auto [subtype, offset] = getConstituentInfo(
                                resultTypes.at(insn->objectId), i);
                        newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                    insn->pointerId,
                                    scalarize(insn->objectId, i, subtype),
                                    insn->memoryAccess,
                                    i*typeSizes.at(subtype)));
                    }
                } else {
                    newList.push_back(std::make_shared<RiscVStore>(insn->lineInfo,
                                insn->pointerId,
                                insn->objectId,
                                insn->memoryAccess,
                                0));
                }
                replaced = true;
                break;
            }

            case SpvOpCompositeConstruct: {
                InsnCompositeConstruct *insn =
                    dynamic_cast<InsnCompositeConstruct *>(instruction);
                const TypeVector *typeVector = getTypeAsVector(insn->type);
                assert(typeVector != nullptr);
                for (int i = 0; i < typeVector->count; i++) {
                    auto [subtype, offset] = getConstituentInfo(insn->type, i);
                    // Re-use existing SSA register.
                    scalarize(insn->resultId, i, subtype, insn->constituentsId[i]);
                }
                replaced = true;
                break;
            }

            case SpvOpFMul:
                 expandVectorsBinOp<InsnFMul>(instruction, newList, replaced);
                 break;

            case SpvOpFDiv:
                 expandVectorsBinOp<InsnFDiv>(instruction, newList, replaced);
                 break;

            default: {
                std::cerr << "Warning: Unhandled opcode "
                    << OpcodeToString.at(instruction->opcode())
                    << " when expanding vectors.\n";
                break;
            }
        }

        if (!replaced) {
            newList.push_back(instructionPtr);
        }
    }

    std::swap(newList, outList);
}

