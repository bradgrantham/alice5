#include <iostream>
#include <vector>
#include <functional>
#include <set>
#include <cstdio>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "risc-v.h"
#include "image.h"
#include "interpreter.h"
#include "program.h"
#include "interpreter_tmpl.h"
#include "shadertoy.h"
#include "timer.h"
#include "compiler.h"

#define DEFAULT_WIDTH (640/2)
#define DEFAULT_HEIGHT (360/2)

// Enable this to check if our virtual RAM is being initialized properly.
#define CHECK_MEMORY_ACCESS
// Enable this to check if our virtual registers are being initialized properly.
#define CHECK_REGISTER_ACCESS

// -----------------------------------------------------------------------------------

void RiscVPhi::emit(Compiler *compiler)
{
    compiler->emit("", "our phi instruction, nothing to do.");
}

void RiscVAddi::emit(Compiler *compiler)
{
    compiler->emitBinaryImmOp("addi", resultId(), rs1, imm);
}

void RiscVLoad::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(resultId())) {
        ss1 << "flw ";
    } else {
        ss1 << "lw ";
    }
    ss1 << compiler->reg(resultId()) << ", ";
    auto r = compiler->asRegister(pointerId);
    if (r == nullptr) {
        // It's a variable reference.
        ss1 << compiler->reg(pointerId);
        if (offset != 0) {
            ss1 << "+" << offset;
        }
        ss1 << "(x0)";
    } else {
        // It's a register reference.
        ss1 << offset << "(" << compiler->reg(pointerId) << ")";
    }
    std::ostringstream ss2;
    ss2 << "r" << resultId() << " = (r" << pointerId << ")";
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVStore::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(objectId)) {
        ss1 << "fsw ";
    } else {
        ss1 << "sw ";
    }
    ss1 << compiler->reg(objectId) << ", " << compiler->reg(pointerId);
    if (offset != 0) {
        ss1 << "+" << offset;
    }
    ss1 << "(x0)";
    std::ostringstream ss2;
    ss2 << "(r" << pointerId << ") = r" << objectId;
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVCross::emit(Compiler *compiler)
{
    compiler->emit("addi sp, sp, -28", "Make room on stack");
    compiler->emit("sw ra, 24(sp)", "Save return address");

    for (int i = argIdList.size() - 1; i >= 0; i--) {
        std::ostringstream ss;
        ss << "fsw " << compiler->reg(argIdList[i]) << ", " << (i*4) << "(sp)";
        compiler->emit(ss.str(), "Push parameter");
    }

    compiler->emit("jal ra, .cross", "Call routine");

    for (int i = 0; i < resIdList.size(); i++) {
        std::ostringstream ss;
        ss << "flw " << compiler->reg(resIdList[i]) << ", " << (i*4) << "(sp)";
        compiler->emit(ss.str(), "Pop result");
    }

    compiler->emit("lw ra, 12(sp)", "Restore return address");
    compiler->emit("addi sp, sp, 16", "Restore stack");
}

void RiscVMove::emit(Compiler *compiler)
{
    const CompilerRegister *r1 = compiler->asRegister(resultId());
    const CompilerRegister *r2 = compiler->asRegister(rs);
    assert(r1 != nullptr);
    assert(r2 != nullptr);

    std::ostringstream ss1;
    if (r1->phy != r2->phy) {
        if (compiler->isRegFloat(resultId())) {
            ss1 << "fsgnj.s ";
        } else {
            ss1 << "and ";
        }
        ss1 << compiler->reg(resultId()) << ", "
            << compiler->reg(rs) << ", "
            << compiler->reg(rs);
    }
    std::ostringstream ss2;
    ss2 << "r" << resultId() << " = r" << rs;
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVLength::emit(Compiler *compiler)
{
    size_t n = operandIds.size();
    assert(n <= 4);

    std::vector<uint32_t> resultIds;
    resultIds.push_back(resultId());

    std::ostringstream functionName;
    functionName << ".length" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

void RiscVNormalize::emit(Compiler *compiler)
{
    size_t n = operandIds.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".normalize" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

void RiscVDistance::emit(Compiler *compiler)
{
    size_t n = vector1Ids.size();
    assert(n <= 4);

    std::vector<uint32_t> resultIds;
    resultIds.push_back(resultId());

    std::vector<uint32_t> operandIds;
    for (auto id : vector1Ids) {
        operandIds.push_back(id);
    }
    for (auto id : vector2Ids) {
        operandIds.push_back(id);
    }

    std::ostringstream functionName;
    functionName << ".distance" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

void RiscVDot::emit(Compiler *compiler)
{
    size_t n = vector1Ids.size();
    assert(n <= 4);

    std::vector<uint32_t> resultIds;
    resultIds.push_back(resultId());

    std::vector<uint32_t> operandIds;
    for (auto id : vector1Ids) {
        operandIds.push_back(id);
    }
    for (auto id : vector2Ids) {
        operandIds.push_back(id);
    }

    std::ostringstream functionName;
    functionName << ".dot" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

void RiscVAll::emit(Compiler *compiler)
{
    size_t n = operandIds.size();
    assert(n <= 4);

    std::vector<uint32_t> resultIds;
    resultIds.push_back(resultId());

    std::ostringstream functionName;
    functionName << ".all" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

void RiscVAny::emit(Compiler *compiler)
{
    size_t n = operandIds.size();
    assert(n <= 4);

    std::vector<uint32_t> resultIds;
    resultIds.push_back(resultId());

    std::ostringstream functionName;
    functionName << ".any" << n;

    compiler->emitCall(functionName.str(), resultIds, operandIds);
}

// -----------------------------------------------------------------------------------

Instruction::Instruction(const LineInfo& lineInfo)
    : lineInfo(lineInfo),
      blockId(NO_BLOCK_ID)
{
    // Nothing.
}

void Instruction::emit(Compiler *compiler)
{
    compiler->emitNotImplemented(name());
}

void InsnFAdd::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fadd.s", resultId(), operand1Id(), operand2Id());
}

void InsnFSub::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fsub.s", resultId(), operand1Id(), operand2Id());
}

void InsnFMul::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fmul.s", resultId(), operand1Id(), operand2Id());
}

void InsnFDiv::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fdiv.s", resultId(), operand1Id(), operand2Id());
}

void InsnFMod::emit(Compiler *compiler)
{
    compiler->emitBinCall(".mod", resultId(), operand1Id(), operand2Id());
}

void InsnIAdd::emit(Compiler *compiler)
{
    uint32_t intValue;
    if (compiler->asIntegerConstant(operand1Id(), intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId(), operand2Id(), intValue);
    } else if (compiler->asIntegerConstant(operand2Id(), intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId(), operand1Id(), intValue);
    } else {
        compiler->emitBinaryOp("add", resultId(), operand1Id(), operand2Id());
    }
}

void InsnSLessThan::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("slt", resultId(), operand1Id(), operand2Id());
}

void InsnFunctionCall::emit(Compiler *compiler)
{
    compiler->emit("push pc", "");
    for(int i = operandIdCount() - 1; i >= 0; i--) {
        compiler->emit(std::string("push ") + compiler->reg(operandId(i)), "");
    }
    compiler->emit(std::string("call ") + compiler->pgm->cleanUpFunctionName(functionId), "");
    compiler->emit(std::string("pop ") + compiler->reg(resultId()), "");
}

void InsnFunctionParameter::emit(Compiler *compiler)
{
    compiler->emit(std::string("pop ") + compiler->reg(resultId()), "");
}

void InsnLoad::emit(Compiler *compiler)
{
    // We expand these to RiscVLoad.
    assert(false);
}

void InsnStore::emit(Compiler *compiler)
{
    // We expand these to RiscVStore.
    assert(false);
}

void InsnBranch::emit(Compiler *compiler)
{
    // See if we need to emit any copies for Phis at our target.
    /// XXX delete compiler->emitPhiCopy(this, targetLabelId);

    std::ostringstream ss;
    ss << "jal x0, label" << targetLabelId;
    compiler->emit(ss.str(), "");
}

void InsnReturn::emit(Compiler *compiler)
{
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnReturnValue::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    ss1 << "mov x10, " << compiler->reg(valueId());
    std::ostringstream ss2;
    ss2 << "return " << valueId();
    compiler->emit(ss1.str(), ss2.str());
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnPhi::emit(Compiler *compiler)
{
    compiler->emit("", "phi instruction, nothing to do.");
}

void InsnFOrdLessThanEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("lte", resultId(), operand1Id(), operand2Id());
}

void InsnBranchConditional::emit(Compiler *compiler)
{
    std::string localLabel = compiler->makeLocalLabel();

    std::ostringstream ss1;
    ss1 << "beq " << compiler->reg(conditionId())
        << ", x0, " << localLabel;
    std::ostringstream ssid;
    ssid << "r" << conditionId();
    compiler->emit(ss1.str(), ssid.str());
    // True path.
    /// XXX delete compiler->emitPhiCopy(this, trueLabelId);
    std::ostringstream ss2;
    ss2 << "jal x0, label" << trueLabelId;
    compiler->emit(ss2.str(), "");
    // False path.
    compiler->emitLabel(localLabel);
    /// XXX delete compiler->emitPhiCopy(this, falseLabelId);
    std::ostringstream ss3;
    ss3 << "jal x0, label" << falseLabelId;
    compiler->emit(ss3.str(), "");
}

void InsnAccessChain::emit(Compiler *compiler)
{
    uint32_t offset = 0;

    const Variable &variable = compiler->pgm->variables.at(baseId());
    uint32_t type = variable.type;
    for (int i = 0; i < indexesIdCount(); i++) {
        uint32_t id = indexesId(i);
        uint32_t intValue;
        if (compiler->asIntegerConstant(id, intValue)) {
            auto [subtype, subOffset] = compiler->pgm->getConstituentInfo(type, intValue);
            type = subtype;
            offset += subOffset;
        } else {
            std::cerr << "Error: Don't yet handle non-constant offsets in AccessChain ("
                << id << ").\n";
            exit(EXIT_FAILURE);
        }
    }

    auto name = compiler->pgm->names.find(baseId());
    if (name == compiler->pgm->names.end()) {
        std::cerr << "Error: Don't yet handle pointer reference in AccessChain ("
            << baseId() << ").\n";
        exit(EXIT_FAILURE);
    }

    std::ostringstream ss;
    ss << "addi " << compiler->reg(resultId()) << ", x0, "
        << compiler->notEmptyLabel(name->second) << "+" << offset;
    compiler->emit(ss.str(), "");
}

void InsnGLSLstd450Sin::emit(Compiler *compiler)
{
    compiler->emitUniCall(".sin", resultId(), xId());
}

void InsnGLSLstd450Cos::emit(Compiler *compiler)
{
    compiler->emitUniCall(".cos", resultId(), xId());
}

void InsnGLSLstd450Atan2::emit(Compiler *compiler)
{
    compiler->emitBinCall(".atan2", resultId(), yId(), xId());
}

void InsnGLSLstd450FAbs::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    ss1 << "fsgnjx.s "
        << compiler->reg(resultId()) << ", "
        << compiler->reg(xId()) << ", "
        << compiler->reg(xId());

    std::ostringstream ss2;
    ss2 << resultId() << " = fabs(" << xId() << ")";

    compiler->emit(ss1.str(), ss2.str());
}

void InsnGLSLstd450Fract::emit(Compiler *compiler)
{
    compiler->emitUniCall(".fract", resultId(), xId());
}

void InsnGLSLstd450Floor::emit(Compiler *compiler)
{
    compiler->emitUniCall(".floor", resultId(), xId());
}

void InsnGLSLstd450Step::emit(Compiler *compiler)
{
    compiler->emitBinCall(".step", resultId(), edgeId(), xId());
}

// -----------------------------------------------------------------------------------

// Returns whether successful.
bool compileProgram(const Program &pgm)
{
    Compiler compiler(&pgm);

    compiler.compile();

    return true;
}

// -----------------------------------------------------------------------------------

void eval(Interpreter &interpreter, float x, float y, v4float& color)
{
    interpreter.clearPrivateVariables();
    interpreter.set(SpvStorageClassInput, 0, v4float {x, y}); // gl_FragCoord is always #0 
    interpreter.set(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
    interpreter.run();
    interpreter.get(SpvStorageClassOutput, 0, color); // color is out #0 in preamble
}


std::string readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!shaderFile.good()) {
        throw std::runtime_error("couldn't open file " + shaderFileName + " for reading");
    }
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
    printf("\t-f S E    Render frames S through and including E [0 0]\n");
    printf("\t-d W H    Render frame at size W by H [%d %d]\n",
            int(DEFAULT_WIDTH), int(DEFAULT_HEIGHT));
    printf("\t-j N      Use N threads [%d]\n",
            int(std::thread::hardware_concurrency()));
    printf("\t-v        Print opcodes as they are parsed\n");
    printf("\t-g        Generate debugging information\n");
    printf("\t-O        Run optimizing passes\n");
    printf("\t-t        Throw an exception on first unimplemented opcode\n");
    printf("\t-n        Compile and load shader, but do not shade an image\n");
    printf("\t-S        show the disassembly of the SPIR-V code\n");
    printf("\t-c        compile to our own ISA\n");
    printf("\t-s        convert vector operations to scalar operations\n");
    printf("\t--json    input file is a ShaderToy JSON file\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
}

const std::string shaderPreambleFilename = "preamble.frag";
const std::string shaderEpilogueFilename = "epilogue.frag";

// Number of rows still left to shade (for progress report).
static std::atomic_int rowsLeft;

// Render rows starting at "startRow" every "skip".
void render(ShaderToyRenderPass* pass, int startRow, int skip, int frameNumber, float when)
{
    Interpreter interpreter(&pass->pgm);
    ImagePtr output = pass->outputs[0].sampledImage.image;

    interpreter.set("iResolution", v2float {static_cast<float>(output->width), static_cast<float>(output->height)});

    interpreter.set("iFrame", frameNumber);

    interpreter.set("iTime", when);

    interpreter.set("iMouse", v4float {0, 0, 0, 0});

    for(int i = 0; i < pass->inputs.size(); i++) {
        auto& input = pass->inputs[i];
        interpreter.set("iChannel" + std::to_string(input.channelNumber), i);
        ImagePtr image = input.sampledImage.image;
        float w = static_cast<float>(image->width);
        float h = static_cast<float>(image->height);
        interpreter.set("iChannelResolution[" + std::to_string(input.channelNumber) + "]", v3float{w, h, 0});
    }

    // This loop acts like a rasterizer fixed function block.  Maybe it should
    // set inputs and read outputs also.
    for(int y = startRow; y < output->height; y += skip) {
        for(int x = 0; x < output->width; x++) {
            v4float color;
            output->get(x, output->height - 1 - y, color);
            eval(interpreter, x + 0.5f, y + 0.5f, color);
            output->set(x, output->height - 1 - y, color);
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

        // Wait one second while polling.
        for (int i = 0; i < 100 && rowsLeft > 0; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

void earwigMessageConsumer(spv_message_level_t level, const char *source,
        const spv_position_t& position, const char *message)
{
    std::cout << source << ": " << message << "\n";
}

bool createSPIRVFromSources(const std::vector<ShaderSource>& sources, bool debug, bool optimize, std::vector<uint32_t>& spirv)
{
    glslang::TShader *shader = new glslang::TShader(EShLangFragment);

    glslang::TProgram& glslang_program = *new glslang::TProgram;

    std::vector<const char *> strings;
    std::vector<const char *> names;
    for(auto& source: sources) {
        strings.push_back(source.code.c_str());
        names.push_back(source.filename.c_str());
    }
    shader->setStringsWithLengthsAndNames(strings.data(), NULL, names.data(), sources.size());

    shader->setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);

    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

    glslang::TShader::ForbidIncluder includer;
    TBuiltInResource resources;

    resources = glslang::DefaultTBuiltInResource;

    if (!shader->parse(&resources, 110, false, messages, includer)) {
        std::cerr << "compile failed\n";
        std::cerr << shader->getInfoLog();
        return false;
    }

    glslang_program.addShader(shader);

    if(!glslang_program.link(messages)) {
        std::cerr << "link failed\n";
        std::cerr << glslang_program.getInfoLog();
        return false;
    }

    std::string warningsErrors;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    options.generateDebugInfo = debug;
    options.disableOptimizer = !optimize;
    options.optimizeSize = false;
    glslang::TIntermediate *shaderInterm = glslang_program.getIntermediate(EShLangFragment);
    glslang::GlslangToSpv(*shaderInterm, spirv, &logger, &options);

    return true;
}

void optimizeSPIRV(spv_target_env targetEnv, std::vector<uint32_t>& spirv)
{
    Timer timer;
    spvtools::Optimizer optimizer(targetEnv);
    optimizer.SetMessageConsumer(earwigMessageConsumer);
    optimizer.RegisterPerformancePasses();
    // optimizer.SetPrintAll(&std::cerr);
    spvtools::OptimizerOptions optimizerOptions;
    bool success = optimizer.Run(spirv.data(), spirv.size(), &spirv, optimizerOptions);
    if (!success) {
        std::cout << "Warning: Optimizer failed.\n";
    }
    std::cerr << "Optimizing took " << timer.elapsed() << " seconds.\n";
}

bool createProgram(const std::vector<ShaderSource>& sources, bool debug, bool optimize, bool disassemble, bool scalarize, Program& program)
{
    std::vector<uint32_t> spirv;

    bool result = createSPIRVFromSources(sources, debug, optimize, spirv);
    if(!result) {
        return result;
    }

    spv_target_env targetEnv = SPV_ENV_UNIVERSAL_1_3;

    if (optimize) {
        if(disassemble) {
            // Not useful.
            /// spv::Disassemble(std::cout, spirv);
        }

        optimizeSPIRV(targetEnv, spirv);
    }

    if(disassemble) {
        spv::Disassemble(std::cout, spirv);
    }

    if(false) {
        FILE *fp = fopen("spirv", "wb");
        fwrite(spirv.data(), 1, spirv.size() * 4, fp);
        fclose(fp);
    }

    spv_context context = spvContextCreate(targetEnv);
    spvBinaryParse(context, &program, spirv.data(), spirv.size(), Program::handleHeader, Program::handleInstruction, nullptr);

    if (program.hasUnimplemented) {
        return false;
    }

    if (scalarize) {
        // Convert vector instructions to scalar instructions.
        program.expandVectors();
    }

    {
        Timer timer;
        program.postParse();
        std::cerr << "Post-parse took " << timer.elapsed() << " seconds.\n";
    }

    return true;
}

// https://stackoverflow.com/a/34571089/211234
static const char *BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64Encode(const std::string &in) {
    std::string out;
    int val = 0;
    int valb = -6;

    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_ALPHABET[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(BASE64_ALPHABET[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }

    return out;
}

int main(int argc, char **argv)
{
    bool debug = false;
    bool disassemble = false;
    bool optimize = false;
    bool doNotShade = false;
    bool inputIsJSON = false;
    bool imageToTerminal = false;
    bool compile = false;
    bool scalarize = false;
    int threadCount = std::thread::hardware_concurrency();
    int frameStart = 0, frameEnd = 0;
    CommandLineParameters params;

    params.outputWidth = DEFAULT_WIDTH;
    params.outputHeight = DEFAULT_HEIGHT;
    params.beVerbose = false;
    params.throwOnUnimplemented = false;

    ShInitialize();

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-g") == 0) {

            debug = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--json") == 0) {

            inputIsJSON = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-S") == 0) {

            disassemble = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-d") == 0) {

            if(argc < 3) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            params.outputWidth = atoi(argv[1]);
            params.outputHeight = atoi(argv[2]);
            argv += 3; argc -= 3;

        } else if(strcmp(argv[0], "-f") == 0) {

            if(argc < 3) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            frameStart = atoi(argv[1]);
            frameEnd = atoi(argv[2]);
            argv += 3; argc -= 3;

        } else if(strcmp(argv[0], "-j") == 0) {

            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            threadCount = atoi(argv[1]);
            argv += 2; argc -= 2;

        } else if(strcmp(argv[0], "-v") == 0) {

            params.beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-t") == 0) {

            params.throwOnUnimplemented = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-O") == 0) {

            optimize = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-n") == 0) {

            doNotShade = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-c") == 0) {

            compile = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-s") == 0) {

            scalarize = true;
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

    std::vector<ShaderToyRenderPassPtr> renderPasses;

    std::string filename = argv[0];
    std::string shader_code;

    if(!inputIsJSON) {

        shader_code = readFileContents(filename);

        ImagePtr image(new Image(Image::FORMAT_R8G8B8_UNORM, Image::DIM_2D, params.outputWidth, params.outputHeight));
        ShaderToyImage output {0, 0, {image, Sampler {}}};

        std::vector<ShaderSource> sources = { {"", ""}, {shader_code, filename}};
        ShaderToyRenderPassPtr pass(new ShaderToyRenderPass("Image", {}, {output}, sources, params));

        renderPasses.push_back(pass);

    } else {

        getOrderedRenderPassesFromJSON(filename, renderPasses, params);

    }

    ShaderSource preamble { readFileContents(shaderPreambleFilename), shaderPreambleFilename };
    ShaderSource epilogue { readFileContents(shaderEpilogueFilename), shaderEpilogueFilename };

    // Do passes

    for(auto& pass: renderPasses) {

        ShaderToyImage output = pass->outputs[0];

        std::vector<ShaderSource> sources;

        sources.push_back(preamble);
        for(auto& ss: pass->sources) {
            sources.push_back(ss);
        }
        sources.push_back(epilogue);

        bool success = createProgram(sources, debug, optimize, disassemble, scalarize, pass->pgm);
        if(!success) {
            exit(EXIT_FAILURE);
        }

        if (compile) {
            pass->pgm.prepareForCompile();
            bool success = compileProgram(pass->pgm);
            exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
        }

        if (doNotShade) {
            exit(EXIT_SUCCESS);
        }

        for(int i = 0; i < pass->inputs.size(); i++) {
            auto& toyImage = pass->inputs[i];
            pass->pgm.sampledImages[i] = toyImage.sampledImage;
        }
    }

    std::cout << "Using " << threadCount << " threads.\n";

    for(int frameNumber = frameStart; frameNumber <= frameEnd; frameNumber++) {
        for(auto& pass: renderPasses) {

            Timer timer;

            ShaderToyImage output = pass->outputs[0];
            ImagePtr image = output.sampledImage.image;

            // Workers decrement rowsLeft at the end of each row.
            rowsLeft = image->height;

            std::vector<std::thread *> thread;

            // Generate the rows on multiple threads.
            for (int t = 0; t < threadCount; t++) {
                thread.push_back(new std::thread(render, pass.get(), t, threadCount, frameNumber, frameNumber / 60.0));
                // std::this_thread::sleep_for(std::chrono::milliseconds(100)); XXX delete
            }

            // Progress information.
            thread.push_back(new std::thread(showProgress, image->height, timer.startTime()));

            // Wait for worker threads to quit.
            for (int t = 0; t < thread.size(); t++) {
                std::thread* td = thread.back();
                thread.pop_back();
                td->join();
            }

            double elapsedSeconds = timer.elapsed();
            std::cerr << "Shading pass " << pass->name << " took " << elapsedSeconds << " seconds ("
                << long(image->width*image->height/elapsedSeconds) << " pixels per second)\n";
        }

        if(false) {
            ShaderToyImage output = renderPasses[0]->outputs[0];
            ImagePtr image = output.sampledImage.image;

            std::ostringstream ss;
            ss << "pass0" << std::setfill('0') << std::setw(4) << frameNumber << std::setw(0) << ".ppm";
            std::ofstream imageFile(ss.str(), std::ios::out | std::ios::binary);
            image->writePpm(imageFile);
            imageFile.close();
        }

        ShaderToyImage output = renderPasses.back()->outputs[0];
        ImagePtr image = output.sampledImage.image;

        std::ostringstream ss;
        ss << "image" << std::setfill('0') << std::setw(4) << frameNumber << std::setw(0) << ".ppm";
        std::ofstream imageFile(ss.str(), std::ios::out | std::ios::binary);
        image->writePpm(imageFile);
        imageFile.close();

        if (imageToTerminal) {
            // https://www.iterm2.com/documentation-images.html
            std::ostringstream ss;
            image->writePpm(ss);
            std::cout << "\033]1337;File=width="
                << image->width << "px;height="
                << image->height << "px;inline=1:"
                << base64Encode(ss.str()) << "\007\n";
        }
    }

    exit(EXIT_SUCCESS);
}
