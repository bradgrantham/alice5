#ifndef INTERPRETER_SET_H
#define INTERPRETER_SET_H

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

#endif // INTERPRETER_SET_H
