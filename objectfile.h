#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <map>
#include <string>

typedef std::map<std::string, uint32_t> SymbolTable;

const uint32_t RunHeader1MagicExpected = 0x31354c41;
struct RunHeader1
{
    // Little-endian
    uint32_t magic = RunHeader1MagicExpected;        // 'AL51', version 1 of Alice 5 header
    uint32_t initialPC;                 // Initial value PC is set to
    uint32_t symbolCount;
    // symbolCount symbols follow that are of the following layout:
    //       uint32_t address
    //       uint32_t stringLength
    //       stringLength bytes for symbol name including NUL
    // Program and data bytes follow.  Bytes are loaded at 0
};

const uint32_t RunHeader2MagicExpected = 0x31354c42;
struct RunHeader2
{
    // All words are little-endian.
    uint32_t magic = RunHeader2MagicExpected;       // 'AL52', version 2 of Alice 5 header
    uint32_t initialPC;                             // Initial value PC is set to, in bytes.
    uint32_t symbolCount;                           // Number of symbols (see below).
    uint32_t textByteCount;                         // Number of bytes of text (code).
    uint32_t dataByteCount;                         // Number of bytes of data.
    // symbolCount symbols follow that are of the following layout:
    //       uint32_t address
    //       uint32_t inDataSegment: if true, in data segment; else in text (code) segment
    //       uint32_t stringLength: including nul.
    //       stringLength bytes for symbol name including nul
    // Program bytes follow. Bytes are loaded at 0 in text memory.
    // Data bytes follow. Bytes are loaded at 0 in data memory.
};

bool ReadBinary(std::ifstream& binaryFile, RunHeader2& header, SymbolTable& text_symbols, SymbolTable& data_symbols, std::vector<uint8_t>& text_bytes, std::vector<uint8_t>& data_bytes)
{
    // TODO: dangerous because of struct packing?
    binaryFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    if(!binaryFile) {
        std::cerr << "ReadBinary : failed to read header, only " << binaryFile.gcount() << " bytes read\n";
        return false;
    }

    if(header.magic != RunHeader2MagicExpected) {
        std::cerr << "ReadBinary : magic read did not match magic expected for RunHeader1\n";
        return false;
    }

    for(uint32_t i = 0; i < header.symbolCount; i++) {
        uint32_t symbolData[3];

        binaryFile.read(reinterpret_cast<char*>(symbolData), sizeof(symbolData));
        if(!binaryFile) {
            std::cerr << "ReadBinary : failed to read address and length for symbol " << i << ", only " << binaryFile.gcount() << " bytes read\n";
            return false;
        }

        uint32_t symbolAddress = symbolData[0];
        bool symbolInDataSegment = symbolData[1] != 0;
        uint32_t symbolStringLength = symbolData[2];

        std::string symbol;
        symbol.resize(symbolStringLength - 1);

        binaryFile.read(symbol.data(), symbolStringLength);
        if(!binaryFile) {
            std::cerr << "ReadBinary : failed to symbol string for symbol " << i << ", only " << binaryFile.gcount() << " bytes read\n";
            return false;
        }

        if(symbolInDataSegment) {
            data_symbols[symbol] = symbolAddress;
        } else {
            text_symbols[symbol] = symbolAddress;
        }
    }

    text_bytes.resize(header.textByteCount);

    binaryFile.read(reinterpret_cast<char*>(text_bytes.data()), header.textByteCount);
    if(!binaryFile) {
        std::cerr << "ReadBinary : failed to text bytes from binary, only " << binaryFile.gcount() << " bytes read\n";
        return false;
    }

    data_bytes.resize(header.dataByteCount);

    binaryFile.read(reinterpret_cast<char*>(data_bytes.data()), header.dataByteCount);
    if(!binaryFile) {
        std::cerr << "ReadBinary : failed to data bytes from binary, only " << binaryFile.gcount() << " bytes read\n";
        return false;
    }

    return true;
}
