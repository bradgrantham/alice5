
#include "program.h"

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
            pgm->types[id] = std::make_shared<TypeVoid>();
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
            pgm->types[id] = std::make_shared<TypeBool>();
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
            pgm->types[id] = std::make_shared<TypeFloat>(width);
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
            pgm->types[id] = std::make_shared<TypeInt>(width, signedness);
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
            pgm->types[id] = std::make_shared<TypeFunction>(returnType, params);
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
            pgm->types[id] = std::make_shared<TypeVector>(pgm->types[type], type, count);
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
            pgm->types[id] = std::make_shared<TypeArray>(pgm->types[type], type, count);
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
            pgm->types[id] = std::make_shared<TypeMatrix>(pgm->types[columnType], columnType, columnCount);
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
            pgm->types[id] = std::make_shared<TypePointer>(pgm->types[type], type, storageClass);
            pgm->typeSizes[id] = sizeof(uint32_t);
            if(pgm->verbose) {
                std::cout << "TypePointer " << id << " class " << storageClass << " type " << type << "\n";
            }
            break;
        }

        case SpvOpTypeStruct: {
            // XXX result id
            uint32_t id = nextu();
            std::vector<uint32_t> memberTypeIds = restv();
            std::vector<std::shared_ptr<Type>> memberTypes;
            size_t size = 0;
            for(auto& t: memberTypeIds) {
                memberTypes.push_back(pgm->types[t]);
                size += pgm->typeSizes[t];
            }
            pgm->types[id] = std::make_shared<TypeStruct>(memberTypes, memberTypeIds);
            pgm->typeSizes[id] = size;
            if(pgm->verbose) {
                std::cout << "TypeStruct " << id;
                if(memberTypeIds.size() > 0) {
                    std::cout << " members"; 
                    for(auto& i: memberTypeIds)
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
            uint32_t pointedType = pgm->type<TypePointer>(type)->type;
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

            do {
                // Treat like a constant.
                Register& r = pgm->allocConstantObject(id, typeId);
                std::fill(r.data, r.data + pgm->typeSizes[typeId], 0);
                // Arguably here we should set r.initialized to false, so we
                // can figure out if it's ever used, but just the fact that
                // it's defined here means it'll be used somewhere later.
                if(pgm->verbose) {
                    std::cout << "Undef " << id << " type " << typeId << "\n";
                }

                // We should fill out the subelements vector with pointers to
                // members of the subtype, but we don't necessarily have any
                // constants or undef of the subtype. So just make some up.
                const TypeVector *typeVector = pgm->getTypeAsVector(typeId);
                if (typeVector != nullptr) {
                    // Loop to create sub-elements.
                    typeId = typeVector->type;
                    id = pgm->nextReg++;
                    r.subelements = std::vector<uint32_t>(typeVector->count, id);
                } else {
                    // Done.
                    typeId = 0;
                }
            } while (typeId != 0);
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
            r.subelements = operands;
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
            pgm->types[id] = std::make_shared<TypeSampledImage>(imageType);
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
            pgm->types[id] = std::make_shared<TypeImage>(sampledType, dim, depth, arrayed,
                    ms, sampled, imageFormat, accessQualifier);
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
            string name = pgm->names.at(id);
            std::shared_ptr<Function> function = std::make_shared<Function>(id, name,
                    resultType, functionControl, functionType);
            pgm->functions[id] = function;
            pgm->currentFunction = function;
            pgm->currentBlock.reset();
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

            // Must be inside a function.
            assert(pgm->currentFunction);

            // Make new block for this label.
            std::shared_ptr<Block> block = std::make_shared<Block>(id, pgm->currentFunction.get());
            pgm->currentFunction->blocks[id] = block;
            pgm->currentBlock = block;

            // The first label we run into after a function definition is its start block.
            if(pgm->currentFunction->startBlockId == NO_BLOCK_ID) {
                pgm->currentFunction->startBlockId = id;
            }
            if(pgm->verbose) {
                std::cout << "Label " << id << "\n";
            }
            break;
        }

        case SpvOpFunctionEnd: {
            pgm->currentFunction.reset();
            pgm->currentBlock.reset();
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
