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
#include "gpuemu.h"
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
    printf("\t-f N      Render frame N\n");
    printf("\t-v        Print memory access\n");
    printf("\t-S        show the disassembly of the SPIR-V code\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
    printf("\t-d        print symbols\n");
    printf("\t--header  print information about the object file and\n");
    printf("\t          exit before emulation\n");
    printf("\t-j N      Use N threads [%d]\n",
            int(std::thread::hardware_concurrency()));
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

struct CoreTemplate
{
    std::vector<uint8_t> text_bytes;
    std::vector<uint8_t> data_bytes;
    SymbolTable text_symbols;
    SymbolTable data_symbols;
    uint32_t initialPC;
    int imageWidth;
    int imageHeight;
    float frameTime;
    int startX;
    int startY;
    int afterLastX;
    int afterLastY;
    AddressToSymbolMap textAddressesToSymbols;
    uint32_t gl_FragCoordAddress;
    uint32_t colorAddress;
    uint32_t iTimeAddress;
    bool printDisassembly = false;
    bool printMemoryAccess = false;
    bool printCoreDiff = false;
};

// Thread to show progress to the user.
void showProgress(const CoreTemplate* tmpl, CoreShared* shared, std::chrono::time_point<std::chrono::steady_clock> startTime)
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

void render(const CoreTemplate* tmpl, CoreShared* shared, int start_row, int skip_rows)
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

            core.regs.x[1] = 0xffffffff; // Set PC to unlikely value to catch ret with no caller
            core.regs.x[2] = tmpl->data_bytes.size(); // Set SP to end of memory 
            core.regs.pc = tmpl->initialPC;

            if(tmpl->printDisassembly) {
                std::cout << "; pixel " << i << ", " << j << '\n';
            }

            try {
                do {
                    if(tmpl->printCoreDiff) {
                        oldRegs = core.regs;
                    }
                    if(tmpl->printDisassembly) {
                        print_inst(core.regs.pc, text_memory.read32(core.regs.pc), tmpl->textAddressesToSymbols);
                    }
                    data_memory.verbose = tmpl->printMemoryAccess;
                    text_memory.verbose = tmpl->printMemoryAccess;
                    status = core.step(text_memory, data_memory);
                    coreDispatchedCount ++;
                    data_memory.verbose = false;
                    text_memory.verbose = false;
                    if(tmpl->printCoreDiff) {
                        dumpRegsDiff(oldRegs, core.regs);
                    }
                } while(core.regs.pc != 0xffffffff && status == GPUCore::RUNNING);
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

int main(int argc, char **argv)
{
    bool imageToTerminal = false;
    bool printSymbols = false;
    bool printSubstitutions = false;
    bool printHeaderInfo = false;
    int specificPixelX = -1;
    int specificPixelY = -1;
    int threadCount = std::thread::hardware_concurrency();

    CoreTemplate tmpl;
    CoreShared shared;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            tmpl.printMemoryAccess = true;
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

            tmpl.printCoreDiff = true;
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

            tmpl.printDisassembly = true;
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

    tmpl.data_bytes.resize(tmpl.data_bytes.size() + 0x10000);

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

    for (int t = 0; t < threadCount; t++) {
        thread.push_back(new std::thread(render, &tmpl, &shared, tmpl.startY + t, threadCount));
    }

    // Progress information.
    Timer frameElapsed;
    shared.rowsLeft = tmpl.afterLastY - tmpl.startY;
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
