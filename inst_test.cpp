#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// Union for converting from float to int and back.
union FloatUint32 {
    float f;
    uint32_t i;
};

// Bit-wise conversion from int to float.
float intToFloat(uint32_t i) {
    FloatUint32 u;
    u.i = i;
    return u.f;
}

// Bit-wise conversion from float to int.
uint32_t floatToInt(float f) {
    FloatUint32 u;
    u.f = f;
    return u.i;
}

template<typename T>
std::string to_hex(T i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << std::uppercase;
    stream << uint64_t(i);
    return stream.str();
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

// first = value, second = max error
// E.g. for {1, .1}, .91 and 1.09 are within the range to be this float, .9 and 1.1 are not.
typedef std::pair<float, float> ApproximateFloat;

struct InstTest
{
    std::string name;
    std::string assemblyFileName;
    std::map<uint32_t, uint32_t> initialMemory; // IGNORED - set initial memory in the assembly
    std::map<uint32_t, uint32_t> expectedMemoryUInt32s;
    std::map<uint32_t, ApproximateFloat> expectedMemoryFloats;
    std::map<int, uint32_t> expectedXRegisters;          // IGNORED - store values in memory
    std::map<int, ApproximateFloat> expectedFRegisters;  // IGNORED - store values in memory
};

std::vector<InstTest> tests = {
    {
        "fclass.s",
        "fclass.s",
        {},
        {
            {0x28, 0x1}, {0x2C, 0x2}, {0x30, 0x4}, {0x34, 0x8},
            {0x38, 0x10}, {0x3C, 0x20}, {0x40, 0x40}, {0x44, 0x80},
            {0x48, 0x100}, {0x4C, 0x200},
        },
        {},
        {},
        {},
    }
};

std::string readFileContents(std::string fileName)
{
    std::ifstream input(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!input.good()) {
        throw std::runtime_error("couldn't open file " + fileName + " for reading");
    }
    std::ifstream::pos_type size = input.tellg();
    input.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    input.read(&text[0], size);

    return text;
}

std::string getTestEngineCommand(const std::string& engineName, const InstTest& test)
{
    std::string engineCommand = engineName + " -j 1 --pixel 0 0";
    if((test.expectedMemoryUInt32s.size() > 0) || (test.expectedMemoryFloats.size() > 0)) {
        engineCommand += " --dumpmem ";
        bool prefixComma = false;
        for(const auto& mem: test.expectedMemoryUInt32s) {
            engineCommand += (prefixComma ? "," : "") + std::to_string(mem.first);
            prefixComma = true;
        }
        for(const auto& mem: test.expectedMemoryFloats) {
            engineCommand += (prefixComma ? "," : "") + std::to_string(mem.first);
            prefixComma = true;
        }
    }
    engineCommand += " /tmp/x.o 2>&1";
    return engineCommand;
}

bool runTest(const InstTest& test, const std::string& engineCommand, std::map<uint32_t, uint32_t>& memoryResults)
{
    // adapted from https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/
    FILE * stream;
    const int maxLineSize = 1024;
    static char inputLine[maxLineSize];
    bool passed = true;

    stream = popen(engineCommand.c_str(), "r");
    if (stream) {
        while (!feof(stream)) {
            if (fgets(inputLine, maxLineSize, stream) != NULL) {
                if(strncmp(inputLine, "memory,", 7) == 0) {
                    uint32_t addr;
                    uint32_t value;
                    if(sscanf(inputLine, "memory,%u,0x%x", &addr, &value) == 2) {
                        memoryResults[addr] = value;
                    }
                }
            }
        }
        pclose(stream);
    }

    for(const auto& mem: test.expectedMemoryUInt32s) {
        uint32_t addr = mem.first;
        uint32_t value = mem.second;
        if(memoryResults.find(addr) == memoryResults.end()) {
            std::cerr << "internal error: didn't find value for address 0x" << to_hex(addr) << " in emu results!\n";
            return false;
        } else if(memoryResults[addr] != value) {
            std::cerr << "error: value 0x" << to_hex(memoryResults[addr]) << " at address 0x" << to_hex(addr) << " doesn't match expected value 0x" << to_hex(value) << "\n";
            passed = false;
        }
    }

    for(const auto& mem: test.expectedMemoryFloats) {
        uint32_t addr = mem.first;
        ApproximateFloat value = mem.second;
        if(memoryResults.find(addr) == memoryResults.end()) {
            std::cerr << "internal error: didn't find value for address 0x" << to_hex(addr) << " in emu results!\n";
            return false;
        } else {
            float value2 = intToFloat(memoryResults[addr]);
            if(!nearlyEqual(value.first, value2, value.second)) {
                std::cerr << "error: value " << value2 << " at address 0x" << to_hex(addr) << " isn't within " << value.second << " of expected value " << value.first << "\n";
                passed = false;
            }
        }
    }

    return passed;
}

int main(int argc, char **argv)
{
    int failedTestsCount = 0;

    if(argc < 2) {
        std::cout << "usage: " << argv[0] << " engine # e.g. emu or gpu/sim/obj_dir/VMain\n";
        exit(EXIT_FAILURE);
    }

    std::string engineName = argv[1];

    for(const auto& test: tests) {
        std::string actualSourceFileName = "gpuemutest/" + test.assemblyFileName;

        std::string assemblerCommand = std::string("./as -o /tmp/x.o ") + actualSourceFileName;
        int result = system(assemblerCommand.c_str());

        if(result != 0) {
            std::cerr << "failed assembling " << actualSourceFileName << "!  Test " << test.name << " skipped.\n";
            failedTestsCount++;
        } else {

            std::string engineCommand = getTestEngineCommand(engineName, test);

            std::map<uint32_t, uint32_t> memoryResults;

            std::cout << "command " << engineCommand << "\n";
            bool passed = runTest(test, engineCommand, memoryResults);
            if(!passed) {
                failedTestsCount++;
            }
        }
    }

    std::cout << failedTestsCount << " tests failed\n";

    if(failedTestsCount > 0) {
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}
