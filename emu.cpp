#include <cmath>
#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <thread>
#include <mutex>
#include "risc-v.h"
#include "emu.h"
#include "timer.h"
#include "disassemble.h"

void dumpGPUCore(const GPUCore& core)
{
    std::cout << std::setfill('0');
    for(int i = 0; i < 32; i++) {
        std::cout << "x" << std::setw(2) << i << ":" << std::hex << std::setw(8) << core.regs.x[i] << std::dec;
        std::cout << (((i + 1) % 4 == 0) ? "\n" : " ");
    }
    for(int i = 0; i < 32; i++) {
        std::cout << "ft" << std::setfill('0') << std::setw(2) << i << ":" << std::setw(8) << std::setfill(' ') << core.regs.f[i];
        std::cout << (((i + 1) % 4 == 0) ? "\n" : " ");
    }
    std::cout << "pc :" << std::hex << std::setw(8) << core.regs.pc << '\n' << std::dec;
    std::cout << std::setfill(' ');
}

void dumpRegsDiff(const GPUCore::Registers& prev, const GPUCore::Registers& cur)
{
    std::cout << std::setfill('0');
    if(prev.pc != cur.pc) {
        std::cout << "pc changed to " << std::hex << std::setw(8) << cur.pc << std::dec << '\n';
    }
    for(int i = 0; i < 32; i++) {
        if(prev.x[i] != cur.x[i]) {
            std::cout << "x" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << cur.x[i] << std::dec << '\n';
        }
    }
    for(int i = 0; i < 32; i++) {
        bool bothnan = std::isnan(cur.f[i]) && std::isnan(prev.f[i]);
        if((prev.f[i] != cur.f[i]) && !bothnan) { // if both NaN, equality test still fails)
            uint32_t x = floatToInt(cur.f[i]);
            std::cout << "f" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << x << std::dec << "(" << cur.f[i] << ")\n";
        }
    }
    std::cout << std::setfill(' ');
}

struct ReadOnlyMemory
{
    std::vector<uint8_t> memorybytes;
    bool verbose;
    ReadOnlyMemory(const std::vector<uint8_t>& memorybytes_, bool verbose_ = false) :
        memorybytes(memorybytes_),
        verbose(verbose_)
    {}
    uint8_t read8(uint32_t addr)
    {
        if(verbose) { printf("read 8 from %08X\n", addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        return memorybytes[addr];
    }
    uint16_t read16(uint32_t addr)
    {
        if(verbose) { printf("read 16 from %08X\n", addr); std::cout.flush(); }
        assert(addr + 1 < memorybytes.size());
        return *reinterpret_cast<uint16_t*>(memorybytes.data() + addr);
    }
    uint32_t read32(uint32_t addr)
    {
        if(verbose) { printf("read 32 from %08X\n", addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
    float readf(uint32_t addr)
    {
        if(verbose) { printf("read float from %08X\n", addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<float*>(memorybytes.data() + addr);
    }
};

struct ReadWriteMemory : public ReadOnlyMemory
{
    ReadWriteMemory(const std::vector<uint8_t>& memorybytes_, bool verbose_ = false) :
        ReadOnlyMemory(memorybytes_, verbose_)
    {}
    void write8(uint32_t addr, uint32_t v)
    {
        if(verbose) { printf("write 8 bits of %08X to %08X\n", v, addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        memorybytes[addr] = static_cast<uint8_t>(v);
    }
    void write16(uint32_t addr, uint32_t v)
    {
        if(verbose) { printf("write 16 bits of %08X to %08X\n", v, addr); std::cout.flush(); }
        assert(addr + 1 < memorybytes.size());
        *reinterpret_cast<uint16_t*>(memorybytes.data() + addr) = static_cast<uint16_t>(v);
    }
    void write32(uint32_t addr, uint32_t v)
    {
        if(verbose) { printf("write 32 bits of %08X to %08X\n", v, addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        *reinterpret_cast<uint32_t*>(memorybytes.data() + addr) = v;
    }
    void writef(uint32_t addr, float v)
    {
        if(verbose) { printf("write float %f to %08X\n", v, addr); std::cout.flush(); }
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        *reinterpret_cast<float*>(memorybytes.data() + addr) = v;
    }
};

template <class MEMORY, class TYPE>
void set(MEMORY& memory, uint32_t address, const TYPE& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    memory.write32(address, *reinterpret_cast<const uint32_t*>(&value));
}

template <class MEMORY, class TYPE, unsigned long N>
void set(MEMORY& memory, uint32_t address, const std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        memory.write32(address + i * sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(&value[i]));
}

template <class MEMORY, class TYPE, unsigned long N>
void get(MEMORY& memory, uint32_t address, std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        *reinterpret_cast<uint32_t*>(&value[i]) = memory.read32(address + i * sizeof(uint32_t));
}

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

void usage(const char* progname)
{
    printf("usage: %s [options] shader.blob\n", progname);
    printf("options:\n");
    printf("\t-f N         Render frame N\n");
    printf("\t-v           Print memory access\n");
    printf("\t-S           Show the disassembly of the SPIR-V code\n");
    printf("\t--term       Draw output image on terminal (in addition to file)\n");
    printf("\t-d           Print symbols\n");
    printf("\t--header     Print information about the object file and\n");
    printf("\t             exit before emulation\n");
    printf("\t-j N         Use N threads [%d]\n",
            int(std::thread::hardware_concurrency()));
    printf("\t--pixel X Y  Render only pixel X and Y\n");
    printf("\t--subst      Print which library functions were substituted\n");
}

struct CoreShared
{
    bool coreHadAnException = false; // Only set to true after initialization
    unsigned char *img;
    std::mutex rendererMutex;

    // Number of rows still left to shade (for progress report).
    std::atomic_int rowsLeft;

    // These require exclusion using rendererMutex
    std::set<std::string> substitutedFunctions;
    uint64_t dispatchedCount = 0;
};

struct CoreParameters
{
    std::vector<uint8_t> text_bytes;
    std::vector<uint8_t> data_bytes;
    SymbolTable text_symbols;
    SymbolTable data_symbols;

    AddressToSymbolMap textAddressesToSymbols;

    uint32_t gl_FragCoordAddress;
    uint32_t colorAddress;
    uint32_t iTimeAddress;

    uint32_t initialPC;

    int imageWidth;
    int imageHeight;
    float frameTime;
    int startX;
    int startY;
    int afterLastX;
    int afterLastY;
};

struct GPUEmuDebugOptions
{
    bool printDisassembly = false;
    bool printMemoryAccess = false;
    bool printCoreDiff = false;
};

// Thread to show progress to the user.
void showProgress(const CoreParameters* tmpl, CoreShared* shared, std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    int totalRows = tmpl->afterLastY - tmpl->startY;

    while(true) {
        int left = shared->rowsLeft;
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
        for (int i = 0; i < 100 && shared->rowsLeft > 0; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

void render(const GPUEmuDebugOptions* debugOptions, const CoreParameters* tmpl, CoreShared* shared, int start_row, int skip_rows)
{
    uint64_t coreDispatchedCount = 0;

    ReadWriteMemory data_memory(tmpl->data_bytes, false);
    ReadOnlyMemory text_memory(tmpl->text_bytes, false);

    GPUCore core(tmpl->text_symbols);
    GPUCore::Status status;

    const float fw = tmpl->imageWidth;
    const float fh = tmpl->imageHeight;
    const float one = 1.0;

    if (tmpl->data_symbols.find(".anonymous") != tmpl->data_symbols.end()) {
        set(data_memory, tmpl->data_symbols.find(".anonymous")->second, v3float{fw, fh, one});
        set(data_memory, tmpl->data_symbols.find(".anonymous")->second + sizeof(v3float), tmpl->frameTime);
    }

    for(int j = start_row; j < tmpl->afterLastY; j += skip_rows) {
        GPUCore::Registers oldRegs;
        for(int i = tmpl->startX; i < tmpl->afterLastX; i++) {

            if(shared->coreHadAnException)
                return;

            set(data_memory, tmpl->gl_FragCoordAddress, v4float{i + 0.5f, j + 0.5f, 0, 0});
            set(data_memory, tmpl->colorAddress, v4float{1, 1, 1, 1});
            if(false) // XXX TODO: when iTime is exported by compilation
                set(data_memory, tmpl->iTimeAddress, tmpl->frameTime);

            core.regs.x[1] = 0xfffffffe; // Set RA to unlikely value to catch ret with no caller
            core.regs.pc = tmpl->initialPC;

            if(debugOptions->printDisassembly) {
                std::cout << "; pixel " << i << ", " << j << '\n';
            }

            try {
                do {
                    if(debugOptions->printCoreDiff) {
                        oldRegs = core.regs;
                    }
                    if(debugOptions->printDisassembly) {
                        print_inst(core.regs.pc, text_memory.read32(core.regs.pc), tmpl->textAddressesToSymbols);
                    }
                    data_memory.verbose = debugOptions->printMemoryAccess;
                    text_memory.verbose = debugOptions->printMemoryAccess;
                    status = core.step(text_memory, data_memory);
                    coreDispatchedCount ++;
                    data_memory.verbose = false;
                    text_memory.verbose = false;
                    if(debugOptions->printCoreDiff) {
                        dumpRegsDiff(oldRegs, core.regs);
                    }
                } while(core.regs.pc != 0xfffffffe && status == GPUCore::RUNNING);
            } catch(const std::exception& e) {
                std::cerr << "core " << start_row << ", " << e.what() << '\n';
                dumpGPUCore(core);
                shared->coreHadAnException = true;
                return;
            }

            if(status != GPUCore::BREAK) {
                std::cerr << "core " << start_row << ", " << "unexpected core step result " << status << '\n';
                dumpGPUCore(core);
                shared->coreHadAnException = true;
                return;
            }

            v3float rgb;
            get(data_memory, tmpl->colorAddress, rgb);

            int pixelOffset = 3 * ((tmpl->imageHeight - 1 - j) * tmpl->imageWidth + i);
            for(int c = 0; c < 3; c++)
                shared->img[pixelOffset + c] = std::clamp(int(rgb[c] * 255.99), 0, 255);
        }
        shared->rowsLeft --;
    }
    {
        std::scoped_lock l(shared->rendererMutex);
        shared->dispatchedCount += coreDispatchedCount;
        shared->substitutedFunctions.insert(core.substitutedFunctions.begin(), core.substitutedFunctions.end());
    }
}

static float MIN_FLOAT_NORMAL = 0x1.0p-126f;
static float MAX_FLOAT_VALUE = 0x1.fffffep127;

/**
 * Compare two floats to see if they are nearly equal.
 */
static bool nearlyEqual(float a, float b, float epsilon) {
    float absA = fabsf(a);
    float absB = fabsf(b);
    float diff = fabsf(a - b);

    if (isnan(a) || isnan(b)) {
        return isnan(a) && isnan(b);
    } else if (a == b) {
        // Shortcut, handles infinities.
        return true;
    } else if (a == 0 || b == 0 || (absA + absB < MIN_FLOAT_NORMAL)) {
        // a or b is zero or both are extremely close to it.
        // Relative error is less meaningful here.
        return diff < epsilon*MIN_FLOAT_NORMAL;
    } else {
        // Use relative error.
        return diff / fmin((absA + absB), MAX_FLOAT_VALUE) < epsilon;
    }
}

/**
 * Run the specified math function. The parameters are in normal parameter order (i.e.,
 * in the order they'd be passed in C code).
 */
static float runMathFunction(GPUEmuDebugOptions *debugOptions, CoreParameters *tmpl,
        const std::string &funcName, const std::vector<float> &params) {

    ReadWriteMemory data_memory(tmpl->data_bytes, false);
    ReadOnlyMemory text_memory(tmpl->text_bytes, false);

    GPUCore core(tmpl->text_symbols);
    GPUCore::Status status;

    // Find address of function.
    auto itr = tmpl->text_symbols.find(funcName);
    if (itr == tmpl->text_symbols.end()) {
        std::cerr << "Error: Can't find function \"" << funcName << ".\"\n";
        exit(EXIT_FAILURE);
    }
    core.regs.pc = itr->second;

    // Set up the stack.
    core.regs.x[2] = RiscVInitialStackPointer;

    // Push parameters in reverse order.
    for (auto itr = params.rbegin(); itr != params.rend(); ++itr) {
        float param = *itr;
        core.regs.x[2] -= 4;
        data_memory.writef(core.regs.x[2], param);
    }

    // Set RA to catch final return.
    core.regs.x[1] = 0xfffffffe;

    // Fill registers with known values so we can see which ones aren't being saved.
    for (int i = 3; i < 32; i++) {
        core.regs.x[i] = i*i*i;
    }
    for (int i = 0; i < 32; i++) {
        core.regs.f[i] = i*i*i + 123;
    }

    GPUCore::Registers oldRegs;
    try {
        do {
            if(debugOptions->printCoreDiff) {
                oldRegs = core.regs;
            }
            if(debugOptions->printDisassembly) {
                print_inst(core.regs.pc,
                        text_memory.read32(core.regs.pc),
                        tmpl->textAddressesToSymbols);
            }
            data_memory.verbose = debugOptions->printMemoryAccess;
            text_memory.verbose = debugOptions->printMemoryAccess;
            status = core.step(text_memory, data_memory);
            data_memory.verbose = false;
            text_memory.verbose = false;
            if(debugOptions->printCoreDiff) {
                dumpRegsDiff(oldRegs, core.regs);
            }
        } while(core.regs.pc != 0xfffffffe && status == GPUCore::RUNNING);
    } catch(const std::exception& e) {
        std::cerr << "Exception running step: " << e.what() << '\n';
        dumpGPUCore(core);
        exit(EXIT_FAILURE);
    }

    if(status != GPUCore::RUNNING) {
        std::cerr << "Unexpected core step status " << status << '\n';
        dumpGPUCore(core);
        exit(EXIT_FAILURE);
    }

    // Check registers so we can see which ones aren't being saved.
    for (int i = 3; i < 32; i++) {
        if (core.regs.x[i] != i*i*i) {
            std::cerr << "Error: Register x" << i << " isn't being saved in " << funcName << "\n";
        }
    }
    for (int i = 0; i < 32; i++) {
        if (core.regs.f[i] != i*i*i + 123) {
            std::cerr << "Error: Register f" << i << " isn't being saved in " << funcName << "\n";
        }
    }

    // Pop result off the stack.
    return data_memory.readf(core.regs.x[2]);
}

/**
 * Test a library unary function against its C++ math library counterpart.
 */
static void tryUnaryFunction(GPUEmuDebugOptions *debugOptions, CoreParameters *tmpl,
        const std::string &funcName, float (*func)(float), float param, int &errors) {

    // Function parameters.
    std::vector<float> params;
    params.push_back(param);

    float expectedValue = func(param);
    float actualValue = runMathFunction(debugOptions, tmpl, funcName, params);

    if (!nearlyEqual(expectedValue, actualValue, 0.000001)) {
        std::cout << funcName << "(" << param << ") = "
            << actualValue << " instead of " << expectedValue << "\n";
        errors++;
    }
}

/**
 * Test a library binary function against its C++ math library counterpart.
 */
static void tryBinaryFunction(GPUEmuDebugOptions *debugOptions, CoreParameters *tmpl,
        const std::string &funcName, float (*func)(float,float),
        float param1, float param2, int &errors) {

    // Function parameters.
    std::vector<float> params;
    params.push_back(param1);
    params.push_back(param2);

    float expectedValue = func(param1, param2);
    float actualValue = runMathFunction(debugOptions, tmpl, funcName, params);

    if (!nearlyEqual(expectedValue, actualValue, 0.000001)) {
        std::cout << funcName << "(" << param1 << ", " << param2 << ") = "
            << actualValue << " instead of " << expectedValue << "\n";
        errors++;
    }
}

/**
 * Test a library ternary function against its C++ math library counterpart.
 */
static void tryTernaryFunction(GPUEmuDebugOptions *debugOptions, CoreParameters *tmpl,
        const std::string &funcName, float (*func)(float,float,float),
        float param1, float param2, float param3, int &errors) {

    // Function parameters.
    std::vector<float> params;
    params.push_back(param1);
    params.push_back(param2);
    params.push_back(param3);

    float expectedValue = func(param1, param2, param3);
    float actualValue = runMathFunction(debugOptions, tmpl, funcName, params);

    if (!nearlyEqual(expectedValue, actualValue, 0.000001)) {
        std::cout << funcName << "(" << param1 << ", " << param2 << ", " << param3 << ") = "
            << actualValue << " instead of " << expectedValue << "\n";
        errors++;
    }
}

/**
 * Run a set of regression tests on our standard library. To run this,
 * first assemble the library:
 *
 *     % ./as library.s
 *     % ./emu --test library.o
 *
 */
static void runLibraryTest(GPUEmuDebugOptions *debugOptions, CoreParameters *tmpl) {
    int errors = 0;

    // .exp
    tryUnaryFunction(debugOptions, tmpl, ".exp", expf, -1.23, errors);
    tryUnaryFunction(debugOptions, tmpl, ".exp", expf, 0.0, errors);
    tryUnaryFunction(debugOptions, tmpl, ".exp", expf, 1.23, errors);
    for (float x = -150.329; x < 150; x++) {
        tryUnaryFunction(debugOptions, tmpl, ".exp", expf, x, errors);
    }

    // .exp2
    tryUnaryFunction(debugOptions, tmpl, ".exp2", exp2f, -1.23, errors);
    tryUnaryFunction(debugOptions, tmpl, ".exp2", exp2f, 0.0, errors);
    tryUnaryFunction(debugOptions, tmpl, ".exp2", exp2f, 1.23, errors);
    for (float x = -150.329; x < 150; x++) {
        tryUnaryFunction(debugOptions, tmpl, ".exp2", exp2f, x, errors);
    }

    // .log
    tryUnaryFunction(debugOptions, tmpl, ".log", logf, -1.23, errors);
    tryUnaryFunction(debugOptions, tmpl, ".log", logf, -0.0, errors);
    tryUnaryFunction(debugOptions, tmpl, ".log", logf, +0.0, errors);
    for (float x = 0.329; x < 150; x++) {
        tryUnaryFunction(debugOptions, tmpl, ".log", logf, x, errors);
    }

    // .log2
    tryUnaryFunction(debugOptions, tmpl, ".log2", log2f, -1.23, errors);
    tryUnaryFunction(debugOptions, tmpl, ".log2", log2f, -0.0, errors);
    tryUnaryFunction(debugOptions, tmpl, ".log2", log2f, +0.0, errors);
    for (float x = 0.329; x < 150; x++) {
        tryUnaryFunction(debugOptions, tmpl, ".log2", log2f, x, errors);
    }

    // .pow
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 0.0, 2.0, errors);
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 2.0, 1.0, errors);
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 2.123, 12.8, errors);
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 2.123, -12.8, errors);
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 21.23, 1.22, errors);
    tryBinaryFunction(debugOptions, tmpl, ".pow", powf, 21.23192, 1.221938, errors);

    // .mix
    tryTernaryFunction(debugOptions, tmpl, ".mix", fmix, -12.18, 182.181, 0.23, errors);
    tryTernaryFunction(debugOptions, tmpl, ".mix", fmix, -12.18, 182.181, 0.0, errors);
    tryTernaryFunction(debugOptions, tmpl, ".mix", fmix, -12.18, 182.181, 1.0, errors);

    std::cout << errors << " test errors.\n";
}

int main(int argc, char **argv)
{
    bool imageToTerminal = false;
    bool printSymbols = false;
    bool printSubstitutions = false;
    bool runTest = false;
    bool printHeaderInfo = false;
    int specificPixelX = -1;
    int specificPixelY = -1;
    int threadCount = std::thread::hardware_concurrency();

    GPUEmuDebugOptions debugOptions;
    CoreParameters tmpl;
    CoreShared shared;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            debugOptions.printMemoryAccess = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--pixel") == 0) {

            if(argc < 2) {
                std::cerr << "Expected pixel X and Y for \"--pixel\"\n";
                usage(progname);
                exit(EXIT_FAILURE);
            }
            specificPixelX = atoi(argv[1]);
            specificPixelY = atoi(argv[2]);
            argv+=3; argc-=3;

        } else if(strcmp(argv[0], "--header") == 0) {

            printHeaderInfo = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--diff") == 0) {

            debugOptions.printCoreDiff = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-j") == 0) {

            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            threadCount = atoi(argv[1]);
            argv += 2; argc -= 2;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-S") == 0) {

            debugOptions.printDisassembly = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-f") == 0) {

            if(argc < 2) {
                std::cerr << "Expected frame parameter for \"-f\"\n";
                usage(progname);
                exit(EXIT_FAILURE);
            }
            tmpl.frameTime = atoi(argv[1]) / 60.0f;
            argv+=2; argc-=2;

        } else if(strcmp(argv[0], "-d") == 0) {

            printSymbols = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--subst") == 0) {

            printSubstitutions = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--test") == 0) {

            runTest = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }

    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    RunHeader2 header;

    std::ifstream binaryFile(argv[0], std::ios::in | std::ios::binary);
    if(!binaryFile.good()) {
        throw std::runtime_error(std::string("couldn't open file ") + argv[0] + " for reading");
    }

    if(!ReadBinary(binaryFile, header, tmpl.text_symbols, tmpl.data_symbols, tmpl.text_bytes, tmpl.data_bytes)) {
        exit(EXIT_FAILURE);
    }
    if(printSymbols) {
        for(auto& [symbol, address]: tmpl.text_symbols) {
            std::cout << "text segment symbol \"" << symbol << "\" is at " << address << "\n";
        }
        for(auto& [symbol, address]: tmpl.data_symbols) {
            std::cout << "data segment symbol \"" << symbol << "\" is at " << address << "\n";
        }
    }
    if(printHeaderInfo) {
        std::cout << tmpl.text_bytes.size() << " bytes of inst memory\n";
        std::cout << tmpl.data_bytes.size() << " bytes of data memory\n";
        std::cout << std::setfill('0');
        std::cout << "initial PC is " << header.initialPC << " (0x" << std::hex << std::setw(8) << header.initialPC << std::dec << ")\n";
        exit(EXIT_SUCCESS);
    }

    tmpl.initialPC = header.initialPC;

    binaryFile.close();

    for(auto& [symbol, address]: tmpl.text_symbols)
        tmpl.textAddressesToSymbols[address] = symbol;

    assert(tmpl.data_bytes.size() < RiscVInitialStackPointer);
    if(tmpl.data_bytes.size() > RiscVInitialStackPointer - 1024) {
        std::cerr << "Warning: stack will be less than 1KiB with this binary's data segment.\n";
    }
    tmpl.data_bytes.resize(RiscVInitialStackPointer);

    if (runTest) {
        runLibraryTest(&debugOptions, &tmpl);
        exit(EXIT_SUCCESS);
    }

    tmpl.imageWidth = 320;
    tmpl.imageHeight = 180;

    for(auto& s: { "gl_FragCoord", "color"}) {
        if (tmpl.data_symbols.find(s) == tmpl.data_symbols.end()) {
            std::cerr << "No memory location for required variable " << s << ".\n";
            exit(EXIT_FAILURE);
        }
    }
    for(auto& s: { "iResolution", "iTime", "iMouse"}) {
        if (tmpl.data_symbols.find(s) == tmpl.data_symbols.end()) {
            // Don't warn, these are in the anonymous params.
            // std::cerr << "Warning: No memory location for variable " << s << ".\n";
        }
    }

    shared.img = new unsigned char[tmpl.imageWidth * tmpl.imageHeight * 3];

    tmpl.gl_FragCoordAddress = tmpl.data_symbols["gl_FragCoord"];
    tmpl.colorAddress = tmpl.data_symbols["color"];
    tmpl.iTimeAddress = tmpl.data_symbols["iTime"];

    tmpl.startX = 0;
    tmpl.afterLastX = tmpl.imageWidth;
    tmpl.startY = 0;
    tmpl.afterLastY = tmpl.imageHeight;

    if(specificPixelX != -1) {
        tmpl.startX = specificPixelX;
        tmpl.afterLastX = specificPixelX + 1;
        tmpl.startY = specificPixelY;
        tmpl.afterLastY = specificPixelY + 1;
    }

    std::cout << "Using " << threadCount << " threads.\n";

    std::vector<std::thread *> thread;

    shared.rowsLeft = tmpl.afterLastY - tmpl.startY;

    for (int t = 0; t < threadCount; t++) {
        thread.push_back(new std::thread(render, &debugOptions, &tmpl, &shared, tmpl.startY + t, threadCount));
    }

    // Progress information.
    Timer frameElapsed;
    thread.push_back(new std::thread(showProgress, &tmpl, &shared, frameElapsed.startTime()));

    // Wait for worker threads to quit.
    while(!thread.empty()) {
        std::thread* td = thread.back();
        thread.pop_back();
        td->join();
    }

    if(shared.coreHadAnException) {
        exit(EXIT_FAILURE);
    }

    if(printSubstitutions) {
        for(auto& subst: shared.substitutedFunctions) {
            std::cout << "substituted for " << subst << '\n';
        }
    }

    std::cout << "shading took " << frameElapsed.elapsed() << " seconds.\n";
    std::cout << shared.dispatchedCount << " instructions executed.\n";
    float fps = 50000000.0f / shared.dispatchedCount;
    std::cout << fps << " fps estimated at 50 MHz.\n";
    std::cout << "at least " << (int)ceilf(5.0 / fps) << " cores required at 50 MHz for 5 fps.\n";

    FILE *fp = fopen("emulated.ppm", "wb");
    fprintf(fp, "P6 %d %d 255\n", tmpl.imageWidth, tmpl.imageHeight);
    fwrite(shared.img, 1, tmpl.imageWidth * tmpl.imageHeight * 3, fp);
    fclose(fp);

    if (imageToTerminal) {
        // https://www.iterm2.com/documentation-images.html
        std::ostringstream ss;
        ss << "P6 " << tmpl.imageWidth << " " << tmpl.imageHeight << " 255\n";
        ss.write(reinterpret_cast<char *>(shared.img), 3*tmpl.imageWidth*tmpl.imageHeight);
        std::cout << "\033]1337;File=width="
            << tmpl.imageWidth << "px;height="
            << tmpl.imageHeight << "px;inline=1:"
            << base64Encode(ss.str()) << "\007\n";
    }
}
