#include <string>
#include <map>
#include <cstdint>
#include <cstdio>

extern "C" {
#include "riscv-disas.h"
}

typedef std::map<uint32_t, std::string> AddressToSymbolMap;

inline void print_inst(uint64_t pc, uint32_t inst, const AddressToSymbolMap& addressesToSymbols)
{
    char buf[80] = { 0 };
    if(addressesToSymbols.find(pc) != addressesToSymbols.end())
        printf("%s:\n", addressesToSymbols.at(pc).c_str());
    disasm_inst(buf, sizeof(buf), rv64, pc, inst);
    printf("        %08" PRIx64 ":  %s\n", pc, buf);
}
    
