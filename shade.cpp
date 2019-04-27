#include <iostream>
#include <vector>
#include <functional>
#include <set>
#include <cstdio>
#include <fstream>
#include <variant>
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

#ifdef USE_CPP17_FILESYSTEM
// filesystem still not available in XCode 2019/04/04
#include <filesystem>
#else
#include <libgen.h>
#endif

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

#include "json.hpp"

#include "basic_types.h"
#include "risc-v.h"
#include "image.h"
#include "interpreter.h"
#include "program.h"
#include "timer.h"

#define DEFAULT_WIDTH (640/2)
#define DEFAULT_HEIGHT (360/2)

using json = nlohmann::json;

// Enable this to check if our virtual RAM is being initialized properly.
#define CHECK_MEMORY_ACCESS
// Enable this to check if our virtual registers are being initialized properly.
#define CHECK_REGISTER_ACCESS

const bool throwOnUninitializedMemoryRead = false;
struct UninitializedMemoryReadException : std::runtime_error
{
    UninitializedMemoryReadException(const std::string& what) : std::runtime_error(what) {}
};

Interpreter::Interpreter(const Program *pgm)
    : pgm(pgm)
{
    memory = new unsigned char[pgm->memorySize];
    memoryInitialized = new bool[pgm->memorySize];

    // So we can catch errors.
    std::fill(memory, memory + pgm->memorySize, 0xFF);
    std::fill(memoryInitialized, memoryInitialized + pgm->memorySize, false);

    // Allocate registers so they aren't allocated during run()
    for(auto [id, type]: pgm->resultTypes) {
        registers[id] = Register {type, pgm->typeSizes.at(type)};
    }
}

const size_t MEMORY_CHECK_OKAY = 0xFFFFFFFF;
size_t Interpreter::checkMemory(size_t address, size_t size)
{
#ifdef CHECK_MEMORY_ACCESS
    for (size_t a = address; a < address + size; a++) {
        if (!memoryInitialized[a]) {
            return a;
        }
    }
#endif
    return MEMORY_CHECK_OKAY;
}

void Interpreter::markMemory(size_t address, size_t size)
{
#ifdef CHECK_MEMORY_ACCESS
    std::fill(memoryInitialized + address, memoryInitialized + address + size, true);
#endif
}

template <class T>
T& Interpreter::objectInMemoryAt(size_t addr, bool reading, size_t size)
{
    if (reading) {
        size_t result = checkMemory(addr, size);
        if(result != MEMORY_CHECK_OKAY) {
            std::cerr << "Warning: Reading uninitialized byte at " << (result - addr) << "\n";
        }
    } else {
        markMemory(addr, size);
    }
    return *reinterpret_cast<T*>(memory + addr);
}

template <class T>
T& Interpreter::objectInClassAt(SpvStorageClass clss, size_t offset, bool reading, size_t size)
{
    size_t addr = pgm->memoryRegions.at(clss).base + offset;
    if (reading) {
        size_t result = checkMemory(addr, size);
        if(result != MEMORY_CHECK_OKAY) {
            std::cerr << "Warning: Reading uninitialized byte " << (result - addr) << " within object at " << offset << " of size " << size << " within class " << clss << " (base " << pgm->memoryRegions.at(clss).base << ")\n";
        }
    } else {
        markMemory(addr, size);
    }
    return *reinterpret_cast<T*>(memory + addr);
}

template <class T>
const T& Interpreter::fromRegister(int id)
{
    Register &reg = registers.at(id);
#ifdef CHECK_REGISTER_ACCESS
    if (!reg.initialized) {
        std::cerr << "Warning: Reading uninitialized register " << id << "\n";
    }
#endif
    return *reinterpret_cast<T*>(reg.data);
}

template <class T>
T& Interpreter::toRegister(int id)
{
    Register &reg = registers.at(id);
#ifdef CHECK_REGISTER_ACCESS
    reg.initialized = true;
#endif
    return *reinterpret_cast<T*>(reg.data);
}

void Interpreter::clearPrivateVariables()
{
    // Global variables are cleared for each run.
    const MemoryRegion &mr = pgm->memoryRegions.at(SpvStorageClassPrivate);
    std::fill(memory + mr.base, memory + mr.top, 0x00);
    markMemory(mr.base, mr.top - mr.base);
}

void Interpreter::stepNop(const InsnNop& insn)
{
    // Nothing to do.
}

void Interpreter::stepLoad(const InsnLoad& insn)
{
    Pointer& ptr = pointers.at(insn.pointerId);
    Register& obj = registers.at(insn.resultId);
    size_t size = pgm->typeSizes.at(insn.type);
    size_t result = checkMemory(ptr.address, size);
    if(result != MEMORY_CHECK_OKAY) {
        std::cerr << "Warning: Reading uninitialized byte " << result << " within object at " << ptr.address << " of size " << size << " in stepLoad from pointer " << insn.pointerId;

        if(pgm->names.find(insn.pointerId) != pgm->names.end()) {
            std::cerr << " named \"" << pgm->names.at(insn.pointerId) << "\"";
        } else {
            std::cerr << " with no name";
        }

        if(insn.lineInfo.fileId != NO_FILE) {
            std::cerr << " at file \"" << pgm->strings.at(insn.lineInfo.fileId) << "\":" << insn.lineInfo.line;
        } else {
            std::cerr << " with no source line information";
        }

        std::cerr << '\n';

        if(throwOnUninitializedMemoryRead) {
            throw UninitializedMemoryReadException("Warning: Reading uninitialized memory " + std::to_string(ptr.address) + " of size " + std::to_string(size) + "\n");
        }
    }

    std::copy(memory + ptr.address, memory + ptr.address + size, obj.data);
    obj.initialized = true;
    if(false) {
        std::cout << "load result is";
        pgm->dumpTypeAt(pgm->types.at(insn.type), obj.data);
        std::cout << "\n";
    }
}

void Interpreter::stepStore(const InsnStore& insn)
{
    Pointer& ptr = pointers.at(insn.pointerId);
    Register& obj = registers.at(insn.objectId);
#ifdef CHECK_REGISTER_ACCESS
    if (!obj.initialized) {
        std::cerr << "Warning: Storing uninitialized register " << insn.objectId << "\n";
    }
#endif
    std::copy(obj.data, obj.data + obj.size, memory + ptr.address);
    markMemory(ptr.address, obj.size);
}

void Interpreter::stepCompositeExtract(const InsnCompositeExtract& insn)
{
    Register& obj = registers.at(insn.resultId);
    Register& src = registers.at(insn.compositeId);
#ifdef CHECK_REGISTER_ACCESS
    if (!src.initialized) {
        std::cerr << "Warning: Extracting uninitialized register " << insn.compositeId << "\n";
    }
#endif
    /* use indexes to walk blob */
    uint32_t type = src.type;
    size_t offset = 0;
    for(auto& j: insn.indexesId) {
        uint32_t constituentOffset;
        std::tie(type, constituentOffset) = pgm->getConstituentInfo(type, j);
        offset += constituentOffset;
    }
    std::copy(src.data + offset, src.data + offset + pgm->typeSizes.at(obj.type), obj.data);
    obj.initialized = true;
    if(false) {
        std::cout << "extracted from ";
        pgm->dumpTypeAt(pgm->types.at(src.type), src.data);
        std::cout << " result is ";
        pgm->dumpTypeAt(pgm->types.at(insn.type), obj.data);
        std::cout << "\n";
    }
}

// XXX This method has not been tested.
void Interpreter::stepCompositeInsert(const InsnCompositeInsert& insn)
{
    Register& res = registers.at(insn.resultId);
    Register& obj = registers.at(insn.objectId);
    Register& cmp = registers.at(insn.compositeId);

#ifdef CHECK_REGISTER_ACCESS
    if (!obj.initialized) {
        std::cerr << "Warning: Inserting uninitialized register " << insn.objectId << "\n";
    }
    if (!cmp.initialized) {
        std::cerr << "Warning: Inserting from uninitialized register " << insn.compositeId << "\n";
    }
#endif

    // Start by copying composite to result.
    std::copy(cmp.data, cmp.data + pgm->typeSizes.at(cmp.type), res.data);
    res.initialized = true;

    /* use indexes to walk blob */
    uint32_t type = res.type;
    size_t offset = 0;
    for(auto& j: insn.indexesId) {
        uint32_t constituentOffset;
        std::tie(type, constituentOffset) = pgm->getConstituentInfo(type, j);
        offset += constituentOffset;
    }
    std::copy(obj.data, obj.data + pgm->typeSizes.at(obj.type), res.data);
}

void Interpreter::stepCompositeConstruct(const InsnCompositeConstruct& insn)
{
    Register& obj = registers.at(insn.resultId);
    size_t offset = 0;
    for(auto& j: insn.constituentsId) {
        Register& src = registers.at(j);
#ifdef CHECK_REGISTER_ACCESS
        if (!src.initialized) {
            std::cerr << "Warning: Compositing from uninitialized register " << j << "\n";
        }
#endif
        std::copy(src.data, src.data + pgm->typeSizes.at(src.type), obj.data + offset);
        offset += pgm->typeSizes.at(src.type);
    }
    obj.initialized = true;
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

            uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
            uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
            uint32_t result = operand1 + operand2;
            toRegister<uint32_t>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
            const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
            uint32_t* result = &toRegister<uint32_t>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] + operand2[i];
            }

        } else {

            std::cout << "Unknown type for IAdd\n";

        }
    }, pgm->types.at(insn.type));
}

void Interpreter::stepISub(const InsnISub& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
            uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
            uint32_t result = operand1 - operand2;
            toRegister<uint32_t>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
            const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
            uint32_t* result = &toRegister<uint32_t>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] - operand2[i];
            }

        } else {

            std::cout << "Unknown type for ISub\n";

        }
    }, pgm->types.at(insn.type));
}

void Interpreter::stepFAdd(const InsnFAdd& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            float result = operand1 + operand2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            float* result = &toRegister<float>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            float result = operand1 - operand2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            float* result = &toRegister<float>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            float result = operand1 * operand2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            float* result = &toRegister<float>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            float result = operand1 / operand2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            float* result = &toRegister<float>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            float result = operand1 - floor(operand1/operand2)*operand2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            float* result = &toRegister<float>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            bool result = operand1 < operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            bool result = operand1 > operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            bool result = operand1 <= operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            // XXX I don't know the difference between ordered and equal
            // vs. unordered and equal, so I don't know which this is.
            bool result = operand1 == operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            toRegister<float>(insn.resultId) = -fromRegister<float>(insn.operandId);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand = &fromRegister<float>(insn.operandId);
            float* result = &toRegister<float>(insn.resultId);
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
float dotProduct(const float *a, const float *b, int count)
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

            const float* vector1 = &fromRegister<float>(insn.vector1Id);
            const float* vector2 = &fromRegister<float>(insn.vector2Id);
            toRegister<float>(insn.resultId) = dotProduct(vector1, vector2, t1.count);

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

            float operand1 = fromRegister<float>(insn.operand1Id);
            float operand2 = fromRegister<float>(insn.operand2Id);
            bool result = operand1 >= operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* operand1 = &fromRegister<float>(insn.operand1Id);
            const float* operand2 = &fromRegister<float>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] >= operand2[i];
            }

        } else {

            std::cout << "Unknown type for FOrdGreaterThanEqual\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
} 

void Interpreter::stepSLessThanEqual(const InsnSLessThanEqual& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
            int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
            bool result = operand1 <= operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
            const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] <= operand2[i];
            }

        } else {

            std::cout << "Unknown type for SLessThanEqual\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepSLessThan(const InsnSLessThan& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
            int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
            bool result = operand1 < operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
            const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
            int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
            int32_t result = operand1 / operand2;
            toRegister<int32_t>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
            const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
            int32_t* result = &toRegister<int32_t>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] / operand2[i];
            }

        } else {

            std::cout << "Unknown type for SDiv\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepINotEqual(const InsnINotEqual& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
            uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
            bool result = operand1 != operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
            const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] != operand2[i];
            }

        } else {

            std::cout << "Unknown type for INotEqual\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepIEqual(const InsnIEqual& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeInt>) {

            uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
            uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
            bool result = operand1 == operand2;
            toRegister<bool>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
            const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            toRegister<bool>(insn.resultId) = !fromRegister<bool>(insn.operandId);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const bool* operand = &fromRegister<bool>(insn.operandId);
            bool* result = &toRegister<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = !operand[i];
            }

        } else {

            std::cout << "Unknown type for LogicalNot\n";

        }
    }, pgm->types.at(registers[insn.operandId].type));
}

void Interpreter::stepLogicalAnd(const InsnLogicalAnd& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;
        if constexpr (std::is_same_v<T, TypeBool>) {

            bool operand1 = fromRegister<bool>(insn.operand1Id);
            bool operand2 = fromRegister<bool>(insn.operand2Id);
            toRegister<bool>(insn.resultId) = operand1 && operand2;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const bool* operand1 = &fromRegister<bool>(insn.operand1Id);
            const bool* operand2 = &fromRegister<bool>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = operand1[i] && operand2[i];
            }

        } else {

            std::cout << "Unknown type for LogicalAnd\n";

        }
    }, pgm->types.at(registers[insn.operand1Id].type));
}

void Interpreter::stepLogicalOr(const InsnLogicalOr& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeBool>) {

            bool operand1 = fromRegister<bool>(insn.operand1Id);
            bool operand2 = fromRegister<bool>(insn.operand2Id);
            toRegister<bool>(insn.resultId) = operand1 || operand2;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const bool* operand1 = &fromRegister<bool>(insn.operand1Id);
            const bool* operand2 = &fromRegister<bool>(insn.operand2Id);
            bool* result = &toRegister<bool>(insn.resultId);
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

            bool condition = fromRegister<bool>(insn.conditionId);
            float object1 = fromRegister<float>(insn.object1Id);
            float object2 = fromRegister<float>(insn.object2Id);
            float result = condition ? object1 : object2;
            toRegister<float>(insn.resultId) = result;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const bool* condition = &fromRegister<bool>(insn.conditionId);
            // XXX shouldn't assume floats here. Any data is valid.
            const float* object1 = &fromRegister<float>(insn.object1Id);
            const float* object2 = &fromRegister<float>(insn.object2Id);
            float* result = &toRegister<float>(insn.resultId);
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
    const float* vector = &fromRegister<float>(insn.vectorId);
    float scalar = fromRegister<float>(insn.scalarId);
    float* result = &toRegister<float>(insn.resultId);

    const TypeVector &type = std::get<TypeVector>(pgm->types.at(insn.type));

    for(int i = 0; i < type.count; i++) {
        result[i] = vector[i] * scalar;
    }
}

void Interpreter::stepMatrixTimesMatrix(const InsnMatrixTimesMatrix& insn)
{
    const float* left = &fromRegister<float>(insn.leftMatrixId);
    const float* right = &fromRegister<float>(insn.rightMatrixId);
    float* result = &toRegister<float>(insn.resultId);

    const Register &leftMatrixReg = registers[insn.leftMatrixId];

    const TypeMatrix &leftMatrixType = std::get<TypeMatrix>(pgm->types.at(leftMatrixReg.type));
    const TypeVector &leftMatrixVectorType = std::get<TypeVector>(pgm->types.at(leftMatrixType.columnType));

    const TypeMatrix &rightMatrixType = std::get<TypeMatrix>(pgm->types.at(leftMatrixReg.type));

    const TypeMatrix &resultType = std::get<TypeMatrix>(pgm->types.at(insn.type));
    const TypeVector &resultVectorType = std::get<TypeVector>(pgm->types.at(resultType.columnType));

    int resultColumnCount = resultType.columnCount;
    int resultRowCount = resultVectorType.count;
    int leftcols = leftMatrixType.columnCount;
    int leftrows = leftMatrixVectorType.count;
    int rightcols = rightMatrixType.columnCount;
    assert(leftrows == rightcols);

    for(int i = 0; i < resultRowCount; i++) {
        for(int j = 0; j < resultColumnCount; j++) {
            float dot = 0;

            for(int k = 0; k < leftcols; k++) {
                dot += left[k * leftrows + i] * right[k + rightcols * j];
            }
            result[j * resultRowCount + i] = dot;
        }
    }
}

void Interpreter::stepMatrixTimesVector(const InsnMatrixTimesVector& insn)
{
    const float* matrix = &fromRegister<float>(insn.matrixId);
    const float* vector = &fromRegister<float>(insn.vectorId);
    float* result = &toRegister<float>(insn.resultId);

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
    const float* vector = &fromRegister<float>(insn.vectorId);
    const float* matrix = &fromRegister<float>(insn.matrixId);
    float* result = &toRegister<float>(insn.resultId);

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
    Register& obj = registers.at(insn.resultId);
    const Register &r1 = registers.at(insn.vector1Id);
    const Register &r2 = registers.at(insn.vector2Id);
    const TypeVector &t1 = std::get<TypeVector>(pgm->types.at(r1.type));
    uint32_t n1 = t1.count;
    uint32_t elementSize = pgm->typeSizes.at(t1.type);

#ifdef CHECK_REGISTER_ACCESS
    if (!r1.initialized) {
        std::cerr << "Warning: Shuffling register " << insn.vector1Id << "\n";
    }
    if (!r2.initialized) {
        std::cerr << "Warning: Shuffling register " << insn.vector2Id << "\n";
    }
#endif

    for(int i = 0; i < insn.componentsId.size(); i++) {
        uint32_t component = insn.componentsId[i];
        unsigned char *src = component < n1
            ? r1.data + component*elementSize
            : r2.data + (component - n1)*elementSize;
        std::copy(src, src + elementSize, obj.data + i*elementSize);
    }
    obj.initialized = true;
}

void Interpreter::stepConvertSToF(const InsnConvertSToF& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            int32_t src = fromRegister<int32_t>(insn.signedValueId);
            toRegister<float>(insn.resultId) = src;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const int32_t* src = &fromRegister<int32_t>(insn.signedValueId);
            float* dst = &toRegister<float>(insn.resultId);
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

            float src = fromRegister<float>(insn.floatValueId);
            toRegister<uint32_t>(insn.resultId) = src;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* src = &fromRegister<float>(insn.floatValueId);
            uint32_t* dst = &toRegister<uint32_t>(insn.resultId);
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
    size_t address = basePointer.address;
    for(auto& id: insn.indexesId) {
        int32_t j = fromRegister<int32_t>(id);
        uint32_t constituentOffset;
        std::tie(type, constituentOffset) = pgm->getConstituentInfo(type, j);
        address += constituentOffset;
    }
    if(false) {
        std::cout << "accesschain of " << basePointer.address << " yielded " << address << "\n";
    }
    uint32_t pointedType = std::get<TypePointer>(pgm->types.at(insn.type)).type;
    pointers[insn.resultId] = Pointer { pointedType, basePointer.storageClass, address };
}

void Interpreter::stepFunctionParameter(const InsnFunctionParameter& insn)
{
    uint32_t sourceId = callstack.back(); callstack.pop_back();
    // XXX is this ever a register?
    pointers[insn.resultId] = pointers[sourceId];
    if(false) std::cout << "function parameter " << insn.resultId << " receives " << sourceId << "\n";
}

void Interpreter::stepKill(const InsnKill& insn)
{
    pc = EXECUTION_ENDED;
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

    Register &value = registers.at(insn.valueId);
#ifdef CHECK_REGISTER_ACCESS
    if (!value.initialized) {
        std::cerr << "Warning: Returning uninitialized register " << insn.valueId << "\n";
    }
#endif

    registers[returnId] = value;

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

            float p0 = fromRegister<float>(insn.p0Id);
            float p1 = fromRegister<float>(insn.p1Id);
            float radicand = (p1 - p0) * (p1 - p0);
            toRegister<float>(insn.resultId) = sqrtf(radicand);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* p0 = &fromRegister<float>(insn.p0Id);
            const float* p1 = &fromRegister<float>(insn.p1Id);
            float radicand = 0;
            for(int i = 0; i < type.count; i++) {
                radicand += (p1[i] - p0[i]) * (p1[i] - p0[i]);
            }
            toRegister<float>(insn.resultId) = sqrtf(radicand);

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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = fabsf(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float length = 0;
            for(int i = 0; i < type.count; i++) {
                length += x[i]*x[i];
            }
            toRegister<float>(insn.resultId) = sqrtf(length);

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

            float x = fromRegister<float>(insn.xId);
            float y = fromRegister<float>(insn.yId);
            toRegister<float>(insn.resultId) = fmaxf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            const float* y = &fromRegister<float>(insn.yId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            float y = fromRegister<float>(insn.yId);
            toRegister<float>(insn.resultId) = fminf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            const float* y = &fromRegister<float>(insn.yId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            float y = fromRegister<float>(insn.yId);
            toRegister<float>(insn.resultId) = powf(x, y);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            const float* y = &fromRegister<float>(insn.yId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = x < 0 ? -1 : 1;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float length = 0;
            for(int i = 0; i < type.count; i++) {
                length += x[i]*x[i];
            }
            length = sqrtf(length);

            float* result = &toRegister<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = length == 0 ? 0 : x[i]/length;
            }

        } else {

            std::cout << "Unknown type for Normalize\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Radians(const InsnGLSLstd450Radians& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float degrees = fromRegister<float>(insn.degreesId);
            toRegister<float>(insn.resultId) = degrees / 180.0 * M_PI;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* degrees = &fromRegister<float>(insn.degreesId);
            float* result = &toRegister<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = degrees[i] / 180.0 * M_PI;
            }

        } else {

            std::cout << "Unknown type for Radians\n";

        }
    }, pgm->types.at(registers[insn.degreesId].type));
}

void Interpreter::stepGLSLstd450Sin(const InsnGLSLstd450Sin& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = sin(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = cos(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float y_over_x = fromRegister<float>(insn.y_over_xId);
            toRegister<float>(insn.resultId) = atanf(y_over_x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* y_over_x = &fromRegister<float>(insn.y_over_xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float y = fromRegister<float>(insn.yId);
            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = atan2f(y, x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* y = &fromRegister<float>(insn.yId);
            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = atan2f(y[i], x[i]);
            }

        } else {

            std::cout << "Unknown type for Atan2\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FSign(const InsnGLSLstd450FSign& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = (x < 0.0f) ? -1.0f : ((x == 0.0f) ? 0.0f : 1.0f);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = x[i] < 0.0f ? -1.0f : ((x[i] == 0.0f) ? 0.0f : 1.0f);
            }

        } else {

            std::cout << "Unknown type for FSign\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450Sqrt(const InsnGLSLstd450Sqrt& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = sqrtf(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
            for(int i = 0; i < type.count; i++) {
                result[i] = sqrtf(x[i]);
            }

        } else {

            std::cout << "Unknown type for Sqrt\n";

        }
    }, pgm->types.at(registers[insn.xId].type));
}

void Interpreter::stepGLSLstd450FAbs(const InsnGLSLstd450FAbs& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = fabsf(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = expf(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = exp2f(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = floor(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = x - floor(x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            float minVal = fromRegister<float>(insn.minValId);
            float maxVal = fromRegister<float>(insn.maxValId);
            toRegister<float>(insn.resultId) = fclamp(x, minVal, maxVal);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            const float* minVal = &fromRegister<float>(insn.minValId);
            const float* maxVal = &fromRegister<float>(insn.maxValId);
            float* result = &toRegister<float>(insn.resultId);
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

            float x = fromRegister<float>(insn.xId);
            float y = fromRegister<float>(insn.yId);
            float a = fromRegister<float>(insn.aId);
            toRegister<float>(insn.resultId) = fmix(x, y, a);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* x = &fromRegister<float>(insn.xId);
            const float* y = &fromRegister<float>(insn.yId);
            const float* a = &fromRegister<float>(insn.aId);
            float* result = &toRegister<float>(insn.resultId);
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

            float edge0 = fromRegister<float>(insn.edge0Id);
            float edge1 = fromRegister<float>(insn.edge1Id);
            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = smoothstep(edge0, edge1, x);

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* edge0 = &fromRegister<float>(insn.edge0Id);
            const float* edge1 = &fromRegister<float>(insn.edge1Id);
            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            float edge = fromRegister<float>(insn.edgeId);
            float x = fromRegister<float>(insn.xId);
            toRegister<float>(insn.resultId) = x < edge ? 0.0 : 1.0;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* edge = &fromRegister<float>(insn.edgeId);
            const float* x = &fromRegister<float>(insn.xId);
            float* result = &toRegister<float>(insn.resultId);
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

            const float* x = &fromRegister<float>(insn.xId);
            const float* y = &fromRegister<float>(insn.yId);
            float* result = &toRegister<float>(insn.resultId);

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

            float i = fromRegister<float>(insn.iId);
            float n = fromRegister<float>(insn.nId);

            float dot = n*i;

            toRegister<float>(insn.resultId) = i - 2.0*dot*n;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* i = &fromRegister<float>(insn.iId);
            const float* n = &fromRegister<float>(insn.nId);
            float* result = &toRegister<float>(insn.resultId);

            float dot = dotProduct(n, i, type.count);

            for (int k = 0; k < type.count; k++) {
                result[k] = i[k] - 2.0*dot*n[k];
            }

        } else {

            std::cout << "Unknown type for Reflect\n";

        }
    }, pgm->types.at(registers[insn.iId].type));
}

void Interpreter::stepGLSLstd450Refract(const InsnGLSLstd450Refract& insn)
{
    std::visit([this, &insn](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            float i = fromRegister<float>(insn.iId);
            float n = fromRegister<float>(insn.nId);
            float eta = fromRegister<float>(insn.etaId);

            float dot = n*i;

            float k = 1.0 - eta * eta * (1.0 - dot * dot);

            if(k < 0.0)
                toRegister<float>(insn.resultId) = 0.0;
            else
                toRegister<float>(insn.resultId) = eta * i - (eta * dot + sqrtf(k)) * n;

        } else if constexpr (std::is_same_v<T, TypeVector>) {

            const float* i = &fromRegister<float>(insn.iId);
            const float* n = &fromRegister<float>(insn.nId);
            float eta = fromRegister<float>(insn.etaId);
            float* result = &toRegister<float>(insn.resultId);

            float dot = dotProduct(n, i, type.count);

            float k = 1.0 - eta * eta * (1.0 - dot * dot);

            if(k < 0.0) {
                for (int m = 0; m < type.count; m++) {
                    result[m] = 0.0;
                }
            } else {
                for (int m = 0; m < type.count; m++) {
                    result[m] = eta * i[m] - (eta * dot + sqrtf(k)) * n[m];
                }
            }

        } else {

            std::cout << "Unknown type for Refract\n";

        }
    }, pgm->types.at(registers[insn.iId].type));
}

void Interpreter::stepBranch(const InsnBranch& insn)
{
    pc = pgm->labels.at(insn.targetLabelId);
}

void Interpreter::stepBranchConditional(const InsnBranchConditional& insn)
{
    bool condition = fromRegister<bool>(insn.conditionId);
    pc = pgm->labels.at(condition ? insn.trueLabelId : insn.falseLabelId);
}

void Interpreter::stepPhi(const InsnPhi& insn)
{
    Register& obj = registers.at(insn.resultId);
    uint32_t size = pgm->typeSizes.at(obj.type);

    bool found = false;
    for(int i = 0; !found && i < insn.operandId.size(); i += 2) {
        uint32_t srcId = insn.operandId[i];
        uint32_t parentId = insn.operandId[i + 1];

        if (parentId == previousBlockId) {
            const Register &src = registers.at(srcId);
#ifdef CHECK_REGISTER_ACCESS
            if (!src.initialized) {
                std::cerr << "Warning: Phi uninitialized register " << srcId << "\n";
            }
#endif
            std::copy(src.data, src.data + size, obj.data);
            found = true;
        }
    }

    if (found) {
        obj.initialized = true;
    } else {
        std::cout << "Error: Phi didn't find any label, previous " << previousBlockId
            << ", current " << currentBlockId << "\n";
        for(int i = 0; i < insn.operandId.size(); i += 2) {
            std::cout << "    " << insn.operandId[i + 1] << "\n";
        }
    }
}

float applyAddressMode(float f, Sampler::AddressMode mode)
{
    if(mode == Sampler::CLAMP_TO_EDGE)
        return std::clamp(f, 0.0f, 1.0f);
    if(mode == Sampler::REPEAT) {
        float wrapped = (f >= 0) ? fmodf(f, 1.0f) : (1 + fmodf(f, 1.0f));
        if(wrapped == 1.0f)
            wrapped = 0.0f;
        return wrapped;
    }
    return f;
}

// XXX implicit LOD level and thus texel interpolants
void Interpreter::stepImageSampleImplicitLod(const InsnImageSampleImplicitLod& insn)
{
    // uint32_t type; // result type
    // uint32_t resultId; // SSA register for result value
    // uint32_t sampledImageId; // operand from register
    // uint32_t coordinateId; // operand from register
    // uint32_t imageOperands; // ImageOperands (optional)
    v4float rgba;

    // Sample the image
    std::visit([this, &insn, &rgba](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeVector>) {

            assert(type.count == 2);

            auto [u, v] = fromRegister<v2float>(insn.coordinateId);

            int imageIndex = fromRegister<int>(insn.sampledImageId);
            const SampledImage& si = pgm->sampledImages[imageIndex];
            const ImagePtr image = si.image;

            unsigned int s, t;

            u = applyAddressMode(u, si.sampler.uAddressMode);
            v = applyAddressMode(v, si.sampler.vAddressMode);

            if(si.sampler.filterMode == Sampler::NEAREST /* && si.sampler.mipMapMode == Sampler::MIPMAP_NEAREST */) {

                s = static_cast<unsigned int>(u * image->width);
                t = static_cast<unsigned int>(v * image->height);
                image->get(s, image->height - 1 - t, rgba);

            } else if(si.sampler.filterMode == Sampler::LINEAR /* && si.sampler.mipMapMode == Sampler::MIPMAP_NEAREST */) {

                s = static_cast<unsigned int>(u * image->width);
                t = static_cast<unsigned int>(v * image->height);
                float alpha = u * image->width - s;
                float beta = v * image->height - t;
                unsigned int s0 = (s + 0) % image->width;
                unsigned int s1 = (s + 1) % image->width;
                unsigned int t0 = (t + 0) % image->height;
                unsigned int t1 = (t + 1) % image->height;
                v4float s0t0, s1t0, s0t1, s1t1;
                image->get(s0, image->height - 1 - t0, s0t0);
                image->get(s1, image->height - 1 - t0, s1t0);
                image->get(s0, image->height - 1 - t1, s0t1);
                image->get(s1, image->height - 1 - t1, s1t1);
                for(int i = 0; i < 4; i++)
                    rgba[i] =
                        (s0t0[i] * (1 - alpha) + s1t0[i] * alpha) * (1 - beta) +
                        (s0t1[i] * (1 - alpha) + s1t1[i] * alpha) * beta;
            } else {
            }


        } else {

            std::cout << "Unhandled type for ImageSampleImplicitLod coordinate\n";

        }
    }, pgm->types.at(registers[insn.coordinateId].type));

    uint32_t resultType = std::get<TypeVector>(pgm->types.at(registers[insn.resultId].type)).type;

    // Store the sample result in register
    std::visit([this, &insn, rgba](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            toRegister<v4float>(insn.resultId) = rgba;

        // else if constexpr (std::is_same_v<T, TypeInt>)


        } else {

            std::cout << "Unhandled type for ImageSampleImplicitLod result\n";

        }
    }, pgm->types.at(resultType));
}

// XXX explicit LOD level and thus texel interpolants
void Interpreter::stepImageSampleExplicitLod(const InsnImageSampleExplicitLod& insn)
{
    // uint32_t type; // result type
    // uint32_t resultId; // SSA register for result value
    // uint32_t sampledImageId; // operand from register
    // uint32_t coordinateId; // operand from register
    // uint32_t imageOperands; // ImageOperands (optional)
    v4float rgba;

    // Sample the image
    std::visit([this, &insn, &rgba](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeVector>) {

            assert(type.count == 2);

            auto [u, v] = fromRegister<v2float>(insn.coordinateId);

            int imageIndex = fromRegister<int>(insn.sampledImageId);
            const SampledImage& si = pgm->sampledImages[imageIndex];

            unsigned int s, t;

            if(si.sampler.uAddressMode == Sampler::CLAMP_TO_EDGE) {
                s = std::clamp(static_cast<unsigned int>(u * si.image->width), 0u, si.image->width - 1);
            } else {
                float wrapped = (u >= 0) ? fmodf(u, 1.0f) : (1 + fmodf(u, 1.0f));
                if(wrapped == 1.0f) /* fmodf of a negative number could return 0, after which "wrapped" would be 1 */
                    wrapped = 0.0f;
                s = static_cast<unsigned int>(wrapped * si.image->width);
            }

            if(si.sampler.vAddressMode == Sampler::CLAMP_TO_EDGE) {
                t = std::clamp(static_cast<unsigned int>(v * si.image->height), 0u, si.image->height - 1);
            } else {
                float wrapped = (v >= 0) ? fmodf(v, 1.0f) : (1 + fmodf(v, 1.0f));
                if(wrapped == 1.0f) /* fmodf of a negative number could return 0, after which "wrapped" would be 1 */
                    wrapped = 0.0f;
                t = static_cast<unsigned int>(wrapped * si.image->height);
            }

            si.image->get(s, si.image->height - 1 - t, rgba);

        } else {

            std::cout << "Unhandled type for ImageSampleExplicitLod coordinate\n";

        }
    }, pgm->types.at(registers[insn.coordinateId].type));

    uint32_t resultType = std::get<TypeVector>(pgm->types.at(registers[insn.resultId].type)).type;

    // Store the sample result in register
    std::visit([this, &insn, rgba](auto&& type) {

        using T = std::decay_t<decltype(type)>;

        if constexpr (std::is_same_v<T, TypeFloat>) {

            toRegister<v4float>(insn.resultId) = rgba;

        // else if constexpr (std::is_same_v<T, TypeInt>)


        } else {

            std::cout << "Unhandled type for ImageSampleExplicitLod result\n";

        }
    }, pgm->types.at(resultType));
}


void Interpreter::step()
{
    if(false) std::cout << "address " << pc << "\n";

    Instruction *instruction = pgm->instructions.at(pc++).get();

    // Update our idea of what block we're in. If we just switched blocks,
    // remember the previous one (for Phi).
    uint32_t thisBlockId = instruction->blockId;
    if (thisBlockId != currentBlockId) {
        // I'm not sure this is fully correct. For example, when returning
        // from a function this will set previousBlockId to a block in
        // the called function, but that's not right, it should always
        // point to a block within the current function. I don't think that
        // matters in practice because of the restricted locations where Phi
        // is placed.
        if(false) {
            std::cout << "Previous " << previousBlockId << ", current "
                << currentBlockId << ", new " << thisBlockId << "\n";
        }
        previousBlockId = currentBlockId;
        currentBlockId = thisBlockId;
    }

    instruction->step(this);
}

template <class T>
void Interpreter::set(SpvStorageClass clss, size_t offset, const T& v)
{
    objectInClassAt<T>(clss, offset, false, sizeof(v)) = v;
}

template <class T>
void Interpreter::set(const std::string& name, const T& v)
{
    if(pgm->namedVariables.find(name) != pgm->namedVariables.end()) {
        const VariableInfo& info = pgm->namedVariables.at(name);
        if(false) {
            std::cout << "set variable " << name << " at address " << info.address << '\n';
        }
        assert(info.size == sizeof(T));
        objectInMemoryAt<T>(info.address, false, sizeof(v)) = v;
    } else {
        std::cerr << "couldn't find variable \"" << name << "\" in Interpreter::set (may have been optimized away)\n";
    }
}

template <class T>
void Interpreter::get(SpvStorageClass clss, size_t offset, T& v)
{
    v = objectInClassAt<T>(clss, offset, true, sizeof(v));
}

void Interpreter::run()
{
    currentBlockId = NO_BLOCK_ID;
    previousBlockId = NO_BLOCK_ID;

    // Copy constants to memory. They're treated like variables.
    for(auto& [id, constant]: pgm->constants) {
        registers[id] = constant;
    }

    // init Function variables with initializers before each invocation
    // XXX also need to initialize within function calls?
    for(auto& [id, var]: pgm->variables) {
        pointers[id] = Pointer { var.type, var.storageClass, var.address };
        if(var.storageClass == SpvStorageClassFunction) {
            assert(var.initializer == NO_INITIALIZER); // XXX will do initializers later
        }
    }

    callstack.clear();
    callstack.push_back(EXECUTION_ENDED); // caller PC
    callstack.push_back(NO_RETURN_REGISTER); // return register
    pc = pgm->mainFunction->start;

    do {
        step();
    } while(pc != EXECUTION_ENDED);
}

// -----------------------------------------------------------------------------------

// Virtual register used by the compiler.
struct CompilerRegister {
    // Type of the data.
    uint32_t type;

    // Number of words of data in this register.
    int count;

    // Physical registers.
    std::vector<uint32_t> phy;

    CompilerRegister()
        : type(NO_REGISTER)
    {
        // Nothing.
    }

    CompilerRegister(uint32_t type, int count)
        : type(type), count(count)
    {
        // Nothing.
    }
};

// Compiles a Program to our ISA.
struct Compiler
{
    const Program *pgm;
    std::map<uint32_t, CompilerRegister> registers;
    uint32_t localLabelCounter;
    InstructionList instructions;

    Compiler(const Program *pgm)
        : pgm(pgm), localLabelCounter(1)
    {
        // Nothing.
    }

    void compile()
    {
        // Transform SPIR-V instructions to RISC-V instructions.
        transformInstructions(pgm->instructions, instructions);

        // Perform physical register assignment.
        assignRegisters();

        // Emit our header.
        emit("jal ra, main", "");
        emit("ebreak", "");

        // Emit instructions.
        for(int pc = 0; pc < instructions.size(); pc++) {
            for(auto &function : pgm->functions) {
                if(pc == function.second.start) {
                    std::string name = pgm->cleanUpFunctionName(function.first);
                    std::cout << "; ---------------------------- function \"" << name << "\"\n";
                    emitLabel(name);

                    // Emit instructions to fill constants.
                    for (auto regId : pgm->instructions.at(function.second.start)->livein) {
                        auto r = registers.find(regId);
                        assert(r != registers.end());
                        assert(r->second.phy.size() == 1);
                        std::ostringstream ss;
                        if (isRegFloat(regId)) {
                            ss << "flw f" << (r->second.phy.at(0) - 32);
                        } else {
                            ss << "lw x" << r->second.phy.at(0);
                        }
                        ss << ", .C" << regId << "(x0)";
                        emit(ss.str(), "Load constant");
                    }
                }
            }

            for(auto &label : pgm->labels) {
                if(pc == label.second) {
                    std::ostringstream ss;
                    ss << "label" << label.first;
                    emitLabel(ss.str());
                }
            }

            instructions.at(pc)->emit(this);
        }

        // Emit variables.
        for (auto &[id, var] : pgm->variables) {
            auto name = pgm->names.find(id);
            if (name != pgm->names.end()) {
                // XXX Check storage class? (var.storageClass)
                emitLabel(name->second);
                size_t size = pgm->typeSizes.at(var.type);
                for (size_t i = 0; i < size/4; i++) {
                    emit(".word 0", "");
                }
                for (size_t i = 0; i < size%4; i++) {
                    emit(".byte 0", "");
                }
            } else {
                std::cerr << "Warning: Name of variable " << id << " not defined.\n";
            }
        }

        // Emit constants.
        for (auto &[id, reg] : pgm->constants) {
            std::string name;
            auto nameItr = pgm->names.find(id);
            if (nameItr == pgm->names.end()) {
                std::ostringstream ss;
                ss << ".C" << id;
                name = ss.str();
            } else {
                name = nameItr->second;
            }
            emitLabel(name);
            emitConstant(id, reg.type, reg.data);
        }
    }

    // Emit constant value for the specified constant ID.
    void emitConstant(uint32_t id, uint32_t typeId, unsigned char *data) {
        Type type = pgm->types.at(typeId);

        if (std::holds_alternative<TypeInt>(type)) {
            uint32_t value = *reinterpret_cast<uint32_t *>(data);
            std::ostringstream ss;
            ss << ".word " << value;
            emit(ss.str(), "");
        } else if (std::holds_alternative<TypeFloat>(type)) {
            uint32_t intValue = *reinterpret_cast<uint32_t *>(data);
            float floatValue = *reinterpret_cast<float *>(data);
            std::ostringstream ss1;
            ss1 << ".word 0x" << std::hex << intValue;
            std::ostringstream ss2;
            ss2 << "Float " << floatValue;
            emit(ss1.str(), ss2.str());
        } else if (std::holds_alternative<TypeVector>(type)) {
            const TypeVector *typeVector = pgm->getTypeAsVector(typeId);
            for (int i = 0; i < typeVector->count; i++) {
                auto [subtype, offset] = pgm->getConstituentInfo(typeId, i);
                emitConstant(id, subtype, data + offset);
            }
        } else {
            std::cerr << "Error: Unhandled type for constant " << id << ".\n";
            exit(EXIT_FAILURE);
        }
    }

    // If the virtual register "id" points to an integer constant, returns it
    // in "value" and returns true. Otherwise returns false and leaves value
    // untouched.
    bool asIntegerConstant(uint32_t id, uint32_t &value) const {
        auto r = pgm->constants.find(id);
        if (r != pgm->constants.end()) {
            Type type = pgm->types.at(r->second.type);
            if (std::holds_alternative<TypeInt>(type)) {
                value = *reinterpret_cast<uint32_t *>(r->second.data);
                return true;
            }
        }

        return false;
    }

    // Returns true if the register is a float, false if it's an integer,
    // otherwise asserts.
    bool isRegFloat(uint32_t id) const {
        auto r = registers.find(id);
        assert(r != registers.end());

        return pgm->isTypeFloat(r->second.type);
    }

    // Make a new label that can be used for local jumps.
    std::string makeLocalLabel() {
        std::ostringstream ss;
        ss << "local" << localLabelCounter;
        localLabelCounter++;
        return ss.str();
    }

    // Transform SPIR-V instructions to RISC-V instructions. Creates a new
    // "instructions" array that's local to the compiler and is closer to
    // machine instructions, but still using SSA. inList and outList
    // may be the same list.
    void transformInstructions(const InstructionList &inList, InstructionList &outList) {
        InstructionList newList;

        for (uint32_t pc = 0; pc < inList.size(); pc++) {
            bool replaced = false;
            const std::shared_ptr<Instruction> &instructionPtr = inList[pc];
            Instruction *instruction = instructionPtr.get();
            if (instruction->opcode() == SpvOpIAdd) {
                InsnIAdd *insnIAdd = dynamic_cast<InsnIAdd *>(instruction);

                uint32_t imm;
                if (asIntegerConstant(insnIAdd->operand1Id, imm)) {
                    // XXX Verify that immediate fits in 12 bits.
                    newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                                insnIAdd->type, insnIAdd->resultId, insnIAdd->operand2Id, imm));
                    replaced = true;
                } else if (asIntegerConstant(insnIAdd->operand2Id, imm)) {
                    // XXX Verify that immediate fits in 12 bits.
                    newList.push_back(std::make_shared<RiscVAddi>(insnIAdd->lineInfo,
                                insnIAdd->type, insnIAdd->resultId, insnIAdd->operand1Id, imm));
                    replaced = true;
                }
            }

            if (!replaced) {
                newList.push_back(instructionPtr);
            }
        }

        std::swap(newList, outList);
    }

    void assignRegisters() {
        // Set up all the virtual registers we'll deal with.
        for(auto [id, type]: pgm->resultTypes) {
            const TypeVector *typeVector = pgm->getTypeAsVector(type);
            int count = typeVector == nullptr ? 1 : typeVector->count;
            registers[id] = CompilerRegister {type, count};
        }

        // 32 registers; x0 is always zero; x1 is ra.
        std::set<uint32_t> PHY_INT_REGS;
        for (int i = 2; i < 32; i++) {
            PHY_INT_REGS.insert(i);
        }

        // 32 float registers.
        std::set<uint32_t> PHY_FLOAT_REGS;
        for (int i = 0; i < 32; i++) {
            PHY_FLOAT_REGS.insert(i + 32);
        }

        // Start with blocks at the start of functions.
        for (auto& [id, function] : pgm->functions) {
            Block *block = pgm->blocks.at(function.labelId).get();

            // Assign registers for constants.
            std::set<uint32_t> constIntRegs = PHY_INT_REGS;
            std::set<uint32_t> constFloatRegs = PHY_FLOAT_REGS;
            for (auto regId : instructions.at(block->begin)->livein) {
                std::cerr << regId << "\n";
                assert(registers.find(regId) == registers.end());
                auto &c = pgm->constants.at(regId);
                auto r = CompilerRegister {c.type, 1}; // XXX get real count.
                uint32_t phy;
                if (pgm->isTypeFloat(c.type)) {
                    assert(!constFloatRegs.empty());
                    phy = *constFloatRegs.begin();
                    constFloatRegs.erase(phy);
                } else {
                    assert(!constIntRegs.empty());
                    phy = *constIntRegs.begin();
                    constIntRegs.erase(phy);
                }
                r.phy.push_back(phy);
                registers[regId] = r;
            }

            assignRegisters(block, PHY_INT_REGS, PHY_FLOAT_REGS);
        }
    }

    // Assigns registers for this block.
    void assignRegisters(Block *block,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy) {

        if (pgm->verbose) {
            std::cout << "assignRegisters(" << block->labelId << ")\n";
        }

        // Assigned physical registers.
        std::set<uint32_t> assigned;

        // Registers that are live going into this block have already been
        // assigned.
        for (auto regId : instructions.at(block->begin)->livein) {
            auto r = registers.find(regId);
            if (r == registers.end()) {
                std::cerr << "Warning: Initial virtual register "
                    << regId << " not found in block " << block->labelId << ".\n";
            } else if (r->second.phy.empty()) {
                std::cerr << "Warning: Expected initial physical register for "
                    << regId << " in block " << block->labelId << ".\n";
            } else {
                for (auto phy : r->second.phy) {
                    assigned.insert(phy);
                }
            }
        }

        for (int pc = block->begin; pc < block->end; pc++) {
            Instruction *instruction = instructions.at(pc).get();

            // Free up now-unused physical registers.
            for (auto argId : instruction->argIds) {
                // If this virtual register doesn't survive past this line, then we
                // can use its physical register again.
                if (instruction->liveout.find(argId) == instruction->liveout.end()) {
                    auto r = registers.find(argId);
                    if (r != registers.end()) {
                        for (auto phy : r->second.phy) {
                            assigned.erase(phy);
                        }
                    }
                }
            }

            // Assign result registers to physical registers.
            uint32_t resId = instruction->resId;
            if (resId != NO_REGISTER) {
                auto r = registers.find(resId);
                if (r == registers.end()) {
                    std::cout << "Error: Virtual register "
                        << resId << " not found in block " << block->labelId << ".\n";
                    exit(EXIT_FAILURE);
                }

                // Register set to pick from for this register.
                const std::set<uint32_t> &allPhy =
                    pgm->isTypeFloat(r->second.type) ? allFloatPhy : allIntPhy;

                // For each element in this virtual register...
                for (int i = 0; i < r->second.count; i++) {
                    // Find an available physical register for this element.
                    bool found = false;
                    for (uint32_t phy : allPhy) {
                        if (assigned.find(phy) == assigned.end()) {
                            r->second.phy.push_back(phy);
                            // If the result lives past this instruction, consider its
                            // register assigned.
                            if (instruction->liveout.find(resId) != instruction->liveout.end()) {
                                assigned.insert(phy);
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        std::cout << "Error: No physical register available for "
                            << instruction->resId << "[" << i << "] on line " << pc << ".\n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // Go down dominance tree.
        for (auto& [labelId, subBlock] : pgm->blocks) {
            if (subBlock->idom == block->labelId) {
                assignRegisters(subBlock.get(), allIntPhy, allFloatPhy);
            }
        }
    }

    // Determine whether these two virtual register IDs are currently assigned
    // to the same physical register. Returns false if one or both aren't
    // assigned at all. Index is the index into the vector, if it is one.
    bool isSamePhysicalRegister(uint32_t id1, uint32_t id2, int index) const {
        auto r1 = registers.find(id1);
        if (r1 != registers.end() && index < r1->second.phy.size()) {
            auto r2 = registers.find(id2);
            if (r2 != registers.end() && index < r2->second.phy.size()) {
                return r1->second.phy[index] == r2->second.phy[index];
            }
        }

        return false;
    }

    // Returns the SSA ID as a compiler register, or null if it's not stored
    // in a compiler register.
    const CompilerRegister *asRegister(uint32_t id) const {
        auto r = registers.find(id);

        return r == registers.end() ? nullptr : &r->second;
    }

    // String for a virtual register ("r12" or more).
    std::string reg(uint32_t id, int index = 0) const {
        std::ostringstream ss;

        auto r = asRegister(id);
        if (r != nullptr) {
            uint32_t phy = r->phy[index];
            if (phy < 32) {
                ss << "x" << phy;
            } else {
                ss << "f" << (phy - 32);
            }
        } else {
            auto constant = pgm->constants.find(id);
            if (constant != pgm->constants.end() &&
                    std::holds_alternative<TypeFloat>(pgm->types.at(constant->second.type))) {

                const float *f = reinterpret_cast<float *>(constant->second.data);
                ss << *f;
            } else {
                auto name = pgm->names.find(id);
                if (name != pgm->names.end()) {
                    ss << name->second;
                } else {
                    ss << "r" << id;
                }
            }
        }

        return ss.str();
    }

    void emitNotImplemented(const std::string &op)
    {
        std::ostringstream ss;

        ss << op << " not implemented";

        emit("nop", ss.str());
    }

    void emitUnaryOp(const std::string &opName, int result, int op)
    {
        auto r = registers.find(result);
        int count = r == registers.end() ? 1 : r->second.count;

        for (int i = 0; i < count; i++) {
            std::ostringstream ss1;
            ss1 << opName << " " << reg(result, i) << ", " << reg(op, i);
            std::ostringstream ss2;
            ss2 << "r" << result << " = " << opName << " r" << op;
            emit(ss1.str(), ss2.str());
        }
    }

    void emitBinaryOp(const std::string &opName, int result, int op1, int op2)
    {
        auto r = registers.find(result);
        int count = r == registers.end() ? 1 : r->second.count;

        for (int i = 0; i < count; i++) {
            std::ostringstream ss1;
            ss1 << opName << " " << reg(result, i) << ", " << reg(op1, i) << ", " << reg(op2, i);
            std::ostringstream ss2;
            ss2 << "r" << result << " = " << opName << " r" << op1 << " r" << op2;
            emit(ss1.str(), ss2.str());
        }
    }

    void emitBinaryImmOp(const std::string &opName, int result, int op, uint32_t imm)
    {
        auto r = registers.find(result);
        int count = r == registers.end() ? 1 : r->second.count;

        for (int i = 0; i < count; i++) {
            std::ostringstream ss1;
            ss1 << opName << " " << reg(result, i) << ", " << reg(op, i) << ", " << imm;
            std::ostringstream ss2;
            ss2 << "r" << result << " = " << opName << " r" << op << " " << imm;
            emit(ss1.str(), ss2.str());
        }
    }

    void emitLabel(const std::string &label)
    {
        std::cout << notEmptyLabel(label) << ":\n";
    }

    // Return the label passed in, unless it's an empty string, in which case
    // it returns an appropriate non-empty string.
    std::string notEmptyLabel(const std::string &label) const {
        return label.empty() ? ".anonymous" : label;
    }

    void emit(const std::string &op, const std::string &comment)
    {
        std::ios oldState(nullptr);
        oldState.copyfmt(std::cout);

        std::cout
            << "        "
            << std::left
            << std::setw(30) << op
            << std::setw(0);
        if(!comment.empty()) {
            std::cout << "; " << comment;
        }
        std::cout << "\n";

        std::cout.copyfmt(oldState);
    }

    // Just before a Branch or BranchConditional instruction, copy any
    // registers that a target OpPhi instruction might need.
    void emitPhiCopy(Instruction *instruction, uint32_t labelId) {
        Block *block = pgm->blocks.at(labelId).get();
        for (int pc = block->begin; pc < block->end; pc++) {
            Instruction *nextInstruction = instructions[pc].get();
            if (nextInstruction->opcode() != SpvOpPhi) {
                // No more phi instructions for this block.
                break;
            }

            InsnPhi *phi = dynamic_cast<InsnPhi *>(nextInstruction);

            // Find the block corresponding to ours.
            bool found = false;
            for (int i = 0; !found && i < phi->operandId.size(); i += 2) {
                uint32_t srcId = phi->operandId[i];
                uint32_t parentId = phi->operandId[i + 1];

                if (parentId == instruction->blockId) {
                    found = true;

                    // XXX need to iterate over all registers of a vector.
                    std::ostringstream ss;
                    if (isSamePhysicalRegister(phi->resultId, srcId, 0)) {
                        ss << "; ";
                    }
                    ss << "mov " << reg(phi->resultId)
                        << ", " << reg(srcId);
                    emit(ss.str(), "phi elimination");
                }
            }
            if (!found) {
                std::cerr << "Error: Can't find source block " << instruction->blockId
                    << " in phi assigning to " << phi->resultId << "\n";
            }
        }
    }

    // Assert that the block at the label ID does not start with a Phi instruction.
    void assertNoPhi(uint32_t labelId) {
        Block *block = pgm->blocks.at(labelId).get();
        assert(instructions[block->begin]->opcode() != SpvOpPhi);
    }
};

// -----------------------------------------------------------------------------------

void RiscVAddi::emit(Compiler *compiler)
{
    compiler->emitBinaryImmOp("addi", resultId, rs1, imm);
}

void RiscVLoad::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    if (compiler->isRegFloat(resultId)) {
        ss1 << "flw ";
    } else {
        ss1 << "lw ";
    }
    ss1 << compiler->reg(resultId) << ", ";
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
    ss2 << "r" << resultId << " = (r" << pointerId << ")";
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

// -----------------------------------------------------------------------------------

Instruction::Instruction(const LineInfo& lineInfo_, uint32_t resId)
    : lineInfo(lineInfo_),
      blockId(NO_BLOCK_ID),
      resId(resId)
{
    // Nothing.
}

void Instruction::emit(Compiler *compiler)
{
    compiler->emitNotImplemented(name());
}

void InsnFAdd::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fadd.s", resultId, operand1Id, operand2Id);
}

void InsnFMul::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fmul.s", resultId, operand1Id, operand2Id);
}

void InsnFDiv::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("fdiv.s", resultId, operand1Id, operand2Id);
}

void InsnIAdd::emit(Compiler *compiler)
{
    uint32_t intValue;
    if (compiler->asIntegerConstant(operand1Id, intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId, operand2Id, intValue);
    } else if (compiler->asIntegerConstant(operand2Id, intValue)) {
        // XXX Verify that immediate fits in 12 bits.
        compiler->emitBinaryImmOp("addi", resultId, operand1Id, intValue);
    } else {
        compiler->emitBinaryOp("add", resultId, operand1Id, operand2Id);
    }
}

void InsnFunctionCall::emit(Compiler *compiler)
{
    compiler->emit("push pc", "");
    for(int i = operandId.size() - 1; i >= 0; i--) {
        compiler->emit(std::string("push ") + compiler->reg(operandId[i]), "");
    }
    compiler->emit(std::string("call ") + compiler->pgm->cleanUpFunctionName(functionId), "");
    compiler->emit(std::string("pop ") + compiler->reg(resultId), "");
}

void InsnFunctionParameter::emit(Compiler *compiler)
{
    compiler->emit(std::string("pop ") + compiler->reg(resultId), "");
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
    ss << "j label" << targetLabelId;
    compiler->emit(ss.str(), "");
}

void InsnReturn::emit(Compiler *compiler)
{
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnReturnValue::emit(Compiler *compiler)
{
    std::ostringstream ss1;
    ss1 << "mov x10, " << compiler->reg(valueId);
    std::ostringstream ss2;
    ss2 << "return " << valueId;
    compiler->emit(ss1.str(), ss2.str());
    compiler->emit("jalr x0, ra, 0", "");
}

void InsnPhi::emit(Compiler *compiler)
{
    compiler->emit("", "phi instruction, nothing to do.");
}

void InsnFOrdLessThanEqual::emit(Compiler *compiler)
{
    compiler->emitBinaryOp("lte", resultId, operand1Id, operand2Id);
}

void InsnBranchConditional::emit(Compiler *compiler)
{
    std::string localLabel = compiler->makeLocalLabel();

    std::ostringstream ss1;
    ss1 << "beq " << compiler->reg(conditionId)
        << ", x0, " << localLabel;
    std::ostringstream ssid;
    ssid << "r" << conditionId;
    compiler->emit(ss1.str(), ssid.str());
    // True path.
    compiler->emitPhiCopy(this, trueLabelId);
    std::ostringstream ss2;
    ss2 << "j label" << trueLabelId;
    compiler->emit(ss2.str(), "");
    // False path.
    compiler->emitLabel(localLabel);
    compiler->emitPhiCopy(this, falseLabelId);
    std::ostringstream ss3;
    ss3 << "j label" << falseLabelId;
    compiler->emit(ss3.str(), "");
}

void InsnAccessChain::emit(Compiler *compiler)
{
    uint32_t offset = 0;

    const Variable &variable = compiler->pgm->variables.at(baseId);
    uint32_t type = variable.type;
    for (auto &id : indexesId) {
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

    auto name = compiler->pgm->names.find(baseId);
    if (name == compiler->pgm->names.end()) {
        std::cerr << "Error: Don't yet handle pointer reference in AccessChain ("
            << baseId << ").\n";
        exit(EXIT_FAILURE);
    }

    std::ostringstream ss;
    ss << "addi " << compiler->reg(resultId) << ", x0, "
        << compiler->notEmptyLabel(name->second) << "+" << offset;
    compiler->emit(ss.str(), "");
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
    printf("\t--json    input file is a ShaderToy JSON file\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
}

struct ShaderToyImage
{
    int channelNumber;
    int id;
    SampledImage sampledImage;
};

struct ShaderSource
{
    std::string code;
    std::string filename;
    ShaderSource() {}
    ShaderSource(const std::string& code_, const std::string& filename_) :
        code(code_),
        filename(filename_)
    {}
};

struct CommandLineParameters
{
    int outputWidth;
    int outputHeight;
    bool beVerbose;
    bool throwOnUnimplemented;
};

const std::string shaderPreambleFilename = "preamble.frag";
const std::string shaderEpilogueFilename = "epilogue.frag";

struct RenderPass
{
    std::string name;
    std::vector<ShaderToyImage> inputs; // in channel order
    std::vector<ShaderToyImage> outputs;
    std::vector<ShaderSource> sources;
    Program pgm;
    void Render(void) {
        // set input images, uniforms, output images, call run()
    }
    RenderPass(const std::string& name_, std::vector<ShaderToyImage> inputs_, std::vector<ShaderToyImage> outputs_, const std::vector<ShaderSource>& sources_, const CommandLineParameters& params) :
        name(name_),
        inputs(inputs_),
        outputs(outputs_),
        sources(sources_),
        pgm(params.throwOnUnimplemented, params.beVerbose)
    {
    }
};

typedef std::shared_ptr<RenderPass> RenderPassPtr;


// Number of rows still left to shade (for progress report).
static std::atomic_int rowsLeft;

// Render rows starting at "startRow" every "skip".
void render(RenderPass* pass, int startRow, int skip, int frameNumber, float when)
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

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

std::string getFilepathAdjacentToPath(const std::string& filename, std::string adjacent)
{
    std::string result;

#ifdef USE_CPP17_FILESYSTEM

    // filesystem still not available in XCode 2019/04/04
    std::filesystem::path adjacent_path(adjacent);
    std::filesystem::path adjacent_dirname = adjacent_path.parent_path();
    std::filesystem::path code_path(filename);

    if(code_path.is_relative()) {
        std::filesystem::path full_path(adjacent_dirname + code_path);
        result = full_path;
    }

#else

    if(filename[0] != '/') {
        // Assume relative path, get directory from JSON filename
        char *adjacent_copy = strdup(adjacent.c_str());;
        result = std::string(dirname(adjacent_copy)) + "/" + filename;
        free(adjacent_copy);
    }

#endif

    return result;
}

void earwigMessageConsumer(spv_message_level_t level, const char *source,
        const spv_position_t& position, const char *message)
{
    std::cout << source << ": " << message << "\n";
}

void sortInDependencyOrder(
    const std::vector<RenderPassPtr>& passes,
    const std::map<int, RenderPassPtr>& channelIdsToPasses,
    const std::map<std::string, RenderPassPtr>& namesToPasses,
    std::vector<RenderPassPtr>& passesOrdered)
{
    std::set<RenderPassPtr> passesToVisit(passes.begin(), passes.end());

    std::function<void(RenderPassPtr)> visit;

    visit = [&visit, &passesToVisit, &channelIdsToPasses, &passesOrdered](RenderPassPtr pass) {
        if(passesToVisit.count(pass) > 0) {
            passesToVisit.erase(pass);
            for(ShaderToyImage& input: pass->inputs) {
                if(channelIdsToPasses.find(input.id) != channelIdsToPasses.end()) {
                    visit(channelIdsToPasses.at(input.id));
                }
            }
            passesOrdered.push_back(pass);
        }
    };

    visit(namesToPasses.at("Image"));
}

void getOrderedRenderPassesFromJSON(const std::string& filename, std::vector<RenderPassPtr>& renderPassesOrdered, const CommandLineParameters& params)
{
    json j = json::parse(readFileContents(filename));

    std::vector<RenderPassPtr> renderPasses;

    ShaderSource common_code;

    // Go through the render passes and load code in from files as necessary
    auto& renderPassesInJSON = j["Shader"]["renderpass"];

    for (auto& pass: renderPassesInJSON) {

        // If special key "codefile" is present, use that file as
        // the shader code instead of the value of key "code".

        if(pass.find("codefile") != pass.end()) {
            std::string code_filename = pass["codefile"];

            code_filename = getFilepathAdjacentToPath(code_filename, filename);

            pass["full_code_filename"] = code_filename;
            pass["code"] = readFileContents(code_filename);
        }

	// If this is the "common" pass, it includes text which is
	// preprended to all other passes.

        if(pass["type"] == "common") {
            common_code.code = pass["code"];
            if(pass.find("codefile") != pass.end()) {
                common_code.filename = pass["codefile"];
            } else {
                common_code.filename = pass["name"].get<std::string>() + " JSON inline";
            }
        } 
    }

    std::map<int, RenderPassPtr> channelIdsToPasses;
    std::map<std::string, RenderPassPtr> namesToPasses;

    for (auto& pass: renderPassesInJSON) {
        std::vector<ShaderSource> sources;

        if(pass["type"] == "common") {
            /* Don't make a renderpass for "common", it's just common preamble code for all passes */
            continue;
        }
        if((pass["type"] != "buffer") && (pass["type"] != "image")) {
            throw std::runtime_error("pass type \"" + pass["type"].get<std::string>() + "\" is not supported");
        }

        ShaderSource asset_preamble;
        ShaderSource shader;

        std::vector<ShaderToyImage> inputs;

        for(auto& input: pass["inputs"]) {
            int channelNumber = input["channel"].get<int>();
            int channelId = input["id"].get<int>();

            const std::string& src = input["src"].get<std::string>();

            bool isABuffer = (src.find("/media/previz/buffer") != std::string::npos);

            if(isABuffer) {

                /* will hook up to output from source pass */
                if(false) printf("pass \"%s\", channel number %d and id %d\n", pass["name"].get<std::string>().c_str(), channelNumber, channelId);
                Sampler sampler; /*  = makeSamplerFromJSON(input); */
                inputs.push_back({channelNumber, channelId, {nullptr, sampler}});
                asset_preamble.code += "layout (binding = " + std::to_string(channelNumber + 1) + ") uniform sampler2D iChannel" + std::to_string(channelNumber) + ";\n";

            } else if(input.find("locally_saved") == input.end()) {

                throw std::runtime_error("downloading assets is not supported; didn't find \"locally_saved\" key for an asset");

            } else {

                if(input["ctype"].get<std::string>() != "texture") {
                    throw std::runtime_error("unsupported input type " + input["ctype"].get<std::string>());
                }

                std::string asset_filename = getFilepathAdjacentToPath(input["locally_saved"].get<std::string>(), filename);

                bool flipInY = ((input.find("vflip") != input.end()) && (input["vflip"].get<std::string>() == std::string("true")));

                ImagePtr image = loadImage(asset_filename, flipInY);
                if(!image) {
                    std::cerr << "image load failed\n";
                    exit(EXIT_FAILURE);
                }

                Sampler sampler; /*  = makeSamplerFromJSON(input); */
                inputs.push_back({channelNumber, channelId, {image, sampler}});
                asset_preamble.code += "layout (binding = " + std::to_string(channelNumber + 1) + ") uniform sampler2D iChannel" + std::to_string(channelNumber) + ";\n";
            }
        }

        if(asset_preamble.code != "") {
            asset_preamble.filename = "BuiltIn Asset Preamble";
            sources.push_back(asset_preamble);
        }

        if(common_code.code != "") {
            sources.push_back(common_code);
        }

        if(pass.find("full_code_filename") != pass.end()) {
            // If we find "full_code_filename", that's the
            // fully-qualified absolute on-disk path we loaded.  Set
            // that for debugging.
            shader.filename = pass["full_code_filename"];
        } else if(pass.find("codefile") != pass.end()) {
            // If we find only "codefile", that's the on-disk path
            // the JSON specified.  Set that for debugging.
            shader.filename = pass["codefile"];
        } else {
            // If we find neither "codefile" nor "full_code_filename",
            // then the shader came out of the renderpass "code", so
            // at least use the pass name to aid debugging.
            shader.filename = std::string("JSON inline shader from pass ") + pass["name"].get<std::string>();
        }

        shader.code = pass["code"];
        sources.push_back(shader);

        int channelId = pass["outputs"][0]["id"].get<int>();
        Image::Format format = (pass["name"].get<std::string>() == "Image") ? Image::FORMAT_R8G8B8_UNORM : Image::FORMAT_R32G32B32A32_SFLOAT;
        ImagePtr image(new Image(format, Image::DIM_2D, params.outputWidth, params.outputHeight));
        ShaderToyImage output {0, channelId, { image, Sampler {}}};
        RenderPassPtr rpass(new RenderPass(
            pass["name"],
            {inputs},
            {output},
            sources,
            params));

        channelIdsToPasses[channelId] = rpass;
        namesToPasses[pass["name"]] = rpass;
        renderPasses.push_back(rpass);
    }

    // Hook up outputs to inputs
    for(auto& pass: renderPasses) {
        for(int i = 0; i < pass->inputs.size(); i++) {
            ShaderToyImage& input = pass->inputs[i];
            if(!input.sampledImage.image) {
                int channelId = input.id;
                ShaderToyImage& source = channelIdsToPasses.at(channelId)->outputs[0];
                pass->inputs[i].sampledImage = source.sampledImage;
            }
        }
    }
    
    // and sort in dependency order
    sortInDependencyOrder(renderPasses, channelIdsToPasses, namesToPasses, renderPassesOrdered);
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
            spv::Disassemble(std::cout, spirv);
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

    std::vector<RenderPassPtr> renderPasses;

    std::string filename = argv[0];
    std::string shader_code;

    if(!inputIsJSON) {

        shader_code = readFileContents(filename);

        ImagePtr image(new Image(Image::FORMAT_R8G8B8_UNORM, Image::DIM_2D, params.outputWidth, params.outputHeight));
        ShaderToyImage output {0, 0, {image, Sampler {}}};

        std::vector<ShaderSource> sources = { {"", ""}, {shader_code, filename}};
        RenderPassPtr pass(new RenderPass("Image", {}, {output}, sources, params));

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
