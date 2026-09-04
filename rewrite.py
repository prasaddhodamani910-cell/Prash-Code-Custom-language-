import re

with open('src/codegen.cpp', 'r') as f:
    content = f.read()

# Add allocReg and freeReg
alloc_free = """
std::string CodeGen::allocReg() {
    for (int i = 0; i < 10; ++i) {
        if (m_freeRegisters[i]) {
            m_freeRegisters[i] = false;
            return "x" + std::to_string(19 + i);
        }
    }
    return "x19"; // Spill fallback (simplified)
}

void CodeGen::freeReg(const std::string& reg) {
    if (reg[0] == 'x') {
        int r = std::stoi(reg.substr(1));
        if (r >= 19 && r <= 28) m_freeRegisters[r - 19] = true;
    }
}
"""
content = content.replace('void CodeGen::enterScope() {', alloc_free + '\nvoid CodeGen::enterScope() {')

# Modify prologue to push x19-x28
prologue_orig = """    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");"""
prologue_new = """    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    for (int i = 19; i <= 27; i += 2) {
        emit("stp x" + std::to_string(i) + ", x" + std::to_string(i+1) + ", [sp, #-16]!");
    }"""
content = content.replace(prologue_orig, prologue_new)

# Modify epilogue to pop x19-x28
epilogue_orig = """    emit("mov sp, x29");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");"""
epilogue_new = """    for (int i = 27; i >= 19; i -= 2) {
        emit("ldp x" + std::to_string(i) + ", x" + std::to_string(i+1) + ", [sp], #16");
    }
    emit("mov sp, x29");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");"""
content = content.replace(epilogue_orig, epilogue_new)

with open('src/codegen.cpp', 'w') as f:
    f.write(content)
