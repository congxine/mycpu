#pragma once
#include <array>
#include <cstdint>
#include "bus.h"

class CPU {
public:
    explicit CPU(Bus& bus);

    void reset(uint32_t pc);
    void step();
    void run(uint64_t max_cycles = 1000000);

    void set_trace(bool enabled);
    void dump_regs() const;

private:
    struct IFID {
        bool valid = false;
        uint32_t pc = 0;
        uint32_t inst = 0;
    };

    struct IDEX {
        bool valid = false;
        uint32_t pc = 0;
        uint32_t inst = 0;

        uint8_t opcode = 0;
        uint8_t rd = 0;
        uint8_t rs1 = 0;
        uint8_t rs2 = 0;
        uint8_t funct3 = 0;
        uint8_t funct7 = 0;

        uint32_t rs1_val = 0;
        uint32_t rs2_val = 0;
        int32_t imm = 0;

        bool reg_write = false;
        bool mem_read = false;
        bool mem_write = false;
        bool wb_from_mem = false;
        uint8_t mem_size = 0;      // 1, 2, 4
        bool mem_unsigned = false; // true for LBU/LHU

        bool is_branch = false;
        bool is_jal = false;
        bool is_jalr = false;
        bool is_system = false;
    };

    struct EXMEM {
        bool valid = false;
        uint32_t pc = 0;
        uint32_t inst = 0;

        uint8_t rd = 0;
        bool reg_write = false;
        bool wb_from_mem = false;

        uint32_t alu_result = 0;
        uint32_t store_data = 0;

        bool mem_read = false;
        bool mem_write = false;
        uint8_t mem_size = 0;
        bool mem_unsigned = false;

        bool halt = false;

        bool control_taken = false;
        uint32_t control_target = 0;
    };

    struct MEMWB {
        bool valid = false;
        uint32_t pc = 0;
        uint32_t inst = 0;

        uint8_t rd = 0;
        bool reg_write = false;
        uint32_t wb_value = 0;
    };

    Bus& bus_;
    uint32_t pc_ = 0;
    std::array<uint32_t, 32> regs_{};

    bool trace_ = false;
    uint64_t cycle_ = 0;

    IFID if_id_{};
    IDEX id_ex_{};
    EXMEM ex_mem_{};
    MEMWB mem_wb_{};

    static int32_t sign_extend(uint32_t value, int bits);
    uint32_t load_reg(uint32_t idx) const;
    void store_reg(uint32_t idx, uint32_t value);
    uint32_t fetch32(uint32_t addr);

    void decode_to_idex(const IFID& in, IDEX& out) const;
    uint32_t forward_operand(uint8_t rs, uint32_t original) const;
};
