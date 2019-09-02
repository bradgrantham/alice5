#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include "emu.h"

int main(int argc, char **argv)
{
    if(argc != 7) {
        std::cerr << "usage: " << argv[0] << " pc fragCoord color time mouse resolution\n";
        exit(EXIT_FAILURE);
    }
    SymbolTable symbols;

    symbols["gl_FragCoord"] = strtoul(argv[2], NULL, 0);
    symbols["color"] = strtoul(argv[3], NULL, 0);
    symbols["iTime"] = strtoul(argv[4], NULL, 0);
    symbols["iMouse"] = strtoul(argv[5], NULL, 0);
    symbols["iResolution"] = strtoul(argv[6], NULL, 0);

    RunHeader1 hdr;

    hdr.initialPC = strtoul(argv[1], NULL, 0);
    hdr.symbolCount = symbols.size();

    fwrite(&hdr, 1, sizeof(hdr), stdout);
    for(auto& [symbol, address] : symbols) {
        fwrite(&address, 1, sizeof(address), stdout);
        uint32_t strsize = symbol.size() + 1;
        fwrite(&strsize, 1, sizeof(strsize), stdout);
        fwrite(symbol.data(), 1, strsize, stdout);
    }
}
