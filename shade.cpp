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
#include "function.h"
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

static const char *DEFAULT_ASSEMBLY_PATHNAME = "out.s";

// -----------------------------------------------------------------------------------

void RiscVPhi::emit(Compiler *compiler)
{
    // Nothing.
}

void RiscVAddi::emit(Compiler *compiler)
{
    compiler->emitBinaryImmOp("addi", resultId(), rs1(), imm);
}

void RiscVLoad::emit(Compiler *compiler)
{
    std::ostringstream ss;
    std::ostringstream ssc;
    if (compiler->isRegFloat(resultId())) {
        ss << "flw ";
    } else {
        ss << "lw ";
    }
    ss << compiler->reg(resultId()) << ", ";
    assert(!compiler->pgm->isConstant(pointerId())); // Use RiscVLoadConst.
    if (compiler->asRegister(pointerId()) == nullptr) {
        // It's a variable reference.
        ss << compiler->getVariableName(pointerId());
        if (offset != 0) {
            ss << "+" << offset;
        }
        ss << "(x0)";
    } else {
        // It's a register reference.
        ss << offset << "(" << compiler->reg(pointerId()) << ")";
    }
    ssc << "r" << resultId() << " = (r" << pointerId() << ")";
    compiler->emit(ss.str(), ssc.str());
}

void RiscVLoadConst::emit(Compiler *compiler)
{
    std::ostringstream ss;
    std::ostringstream ssc;

    if (compiler->isRegFloat(resultId())) {
        ss << "flw ";
    } else {
        ss << "lw ";
    }
    ss << compiler->reg(resultId()) << ", " << ".C" << constId << "(x0)";
    ssc << "r" << resultId() << " = constant r" << constId;

    compiler->emit(ss.str(), ssc.str());
}

void RiscVStore::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(objectId())) {
        ss1 << "fsw ";
    } else {
        ss1 << "sw ";
    }
    ss1 << compiler->reg(objectId()) << ", ";
    if (compiler->asRegister(pointerId()) == nullptr) {
        // It's a variable reference.
        ss1 << compiler->getVariableName(pointerId());
        if (offset != 0) {
            ss1 << "+" << offset;
        }
        ss1 << "(x0)";
    } else {
        // It's a register reference.
        ss1 << offset << "(" << compiler->reg(pointerId()) << ")";
    }
    std::ostringstream ss2;
    ss2 << "(r" << pointerId() << ") = r" << objectId();
    compiler->emit(ss1.str(), ss2.str());
}

void RiscVCross::emit(Compiler *compiler)
{
    compiler->emit("addi sp, sp, -28", "Make room on stack");
    compiler->emit("sw ra, 24(sp)", "Save return address");

    for (int i = argIdList.size() - 1; i >= 0; i--) {
        std::ostringstream ss;
        ss << "fsw " << compiler->reg(argIdList[i]) << ", " << (i*4) << "(sp)";
        std::ostringstream ssc;
        ssc << "Push parameter r" << argIdList[i];
        compiler->emit(ss.str(), ssc.str());
    }

    compiler->emit("jal ra, .cross", "Call routine");

    for (size_t i = 0; i < resIdList.size(); i++) {
        std::ostringstream ss;
        ss << "flw " << compiler->reg(resIdList[i]) << ", " << (i*4) << "(sp)";
        std::ostringstream ssc;
        ssc << "Pop result r" << resIdList[i];
        compiler->emit(ss.str(), ssc.str());
    }

    compiler->emit("lw ra, 12(sp)", "Restore return address");
    compiler->emit("addi sp, sp, 16", "Restore stack");
}

void RiscVLength::emit(Compiler *compiler)
{
    size_t n = argIdList.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".length" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVReflect::emit(Compiler *compiler)
{
    size_t n = resIdList.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".reflect" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVNormalize::emit(Compiler *compiler)
{
    size_t n = argIdList.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".normalize" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVDistance::emit(Compiler *compiler)
{
    size_t n = argIdList.size()/2;
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".distance" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVDot::emit(Compiler *compiler)
{
    size_t n = argIdList.size()/2;
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".dot" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVAll::emit(Compiler *compiler)
{
    size_t n = argIdList.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".all" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

void RiscVAny::emit(Compiler *compiler)
{
    size_t n = argIdList.size();
    assert(n <= 4);

    std::ostringstream functionName;
    functionName << ".any" << n;

    compiler->emitCall(functionName.str(), resIdList, argIdList);
}

// -----------------------------------------------------------------------------------

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

void InsnFNegate::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fsgnjn.s", resultId(), operandId(), operandId());
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

void InsnIEqual::emit(Compiler *compiler)
{
    // sub result, op1, op2
    std::ostringstream ss1;
    ss1 << "sub "
        << compiler->reg(resultId()) << ", "
        << compiler->reg(operand1Id()) << ", "
        << compiler->reg(operand2Id());
    std::ostringstream ssc1;
    ssc1 << "r" << resultId() << " = r" << operand1Id() << " - r" << operand2Id();
    compiler->emit(ss1.str(), ssc1.str());

    // sltiu result, result, 1
    std::ostringstream ss2;
    ss2 << "sltiu " << compiler->reg(resultId()) << ", " << compiler->reg(resultId()) << ", 1";
    std::ostringstream ssc2;
    ssc2 << "r" << resultId() << " = 1 if difference was 0 (r"
        << operand1Id() << " == r" << operand2Id() << ")";
    compiler->emit(ss2.str(), ssc2.str());
}

void InsnSLessThan::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("slt", resultId(), operand1Id(), operand2Id());
}

void InsnLogicalAnd::emit(Compiler *compiler)
{
    // SPIR-V guarantees that the two operands are Bool type, but doesn't specify
    // the bit pattern. I believe our current code always generates 0 or 1 for
    // results of boolean ops (e.g., fle.s, flt.s).
    compiler->emitBinaryOp("and", resultId(), operand1Id(), operand2Id());
}

void InsnLogicalOr::emit(Compiler *compiler)
{
    // SPIR-V guarantees that the two operands are Bool type, but doesn't specify
    // the bit pattern. I believe our current code always generates 0 or 1 for
    // results of boolean ops (e.g., fle.s, flt.s).
    compiler->emitBinaryOp("or", resultId(), operand1Id(), operand2Id());
}

void InsnLogicalNot::emit(Compiler *compiler)
{
    // SPIR-V guarantees that the two operands are Bool type, but doesn't specify
    // the bit pattern. I believe our current code always generates 0 or 1 for
    // results of boolean ops (e.g., fle.s, flt.s).
    compiler->emitBinaryImmOp("xori", resultId(), operandId(), 1);
}

void InsnSelect::emit(Compiler *compiler)
{
    // If conditionId() is true, pick object1Id(), else pick object2Id().

    std::string falseLabel = compiler->makeLocalLabel();
    std::string endLabel = compiler->makeLocalLabel();

    // Skip to false copy if condition is zero.
    {
        std::ostringstream ss;
        ss << "beq " << compiler->reg(conditionId()) << ", x0, " << falseLabel;
        std::ostringstream ssc;
        ssc << "r" << conditionId() << " == 0 ?";
        compiler->emit(ss.str(), ssc.str());
    }

    // Copy true value to result.
    compiler->emitCopyVariable(resultId(), object1Id(), " (true result)");

    {
        std::ostringstream ss;
        ss << "jal x0, " << endLabel;
        compiler->emit(ss.str(), "jump to end of select");
    }

    // Copy false value to result.
    compiler->emitLabel(falseLabel);
    compiler->emitCopyVariable(resultId(), object2Id(), " (false result)");
    compiler->emitLabel(endLabel);
}

void InsnFunctionCall::emit(Compiler *compiler)
{
    compiler->emit("push pc", "");
    for(int i = operandIdCount() - 1; i >= 0; i--) {
        compiler->emit(std::string("push ") + compiler->reg(operandId(i)), "");
    }
    compiler->emit(std::string("call ") + compiler->pgm->functions.at(functionId)->cleanName, "");
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
    compiler->emitPhiCopy(this, targetLabelId);

    std::ostringstream ss;
    ss << "jal x0, block" << targetLabelId;
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

void InsnConvertFToS::emit(Compiler *compiler)
{
    compiler->emitUnaryOp("fcvt.w.s", resultId(), floatValueId());
}

void InsnConvertSToF::emit(Compiler *compiler)
{
    compiler->emitUnaryOp("fcvt.s.w", resultId(), signedValueId());
}

void InsnFOrdEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("feq.s", resultId(), operand1Id(), operand2Id());
}

void InsnFOrdLessThanEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fle.s", resultId(), operand1Id(), operand2Id());
}

void InsnFOrdLessThan::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("flt.s", resultId(), operand1Id(), operand2Id());
}

void InsnFOrdGreaterThanEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fle.s", resultId(), operand2Id(), operand1Id());
}

void InsnFOrdGreaterThan::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("flt.s", resultId(), operand2Id(), operand1Id());
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
    compiler->emitPhiCopy(this, trueLabelId);
    std::ostringstream ss2;
    ss2 << "jal x0, block" << trueLabelId;
    compiler->emit(ss2.str(), "");
    // False path.
    compiler->emitLabel(localLabel);
    compiler->emitPhiCopy(this, falseLabelId);
    std::ostringstream ss3;
    ss3 << "jal x0, block" << falseLabelId;
    compiler->emit(ss3.str(), "");
}

void InsnAccessChain::emit(Compiler *compiler)
{
    uint32_t offset = 0;

    const Variable &variable = compiler->pgm->variables.at(baseId());
    uint32_t type = variable.type;
    bool variableIndex = false;
    for (size_t i = 0; i < indexesIdCount(); i++) {
        uint32_t id = indexesId(i);
        uint32_t intValue;
        if (compiler->asIntegerConstant(id, intValue)) {
            auto [subtype, subOffset] = compiler->pgm->getConstituentInfo(type, intValue);
            type = subtype;
            offset += subOffset;
        } else {
            variableIndex = true;
            break;
        }
    }

    // If we have a variable (non-constant) index, see if it meets some strict requirements
    // for an easy solution.
    if (variableIndex) {
        // This will assert if the type doesn't have constant subtype size.
        // I think this only happens for structs.
        uint32_t subtypeSize = compiler->pgm->types.at(variable.type)->getSubtypeSize();

        // Must be power of two.
        if (indexesIdCount() != 1 || (subtypeSize & (subtypeSize - 1)) != 0) {
            std::cerr << "Error: Don't yet handle non-constant offsets in AccessChain (at "
                << resultId() << ").\n";
            exit(EXIT_FAILURE);
        }

        // Figure out shift.
        int shift = 0;
        while (subtypeSize > 1) {
            shift++;
            subtypeSize >>= 1;
        }
        subtypeSize <<= shift;

        // Emit shift to get offset from base of pointer.
        {
            uint32_t id = indexesId(0);
            std::ostringstream ss;
            ss << "slli " << compiler->reg(resultId()) << ", "
                << compiler->reg(id) << ", " << shift;
            std::ostringstream ssc;
            ssc << "Multiply by " << subtypeSize;
            compiler->emit(ss.str(), ssc.str());
        }

        // Add to base.
        {
            std::ostringstream ss;
            ss << "addi " << compiler->reg(resultId()) << ", "
                << compiler->reg(resultId()) << ", "
                << compiler->getVariableName(baseId());
            compiler->emit(ss.str(), "Add to variable location");
        }
    } else {
        // All constants.
        auto name = compiler->pgm->names.find(baseId());
        if (name == compiler->pgm->names.end()) {
            std::cerr << "Error: Don't yet handle pointer reference in AccessChain ("
                << baseId() << ", at " << resultId() << ").\n";
            exit(EXIT_FAILURE);
        }

        std::ostringstream ss;
        ss << "addi " << compiler->reg(resultId()) << ", x0, "
            << compiler->notEmptyLabel(name->second) << "+" << offset;
        compiler->emit(ss.str(), "");
    }
}

void InsnCopyObject::emit(Compiler *compiler)
{
    const CompilerRegister *r1 = compiler->asRegister(resultId());
    const CompilerRegister *r2 = compiler->asRegister(operandId());
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
            << compiler->reg(operandId()) << ", "
            << compiler->reg(operandId());
    }
    std::ostringstream ss2;
    ss2 << "r" << resultId() << " = r" << operandId();
    compiler->emit(ss1.str(), ss2.str());
}

void InsnGLSLstd450Sqrt::emit(Compiler *compiler)
{
    std::ostringstream ss;
    ss << "fsqrt.s " << compiler->reg(resultId()) << ", " << compiler->reg(xId());
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

void InsnGLSLstd450Exp::emit(Compiler *compiler)
{
    compiler->emitUniCall(".exp", resultId(), xId());
}

void InsnGLSLstd450Exp2::emit(Compiler *compiler)
{
    compiler->emitUniCall(".exp2", resultId(), xId());
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

void InsnGLSLstd450FMin::emit(Compiler *compiler)
{
    std::ostringstream ss;
    ss << "fmin.s " << compiler->reg(resultId())
        << ", " << compiler->reg(xId())
        << ", " << compiler->reg(yId());
    compiler->emit(ss.str(), "");
}

void InsnGLSLstd450FMax::emit(Compiler *compiler)
{
    std::ostringstream ss;
    ss << "fmax.s " << compiler->reg(resultId())
        << ", " << compiler->reg(xId())
        << ", " << compiler->reg(yId());
    compiler->emit(ss.str(), "");
}

void InsnGLSLstd450Pow::emit(Compiler *compiler)
{
    compiler->emitBinCall(".pow", resultId(), xId(), yId());
}

void InsnGLSLstd450Log::emit(Compiler *compiler)
{
    compiler->emitUniCall(".log", resultId(), xId());
}

void InsnGLSLstd450Log2::emit(Compiler *compiler)
{
    compiler->emitUniCall(".log2", resultId(), xId());
}

void InsnGLSLstd450FClamp::emit(Compiler *compiler)
{
    compiler->emitTerCall(".clamp", resultId(), xId(), minValId(), maxValId());
}

void InsnGLSLstd450FMix::emit(Compiler *compiler)
{
    compiler->emitTerCall(".mix", resultId(), xId(), yId(), aId());
}

void InsnGLSLstd450SmoothStep::emit(Compiler *compiler)
{
    compiler->emitTerCall(".smoothstep", resultId(), edge0Id(), edge1Id(), xId());
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
    printf("\t--json    input file is a ShaderToy JSON file\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
    printf("\t-o out.s  output assembly pathname [%s]\n", DEFAULT_ASSEMBLY_PATHNAME);
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

    interpreter.set("iResolution", v3float {static_cast<float>(output->width), static_cast<float>(output->height), 1.0f});

    interpreter.set("iFrame", frameNumber);

    interpreter.set("iTime", when);

    interpreter.set("iTimeDelta", 1.0f / 60.0f);

    interpreter.set("iMouse", v4float {0, 0, 0, 0});

    for(size_t i = 0; i < pass->inputs.size(); i++) {
        auto& input = pass->inputs[i];
        interpreter.set("iChannel" + std::to_string(input.channelNumber), i);
        ImagePtr image = input.sampledImage.image;
        float w = static_cast<float>(image->width);
        float h = static_cast<float>(image->height);
        interpreter.set("iChannelResolution[" + std::to_string(input.channelNumber) + "]", v3float{w, h, 0});
    }

    // This loop acts like a rasterizer fixed function block.  Maybe it should
    // set inputs and read outputs also.
    for(uint32_t y = startRow; y < output->height; y += skip) {
        for(uint32_t x = 0; x < output->width; x++) {
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
    if (PRINT_TIMER_RESULTS) {
        std::cerr << "Optimizing took " << timer.elapsed() << " seconds.\n";
    }
}

bool createProgram(const std::vector<ShaderSource>& sources, bool debug, bool optimize, bool disassemble, Program& program)
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

    {
        Timer timer;
        program.postParse();
        if (PRINT_TIMER_RESULTS) {
            std::cerr << "Post-parse took " << timer.elapsed() << " seconds.\n";
        }
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
    int threadCount = std::thread::hardware_concurrency();
    int frameStart = 0, frameEnd = 0;
    CommandLineParameters params;
    std::string outputAssemblyPathname = DEFAULT_ASSEMBLY_PATHNAME;

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

        } else if(strcmp(argv[0], "-o") == 0) {

            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            outputAssemblyPathname = argv[1];
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

        bool success = createProgram(sources, debug, optimize, disassemble, pass->pgm);
        if(!success) {
            exit(EXIT_FAILURE);
        }

        if (compile) {
            pass->pgm.prepareForCompile();
            Compiler compiler(&pass->pgm, outputAssemblyPathname);
            compiler.compile();
            exit(EXIT_SUCCESS);
        }

        if (doNotShade) {
            exit(EXIT_SUCCESS);
        }

        for(size_t i = 0; i < pass->inputs.size(); i++) {
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
            while (!thread.empty()) {
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
