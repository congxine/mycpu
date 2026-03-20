#include <iostream>
#include <cstdint>
#include <vector>
#include <iomanip>

const uint32_t MEMORY_SIZE = 1024 * 4; 

struct RISCV_CPU {
    uint32_t regs[32];
    uint32_t pc;
    std::vector<uint8_t> memory;

    RISCV_CPU() : pc(0), memory(MEMORY_SIZE, 0) {
        for(int i = 0; i < 32; i++) regs[i] = 0;
    }

    // 阶段 1: 取指 (Fetch) - 保持不变
    uint32_t fetch() {
        uint32_t inst = memory[pc] | (memory[pc+1] << 8) | (memory[pc+2] << 16) | (memory[pc+3] << 24);
        pc += 4; 
        return inst;
    }

    // 阶段 2: 译码 (Decode) - 新增！
    void decode(uint32_t inst) {
        // RISC-V 规定：最低 7 位是操作码 (Opcode)
        uint32_t opcode = inst & 0x7F;          
        // 第 7 到 11 位是目标寄存器 (rd)
        uint32_t rd     = (inst >> 7) & 0x1F;   
        // 第 12 到 14 位是功能码 (funct3)
        uint32_t funct3 = (inst >> 12) & 0x7;   
        // 第 15 到 19 位是源寄存器 1 (rs1)
        uint32_t rs1    = (inst >> 15) & 0x1F;  
        // 第 20 到 31 位是立即数 (imm) - 这里先做简单的无符号截取
        uint32_t imm    = (inst >> 20) & 0xFFF; 

        std::cout << "--- Decode Result ---" << std::endl;
        std::cout << "Opcode: 0x" << std::hex << opcode << std::dec << std::endl;
        std::cout << "rd: x" << rd << ", rs1: x" << rs1 << std::endl;
        std::cout << "funct3: " << funct3 << ", imm: " << imm << std::endl;

        // 识别我们放入的 0x00000013
        if (opcode == 0x13 && funct3 == 0) {
            std::cout << ">>> Instruction matched: ADDI x" << rd << ", x" << rs1 << ", " << imm << std::endl;
            if (rd == 0 && rs1 == 0 && imm == 0) {
                std::cout << ">>> This is effectively a NOP (No Operation) instruction!" << std::endl;
            }
        } else {
            std::cout << ">>> Unknown or unimplemented instruction." << std::endl;
        }
    }
};

int main() {
    RISCV_CPU myCPU;
    std::cout << "--- myCPU Simulator Started ---" << std::endl;

    // 依然是在内存放入 0x00000013
    myCPU.memory[0] = 0x13; 
    myCPU.memory[1] = 0x00;
    myCPU.memory[2] = 0x00;
    myCPU.memory[3] = 0x00;

    std::cout << "Initial PC: 0x" << myCPU.pc << std::endl;

    // 1. 取指
    uint32_t instruction = myCPU.fetch();
    std::cout << "Fetched: 0x" << std::setfill('0') << std::setw(8) << std::hex << instruction << std::dec << std::endl;
    
    // 2. 译码
    myCPU.decode(instruction);

    return 0;
}
