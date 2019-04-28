
#include <vector>
#include <map>
#include <set>

#include "program.h"
#include "interpreter.h"
#include "interpreter_tmpl.h"

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
        pgm->types.at(insn.type)->dump(obj.data);
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
        ConstituentInfo info = pgm->getConstituentInfo(type, j);
        type = info.subtype;
        offset += info.offset;
    }
    std::copy(src.data + offset, src.data + offset + pgm->typeSizes.at(obj.type), obj.data);
    obj.initialized = true;
    if(false) {
        std::cout << "extracted from ";
        pgm->types.at(src.type)->dump(src.data);
        std::cout << " result is ";
        pgm->types.at(insn.type)->dump(obj.data);
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
        ConstituentInfo info = pgm->getConstituentInfo(type, j);
        type = info.subtype;
        offset += info.offset;
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
        pgm->types.at(obj.type)->dump(obj.data);
        std::cout << "\n";
    }
}

void Interpreter::stepIAdd(const InsnIAdd& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
        const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
        uint32_t* result = &toRegister<uint32_t>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] + operand2[i];
        }
    } else {
        uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
        uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
        uint32_t result = operand1 + operand2;
        toRegister<uint32_t>(insn.resultId) = result;
    }
}

void Interpreter::stepISub(const InsnISub& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
        const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
        uint32_t* result = &toRegister<uint32_t>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] - operand2[i];
        }
    } else {
        uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
        uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
        uint32_t result = operand1 - operand2;
        toRegister<uint32_t>(insn.resultId) = result;
    }
}

void Interpreter::stepFAdd(const InsnFAdd& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] + operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        float result = operand1 + operand2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepFSub(const InsnFSub& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] - operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        float result = operand1 - operand2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepFMul(const InsnFMul& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] * operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        float result = operand1 * operand2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepFDiv(const InsnFDiv& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] / operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        float result = operand1 / operand2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepFMod(const InsnFMod& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] - floor(operand1[i]/operand2[i])*operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        float result = operand1 - floor(operand1/operand2)*operand2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepFOrdLessThan(const InsnFOrdLessThan& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] < operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        bool result = operand1 < operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepFOrdGreaterThan(const InsnFOrdGreaterThan& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] > operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        bool result = operand1 > operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepFOrdLessThanEqual(const InsnFOrdLessThanEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] <= operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        bool result = operand1 <= operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepFOrdEqual(const InsnFOrdEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] == operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        // XXX I don't know the difference between ordered and equal
        // vs. unordered and equal, so I don't know which this is.
        bool result = operand1 == operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepFNegate(const InsnFNegate& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand = &fromRegister<float>(insn.operandId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = -operand[i];
        }
    } else {
        toRegister<float>(insn.resultId) = -fromRegister<float>(insn.operandId);
    }
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
    const Register &r1 = registers[insn.vector1Id];
    const TypeVector *t1 = dynamic_cast<const TypeVector *>(pgm->types.at(r1.type).get());

    const float* vector1 = &fromRegister<float>(insn.vector1Id);
    const float* vector2 = &fromRegister<float>(insn.vector2Id);
    toRegister<float>(insn.resultId) = dotProduct(vector1, vector2, t1->count);
}

void Interpreter::stepFOrdGreaterThanEqual(const InsnFOrdGreaterThanEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* operand1 = &fromRegister<float>(insn.operand1Id);
        const float* operand2 = &fromRegister<float>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] >= operand2[i];
        }
    } else {
        float operand1 = fromRegister<float>(insn.operand1Id);
        float operand2 = fromRegister<float>(insn.operand2Id);
        bool result = operand1 >= operand2;
        toRegister<bool>(insn.resultId) = result;
    }
} 

void Interpreter::stepSLessThanEqual(const InsnSLessThanEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
        const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] <= operand2[i];
        }
    } else {
        int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
        int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
        bool result = operand1 <= operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepSLessThan(const InsnSLessThan& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
        const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] < operand2[i];
        }
    } else {
        int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
        int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
        bool result = operand1 < operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepSDiv(const InsnSDiv& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const int32_t* operand1 = &fromRegister<int32_t>(insn.operand1Id);
        const int32_t* operand2 = &fromRegister<int32_t>(insn.operand2Id);
        int32_t* result = &toRegister<int32_t>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] / operand2[i];
        }
    } else {
        int32_t operand1 = fromRegister<int32_t>(insn.operand1Id);
        int32_t operand2 = fromRegister<int32_t>(insn.operand2Id);
        int32_t result = operand1 / operand2;
        toRegister<int32_t>(insn.resultId) = result;
    }
}

void Interpreter::stepINotEqual(const InsnINotEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
        const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] != operand2[i];
        }
    } else {
        uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
        uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
        bool result = operand1 != operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepIEqual(const InsnIEqual& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const uint32_t* operand1 = &fromRegister<uint32_t>(insn.operand1Id);
        const uint32_t* operand2 = &fromRegister<uint32_t>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] == operand2[i];
        }
    } else {
        uint32_t operand1 = fromRegister<uint32_t>(insn.operand1Id);
        uint32_t operand2 = fromRegister<uint32_t>(insn.operand2Id);
        bool result = operand1 == operand2;
        toRegister<bool>(insn.resultId) = result;
    }
}

void Interpreter::stepLogicalNot(const InsnLogicalNot& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const bool* operand = &fromRegister<bool>(insn.operandId);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = !operand[i];
        }
    } else {
        toRegister<bool>(insn.resultId) = !fromRegister<bool>(insn.operandId);
    }
}

void Interpreter::stepLogicalAnd(const InsnLogicalAnd& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const bool* operand1 = &fromRegister<bool>(insn.operand1Id);
        const bool* operand2 = &fromRegister<bool>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] && operand2[i];
        }
    } else {
        bool operand1 = fromRegister<bool>(insn.operand1Id);
        bool operand2 = fromRegister<bool>(insn.operand2Id);
        toRegister<bool>(insn.resultId) = operand1 && operand2;
    }
}

void Interpreter::stepAll(const InsnAll& insn)
{
    const Type *type = pgm->types.at(registers[insn.vectorId].type).get();
    const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

    const bool* operand = &fromRegister<bool>(insn.vectorId);

    bool result = true;
    for(int i = 0; i < typeVector->count; i++) {
        result = result && operand[i];
        if (!result) {
            break;
        }
    }

    toRegister<bool>(insn.resultId) = result;
}

void Interpreter::stepAny(const InsnAny& insn)
{
    const Type *type = pgm->types.at(registers[insn.vectorId].type).get();
    const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

    const bool* operand = &fromRegister<bool>(insn.vectorId);

    bool result = false;
    for(int i = 0; i < typeVector->count; i++) {
        result = result || operand[i];
        if (result) {
            break;
        }
    }

    toRegister<bool>(insn.resultId) = result;
}

void Interpreter::stepLogicalOr(const InsnLogicalOr& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const bool* operand1 = &fromRegister<bool>(insn.operand1Id);
        const bool* operand2 = &fromRegister<bool>(insn.operand2Id);
        bool* result = &toRegister<bool>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = operand1[i] || operand2[i];
        }
    } else {
        bool operand1 = fromRegister<bool>(insn.operand1Id);
        bool operand2 = fromRegister<bool>(insn.operand2Id);
        toRegister<bool>(insn.resultId) = operand1 || operand2;
    }
}

void Interpreter::stepSelect(const InsnSelect& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const bool* condition = &fromRegister<bool>(insn.conditionId);
        // XXX shouldn't assume floats here. Any data is valid.
        const float* object1 = &fromRegister<float>(insn.object1Id);
        const float* object2 = &fromRegister<float>(insn.object2Id);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = condition[i] ? object1[i] : object2[i];
        }
    } else {
        bool condition = fromRegister<bool>(insn.conditionId);
        float object1 = fromRegister<float>(insn.object1Id);
        float object2 = fromRegister<float>(insn.object2Id);
        float result = condition ? object1 : object2;
        toRegister<float>(insn.resultId) = result;
    }
}

void Interpreter::stepVectorTimesScalar(const InsnVectorTimesScalar& insn)
{
    const float* vector = &fromRegister<float>(insn.vectorId);
    float scalar = fromRegister<float>(insn.scalarId);
    float* result = &toRegister<float>(insn.resultId);

    const TypeVector *typeVector = pgm->type<TypeVector>(insn.type);

    for(int i = 0; i < typeVector->count; i++) {
        result[i] = vector[i] * scalar;
    }
}

void Interpreter::stepMatrixTimesMatrix(const InsnMatrixTimesMatrix& insn)
{
    const float* left = &fromRegister<float>(insn.leftMatrixId);
    const float* right = &fromRegister<float>(insn.rightMatrixId);
    float* result = &toRegister<float>(insn.resultId);

    const Register &leftMatrixReg = registers[insn.leftMatrixId];

    const TypeMatrix *leftMatrixType = pgm->type<TypeMatrix>(leftMatrixReg.type);
    const TypeVector *leftMatrixVectorType = pgm->type<TypeVector>(leftMatrixType->columnType);

    const TypeMatrix *rightMatrixType = pgm->type<TypeMatrix>(leftMatrixReg.type);

    const TypeMatrix *resultType = pgm->type<TypeMatrix>(insn.type);
    const TypeVector *resultVectorType = pgm->type<TypeVector>(resultType->columnType);

    int resultColumnCount = resultType->columnCount;
    int resultRowCount = resultVectorType->count;
    int leftcols = leftMatrixType->columnCount;
    int leftrows = leftMatrixVectorType->count;
    int rightcols = rightMatrixType->columnCount;
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

    const TypeVector *resultType = pgm->type<TypeVector>(insn.type);
    const TypeVector *vectorType = pgm->type<TypeVector>(vectorReg.type);

    int rn = resultType->count;
    int vn = vectorType->count;

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

    const TypeVector *resultType = pgm->type<TypeVector>(insn.type);
    const TypeVector *vectorType = pgm->type<TypeVector>(vectorReg.type);

    int rn = resultType->count;
    int vn = vectorType->count;

    for(int i = 0; i < rn; i++) {
        result[i] = dotProduct(vector, matrix + vn*i, vn);
    }
}

void Interpreter::stepVectorShuffle(const InsnVectorShuffle& insn)
{
    Register& obj = registers.at(insn.resultId);
    const Register &r1 = registers.at(insn.vector1Id);
    const Register &r2 = registers.at(insn.vector2Id);
    const TypeVector *t1 = pgm->type<TypeVector>(r1.type);
    uint32_t n1 = t1->count;
    uint32_t elementSize = pgm->typeSizes.at(t1->type);

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
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const int32_t* src = &fromRegister<int32_t>(insn.signedValueId);
        float* dst = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            dst[i] = src[i];
        }
    } else {
        int32_t src = fromRegister<int32_t>(insn.signedValueId);
        toRegister<float>(insn.resultId) = src;
    }
}

void Interpreter::stepConvertFToS(const InsnConvertFToS& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* src = &fromRegister<float>(insn.floatValueId);
        uint32_t* dst = &toRegister<uint32_t>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            dst[i] = src[i];
        }
    } else {
        float src = fromRegister<float>(insn.floatValueId);
        toRegister<uint32_t>(insn.resultId) = src;
    }
}

void Interpreter::stepAccessChain(const InsnAccessChain& insn)
{
    Pointer& basePointer = pointers.at(insn.baseId);
    uint32_t type = basePointer.type;
    size_t address = basePointer.address;
    for(auto& id: insn.indexesId) {
        int32_t j = fromRegister<int32_t>(id);
        ConstituentInfo info = pgm->getConstituentInfo(type, j);
        type = info.subtype;
        address += info.offset;
    }
    if(false) {
        std::cout << "accesschain of " << basePointer.address << " yielded " << address << "\n";
    }
    uint32_t pointedType = pgm->type<TypePointer>(insn.type)->type;
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
    const Type *type = pgm->types.at(registers[insn.p0Id].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* p0 = &fromRegister<float>(insn.p0Id);
        const float* p1 = &fromRegister<float>(insn.p1Id);
        float radicand = 0;
        for(int i = 0; i < typeVector->count; i++) {
            radicand += (p1[i] - p0[i]) * (p1[i] - p0[i]);
        }
        toRegister<float>(insn.resultId) = sqrtf(radicand);
    } else {
        float p0 = fromRegister<float>(insn.p0Id);
        float p1 = fromRegister<float>(insn.p1Id);
        float radicand = (p1 - p0) * (p1 - p0);
        toRegister<float>(insn.resultId) = sqrtf(radicand);
    }
}

void Interpreter::stepGLSLstd450Length(const InsnGLSLstd450Length& insn)
{
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float length = 0;
        for(int i = 0; i < typeVector->count; i++) {
            length += x[i]*x[i];
        }
        toRegister<float>(insn.resultId) = sqrtf(length);
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = fabsf(x);
    }
}

void Interpreter::stepGLSLstd450FMax(const InsnGLSLstd450FMax& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        const float* y = &fromRegister<float>(insn.yId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = fmaxf(x[i], y[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        float y = fromRegister<float>(insn.yId);
        toRegister<float>(insn.resultId) = fmaxf(x, y);
    }
}

void Interpreter::stepGLSLstd450FMin(const InsnGLSLstd450FMin& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        const float* y = &fromRegister<float>(insn.yId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = fminf(x[i], y[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        float y = fromRegister<float>(insn.yId);
        toRegister<float>(insn.resultId) = fminf(x, y);
    }
}

void Interpreter::stepGLSLstd450Pow(const InsnGLSLstd450Pow& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        const float* y = &fromRegister<float>(insn.yId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = powf(x[i], y[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        float y = fromRegister<float>(insn.yId);
        toRegister<float>(insn.resultId) = powf(x, y);
    }
}

void Interpreter::stepGLSLstd450Normalize(const InsnGLSLstd450Normalize& insn)
{
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float length = 0;
        for(int i = 0; i < typeVector->count; i++) {
            length += x[i]*x[i];
        }
        length = sqrtf(length);

        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = length == 0 ? 0 : x[i]/length;
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = x < 0 ? -1 : 1;
    }
}

void Interpreter::stepGLSLstd450Radians(const InsnGLSLstd450Radians& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* degrees = &fromRegister<float>(insn.degreesId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = degrees[i] / 180.0 * M_PI;
        }
    } else {
        float degrees = fromRegister<float>(insn.degreesId);
        toRegister<float>(insn.resultId) = degrees / 180.0 * M_PI;
    }
}

void Interpreter::stepGLSLstd450Sin(const InsnGLSLstd450Sin& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = sin(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = sin(x);
    }
}

void Interpreter::stepGLSLstd450Cos(const InsnGLSLstd450Cos& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = cos(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = cos(x);
    }
}

void Interpreter::stepGLSLstd450Atan(const InsnGLSLstd450Atan& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* y_over_x = &fromRegister<float>(insn.y_over_xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = atanf(y_over_x[i]);
        }
    } else {
        float y_over_x = fromRegister<float>(insn.y_over_xId);
        toRegister<float>(insn.resultId) = atanf(y_over_x);
    }
}

void Interpreter::stepGLSLstd450Atan2(const InsnGLSLstd450Atan2& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* y = &fromRegister<float>(insn.yId);
        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = atan2f(y[i], x[i]);
        }
    } else {
        float y = fromRegister<float>(insn.yId);
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = atan2f(y, x);
    }
}

void Interpreter::stepGLSLstd450FSign(const InsnGLSLstd450FSign& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = x[i] < 0.0f ? -1.0f : ((x[i] == 0.0f) ? 0.0f : 1.0f);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = (x < 0.0f) ? -1.0f : ((x == 0.0f) ? 0.0f : 1.0f);
    }
}

void Interpreter::stepGLSLstd450Sqrt(const InsnGLSLstd450Sqrt& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = sqrtf(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = sqrtf(x);
    }
}

void Interpreter::stepGLSLstd450FAbs(const InsnGLSLstd450FAbs& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = fabsf(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = fabsf(x);
    }
}

void Interpreter::stepGLSLstd450Exp(const InsnGLSLstd450Exp& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = expf(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = expf(x);
    }
}

void Interpreter::stepGLSLstd450Exp2(const InsnGLSLstd450Exp2& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = exp2f(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = exp2f(x);
    }
}

void Interpreter::stepGLSLstd450Floor(const InsnGLSLstd450Floor& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = floor(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = floor(x);
    }
}

void Interpreter::stepGLSLstd450Fract(const InsnGLSLstd450Fract& insn)
{
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = x[i] - floor(x[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = x - floor(x);
    }
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
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        const float* minVal = &fromRegister<float>(insn.minValId);
        const float* maxVal = &fromRegister<float>(insn.maxValId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = fclamp(x[i], minVal[i], maxVal[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        float minVal = fromRegister<float>(insn.minValId);
        float maxVal = fromRegister<float>(insn.maxValId);
        toRegister<float>(insn.resultId) = fclamp(x, minVal, maxVal);
    }
}

void Interpreter::stepGLSLstd450FMix(const InsnGLSLstd450FMix& insn)
{
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* x = &fromRegister<float>(insn.xId);
        const float* y = &fromRegister<float>(insn.yId);
        const float* a = &fromRegister<float>(insn.aId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = fmix(x[i], y[i], a[i]);
        }
    } else {
        float x = fromRegister<float>(insn.xId);
        float y = fromRegister<float>(insn.yId);
        float a = fromRegister<float>(insn.aId);
        toRegister<float>(insn.resultId) = fmix(x, y, a);
    }
}

void Interpreter::stepGLSLstd450SmoothStep(const InsnGLSLstd450SmoothStep& insn)
{
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* edge0 = &fromRegister<float>(insn.edge0Id);
        const float* edge1 = &fromRegister<float>(insn.edge1Id);
        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = smoothstep(edge0[i], edge1[i], x[i]);
        }
    } else {
        float edge0 = fromRegister<float>(insn.edge0Id);
        float edge1 = fromRegister<float>(insn.edge1Id);
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = smoothstep(edge0, edge1, x);
    }
}

void Interpreter::stepGLSLstd450Step(const InsnGLSLstd450Step& insn)
{
    const Type *type = pgm->types.at(registers[insn.xId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* edge = &fromRegister<float>(insn.edgeId);
        const float* x = &fromRegister<float>(insn.xId);
        float* result = &toRegister<float>(insn.resultId);
        for(int i = 0; i < typeVector->count; i++) {
            result[i] = x[i] < edge[i] ? 0.0 : 1.0;
        }
    } else {
        float edge = fromRegister<float>(insn.edgeId);
        float x = fromRegister<float>(insn.xId);
        toRegister<float>(insn.resultId) = x < edge ? 0.0 : 1.0;
    }
}

void Interpreter::stepGLSLstd450Cross(const InsnGLSLstd450Cross& insn)
{
    const TypeVector *typeVector = pgm->type<TypeVector>(insn.type);

    const float* x = &fromRegister<float>(insn.xId);
    const float* y = &fromRegister<float>(insn.yId);
    float* result = &toRegister<float>(insn.resultId);

    assert(typeVector->count == 3);

    result[0] = x[1]*y[2] - y[1]*x[2];
    result[1] = x[2]*y[0] - y[2]*x[0];
    result[2] = x[0]*y[1] - y[0]*x[1];
}

void Interpreter::stepGLSLstd450Reflect(const InsnGLSLstd450Reflect& insn)
{
    const Type *type = pgm->types.at(registers[insn.iId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* i = &fromRegister<float>(insn.iId);
        const float* n = &fromRegister<float>(insn.nId);
        float* result = &toRegister<float>(insn.resultId);

        float dot = dotProduct(n, i, typeVector->count);

        for (int k = 0; k < typeVector->count; k++) {
            result[k] = i[k] - 2.0*dot*n[k];
        }
    } else {
        float i = fromRegister<float>(insn.iId);
        float n = fromRegister<float>(insn.nId);

        float dot = n*i;

        toRegister<float>(insn.resultId) = i - 2.0*dot*n;
    }
}

void Interpreter::stepGLSLstd450Refract(const InsnGLSLstd450Refract& insn)
{
    const Type *type = pgm->types.at(registers[insn.iId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        const float* i = &fromRegister<float>(insn.iId);
        const float* n = &fromRegister<float>(insn.nId);
        float eta = fromRegister<float>(insn.etaId);
        float* result = &toRegister<float>(insn.resultId);

        float dot = dotProduct(n, i, typeVector->count);

        float k = 1.0 - eta * eta * (1.0 - dot * dot);

        if(k < 0.0) {
            for (int m = 0; m < typeVector->count; m++) {
                result[m] = 0.0;
            }
        } else {
            for (int m = 0; m < typeVector->count; m++) {
                result[m] = eta * i[m] - (eta * dot + sqrtf(k)) * n[m];
            }
        }
    } else {
        float i = fromRegister<float>(insn.iId);
        float n = fromRegister<float>(insn.nId);
        float eta = fromRegister<float>(insn.etaId);

        float dot = n*i;

        float k = 1.0 - eta * eta * (1.0 - dot * dot);

        if(k < 0.0)
            toRegister<float>(insn.resultId) = 0.0;
        else
            toRegister<float>(insn.resultId) = eta * i - (eta * dot + sqrtf(k)) * n;
    }
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
    const Type *type = pgm->types.at(insn.type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        assert(typeVector->count == 2);

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
            std::cout << "Unhandled filter mode for ImageSampleImplicitLod coordinate\n";
        }
    } else {
        std::cout << "Unhandled type for ImageSampleImplicitLod coordinate\n";
    }

    uint32_t resultTypeId = pgm->type<TypeVector>(registers[insn.resultId].type)->type;

    // Store the sample result in register
    const Type *resultType = pgm->types.at(resultTypeId).get();
    if (resultType->op() == SpvOpTypeFloat) {
        toRegister<v4float>(insn.resultId) = rgba;
    } else {
        std::cout << "Unhandled type for ImageSampleImplicitLod result\n";
    }
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
    const Type *type = pgm->types.at(registers[insn.coordinateId].type).get();

    if (type->op() == SpvOpTypeVector) {
        const TypeVector *typeVector = dynamic_cast<const TypeVector *>(type);

        assert(typeVector->count == 2);

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

    uint32_t resultTypeId = pgm->type<TypeVector>(registers[insn.resultId].type)->type;

    // Store the sample result in register
    const Type *resultType = pgm->types.at(resultTypeId).get();

    if (resultType->op() == SpvOpTypeFloat) {
        toRegister<v4float>(insn.resultId) = rgba;
    } else {
        std::cout << "Unhandled type for ImageSampleExplicitLod result\n";
    }
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
