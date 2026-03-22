#include "cpu.h"
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {

static int32_t sign_extend_local(uint32_t value, int bits) {
    uint32_t mask = 1u << (bits - 1);
    return static_cast<int32_t>((value ^ mask) - mask);
}

static uint8_t opcode_of(uint32_t inst) { return static_cast<uint8_t>(inst & 0x7f); }
static uint8_t rd_of(uint32_t inst)     { return static_cast<uint8_t>((inst >> 7) & 0x1f); }
static uint8_t funct3_of(uint32_t inst) { return static_cast<uint8_t>((inst >> 12) & 0x7); }
static uint8_t rs1_of(uint32_t inst)    { return static_cast<uint8_t>((inst >> 15) & 0x1f); }
static uint8_t rs2_of(uint32_t inst)    { return static_cast<uint8_t>((inst >> 20) & 0x1f); }
static uint8_t funct7_of(uint32_t inst) { return static_cast<uint8_t>((inst >> 25) & 0x7f); }

static int32_t imm_i(uint32_t inst) {
    return sign_extend_local(inst >> 20, 12);
}

static int32_t imm_s(uint32_t inst) {
    uint32_t imm11_5 = (inst >> 25) & 0x7f;
    uint32_t imm4_0 = (inst >> 7) & 0x1f;
    return sign_extend_local((imm11_5 << 5) | imm4_0, 12);
}

static int32_t imm_b(uint32_t inst) {
    uint32_t imm12   = ((inst >> 31) & 0x1) << 12;
    uint32_t imm10_5 = ((inst >> 25) & 0x3f) << 5;
    uint32_t imm4_1  = ((inst >> 8) & 0xf) << 1;
    uint32_t imm11   = ((inst >> 7) & 0x1) << 11;
    return sign_extend_local(imm12 | imm11 | imm10_5 | imm4_1, 13);
}

static int32_t imm_u(uint32_t inst) {
    return static_cast<int32_t>(inst & 0xfffff000u);
}

static int32_t imm_j(uint32_t inst) {
    uint32_t imm20    = ((inst >> 31) & 0x1) << 20;
    uint32_t imm10_1  = ((inst >> 21) & 0x3ff) << 1;
    uint32_t imm11    = ((inst >> 20) & 0x1) << 11;
    uint32_t imm19_12 = ((inst >> 12) & 0xff) << 12;
    return sign_extend_local(imm20 | imm19_12 | imm11 | imm10_1, 21);
}

static bool uses_rs1(uint32_t inst) {
    switch (opcode_of(inst)) {
        case 0x67: // JALR
        case 0x63: // BRANCH
        case 0x03: // LOAD
        case 0x23: // STORE
        case 0x13: // OP-IMM
        case 0x33: // OP
            return true;
        default:
            return false;
    }
}

static bool uses_rs2(uint32_t inst) {
    switch (opcode_of(inst)) {
        case 0x63: // BRANCH
        case 0x23: // STORE
        case 0x33: // OP
            return true;
        default:
            return false;
    }
}

} // namespace

CPU::CPU(Bus& bus) : bus_(bus) {}

void CPU::reset(uint32_t pc) {
    pc_ = pc;
    regs_.fill(0);
    trace_ = false;
    cycle_ = 0;
    if_id_ = IFID{};
    id_ex_ = IDEX{};
    ex_mem_ = EXMEM{};
    mem_wb_ = MEMWB{};
}

void CPU::set_trace(bool enabled) {
    trace_ = enabled;
}

void CPU::dump_regs() const {
    std::cout << "\n=== Register Dump ===\n";
    for (int i = 0; i < 32; ++i) {
        std::cout << "x" << std::dec << std::setw(2) << std::setfill(' ') << i
                  << " = 0x" << std::hex << std::setw(8) << std::setfill('0')
                  << (i == 0 ? 0u : regs_[i]);

        if ((i % 4) == 3) {
            std::cout << "\n";
        } else {
            std::cout << "    ";
        }
    }
    std::cout << "pc = 0x" << std::hex << std::setw(8) << std::setfill('0') << pc_ << "\n";
    std::cout << std::dec;
}

int32_t CPU::sign_extend(uint32_t value, int bits) {
    return sign_extend_local(value, bits);
}

uint32_t CPU::load_reg(uint32_t idx) const {
    return idx == 0 ? 0u : regs_[idx];
}

void CPU::store_reg(uint32_t idx, uint32_t value) {
    if (idx != 0) {
        regs_[idx] = value;
        if (trace_) {
            std::cout << "    WB: x" << std::dec << static_cast<unsigned>(idx)
                      << " <= 0x" << std::hex << std::setw(8) << std::setfill('0')
                      << value << std::dec << "\n";
        }
    }
}

uint32_t CPU::fetch32(uint32_t addr) {
    return bus_.load32(addr);
}

void CPU::decode_to_idex(const IFID& in, IDEX& out) const {
    out = IDEX{};
    if (!in.valid) {
        return;
    }

    out.valid = true;
    out.pc = in.pc;
    out.inst = in.inst;

    out.opcode = opcode_of(in.inst);
    out.rd = rd_of(in.inst);
    out.rs1 = rs1_of(in.inst);
    out.rs2 = rs2_of(in.inst);
    out.funct3 = funct3_of(in.inst);
    out.funct7 = funct7_of(in.inst);

    out.rs1_val = load_reg(out.rs1);
    out.rs2_val = load_reg(out.rs2);

    switch (out.opcode) {
        case 0x37: // LUI
            out.imm = imm_u(in.inst);
            out.reg_write = true;
            break;

        case 0x17: // AUIPC
            out.imm = imm_u(in.inst);
            out.reg_write = true;
            break;

        case 0x6f: // JAL
            out.imm = imm_j(in.inst);
            out.reg_write = true;
            out.is_jal = true;
            break;

        case 0x67: // JALR
            out.imm = imm_i(in.inst);
            out.reg_write = true;
            out.is_jalr = true;
            break;

        case 0x63: // BRANCH
            out.imm = imm_b(in.inst);
            out.is_branch = true;
            break;

        case 0x03: // LOAD
            out.imm = imm_i(in.inst);
            out.reg_write = true;
            out.mem_read = true;
            out.wb_from_mem = true;
            switch (out.funct3) {
                case 0x0: out.mem_size = 1; out.mem_unsigned = false; break; // LB
                case 0x1: out.mem_size = 2; out.mem_unsigned = false; break; // LH
                case 0x2: out.mem_size = 4; out.mem_unsigned = false; break; // LW
                case 0x4: out.mem_size = 1; out.mem_unsigned = true;  break; // LBU
                case 0x5: out.mem_size = 2; out.mem_unsigned = true;  break; // LHU
                default:
                    throw std::runtime_error("Unsupported load funct3");
            }
            break;

        case 0x23: // STORE
            out.imm = imm_s(in.inst);
            out.mem_write = true;
            switch (out.funct3) {
                case 0x0: out.mem_size = 1; break; // SB
                case 0x1: out.mem_size = 2; break; // SH
                case 0x2: out.mem_size = 4; break; // SW
                default:
                    throw std::runtime_error("Unsupported store funct3");
            }
            break;

        case 0x13: // OP-IMM
            out.imm = imm_i(in.inst);
            out.reg_write = true;
            break;

        case 0x33: // OP
            out.reg_write = true;
            break;

        case 0x73: // SYSTEM
            out.is_system = true;
            break;

        default:
            throw std::runtime_error("Unsupported opcode");
    }
}

uint32_t CPU::forward_operand(uint8_t rs, uint32_t original) const {
    if (rs == 0) {
        return 0;
    }

    if (ex_mem_.valid &&
        ex_mem_.reg_write &&
        !ex_mem_.wb_from_mem &&
        ex_mem_.rd != 0 &&
        ex_mem_.rd == rs) {
        return ex_mem_.alu_result;
    }

    if (mem_wb_.valid &&
        mem_wb_.reg_write &&
        mem_wb_.rd != 0 &&
        mem_wb_.rd == rs) {
        return mem_wb_.wb_value;
    }

    return original;
}

void CPU::run(uint64_t max_cycles) {
    uint64_t cycles = 0;
    while (!bus_.halted() && cycles < max_cycles) {
        step();
        cycles++;
    }

    if (cycles >= max_cycles) {
        throw std::runtime_error("Simulation stopped: max cycles reached");
    }
}

void CPU::step() {
    if (trace_) {
        std::cout << "\n[CYCLE " << cycle_ << "]\n";

        if (if_id_.valid) {
            std::cout << "  IF/ID : pc=0x" << std::hex << std::setw(8) << std::setfill('0') << if_id_.pc
                      << " inst=0x" << std::setw(8) << if_id_.inst << std::dec << "\n";
        } else {
            std::cout << "  IF/ID : <empty>\n";
        }

        if (id_ex_.valid) {
            std::cout << "  ID/EX : pc=0x" << std::hex << std::setw(8) << std::setfill('0') << id_ex_.pc
                      << " inst=0x" << std::setw(8) << id_ex_.inst
                      << " rd=x" << std::dec << static_cast<unsigned>(id_ex_.rd)
                      << " rs1=x" << static_cast<unsigned>(id_ex_.rs1)
                      << " rs2=x" << static_cast<unsigned>(id_ex_.rs2) << "\n";
        } else {
            std::cout << "  ID/EX : <empty>\n";
        }

        if (ex_mem_.valid) {
            std::cout << "  EX/MEM: pc=0x" << std::hex << std::setw(8) << std::setfill('0') << ex_mem_.pc
                      << " inst=0x" << std::setw(8) << ex_mem_.inst
                      << " rd=x" << std::dec << static_cast<unsigned>(ex_mem_.rd) << "\n";
        } else {
            std::cout << "  EX/MEM: <empty>\n";
        }

        if (mem_wb_.valid) {
            std::cout << "  MEM/WB: pc=0x" << std::hex << std::setw(8) << std::setfill('0') << mem_wb_.pc
                      << " inst=0x" << std::setw(8) << mem_wb_.inst
                      << " rd=x" << std::dec << static_cast<unsigned>(mem_wb_.rd) << "\n";
        } else {
            std::cout << "  MEM/WB: <empty>\n";
        }
    }

    // ---------------- WB ----------------
    if (mem_wb_.valid && mem_wb_.reg_write) {
        store_reg(mem_wb_.rd, mem_wb_.wb_value);
    }

    // ---------------- MEM ----------------
    MEMWB next_mem_wb{};

    if (ex_mem_.valid) {
        next_mem_wb.valid = true;
        next_mem_wb.pc = ex_mem_.pc;
        next_mem_wb.inst = ex_mem_.inst;
        next_mem_wb.rd = ex_mem_.rd;
        next_mem_wb.reg_write = ex_mem_.reg_write;
        next_mem_wb.wb_value = ex_mem_.alu_result;

        if (ex_mem_.mem_read) {
            switch (ex_mem_.mem_size) {
                case 1: {
                    uint8_t v = bus_.load8(ex_mem_.alu_result);
                    next_mem_wb.wb_value = ex_mem_.mem_unsigned
                        ? static_cast<uint32_t>(v)
                        : static_cast<uint32_t>(sign_extend(v, 8));
                    break;
                }
                case 2: {
                    uint16_t v = bus_.load16(ex_mem_.alu_result);
                    next_mem_wb.wb_value = ex_mem_.mem_unsigned
                        ? static_cast<uint32_t>(v)
                        : static_cast<uint32_t>(sign_extend(v, 16));
                    break;
                }
                case 4:
                    next_mem_wb.wb_value = bus_.load32(ex_mem_.alu_result);
                    break;
                default:
                    throw std::runtime_error("Invalid mem read size");
            }
        }

        if (ex_mem_.mem_write) {
            switch (ex_mem_.mem_size) {
                case 1:
                    bus_.store8(ex_mem_.alu_result, static_cast<uint8_t>(ex_mem_.store_data & 0xff));
                    break;
                case 2:
                    bus_.store16(ex_mem_.alu_result, static_cast<uint16_t>(ex_mem_.store_data & 0xffff));
                    break;
                case 4:
                    bus_.store32(ex_mem_.alu_result, ex_mem_.store_data);
                    break;
                default:
                    throw std::runtime_error("Invalid mem write size");
            }
            next_mem_wb.reg_write = false;
        }

        if (ex_mem_.halt) {
            bus_.store32(Bus::SIM_HALT, 1);
            next_mem_wb.reg_write = false;
        }
    }

    // ---------------- EX ----------------
    EXMEM next_ex_mem{};
    bool control_taken = false;
    uint32_t control_target = 0;

    if (id_ex_.valid) {
        next_ex_mem.valid = true;
        next_ex_mem.pc = id_ex_.pc;
        next_ex_mem.inst = id_ex_.inst;
        next_ex_mem.rd = id_ex_.rd;
        next_ex_mem.reg_write = id_ex_.reg_write;
        next_ex_mem.wb_from_mem = id_ex_.wb_from_mem;
        next_ex_mem.mem_read = id_ex_.mem_read;
        next_ex_mem.mem_write = id_ex_.mem_write;
        next_ex_mem.mem_size = id_ex_.mem_size;
        next_ex_mem.mem_unsigned = id_ex_.mem_unsigned;

        uint32_t a = forward_operand(id_ex_.rs1, id_ex_.rs1_val);
        uint32_t b = forward_operand(id_ex_.rs2, id_ex_.rs2_val);

        switch (id_ex_.opcode) {
            case 0x37: // LUI
                next_ex_mem.alu_result = static_cast<uint32_t>(id_ex_.imm);
                break;

            case 0x17: // AUIPC
                next_ex_mem.alu_result = id_ex_.pc + static_cast<uint32_t>(id_ex_.imm);
                break;

            case 0x6f: // JAL
                next_ex_mem.alu_result = id_ex_.pc + 4;
                control_taken = true;
                control_target = id_ex_.pc + static_cast<uint32_t>(id_ex_.imm);
                break;

            case 0x67: // JALR
                next_ex_mem.alu_result = id_ex_.pc + 4;
                control_taken = true;
                control_target = (a + static_cast<uint32_t>(id_ex_.imm)) & ~1u;
                break;

            case 0x63: { // BRANCH
                bool taken = false;
                switch (id_ex_.funct3) {
                    case 0x0: taken = (a == b); break; // BEQ
                    case 0x1: taken = (a != b); break; // BNE
                    case 0x4: taken = (static_cast<int32_t>(a) <  static_cast<int32_t>(b)); break; // BLT
                    case 0x5: taken = (static_cast<int32_t>(a) >= static_cast<int32_t>(b)); break; // BGE
                    case 0x6: taken = (a < b); break;  // BLTU
                    case 0x7: taken = (a >= b); break; // BGEU
                    default:
                        throw std::runtime_error("Unsupported branch funct3");
                }

                next_ex_mem.reg_write = false;
                if (taken) {
                    control_taken = true;
                    control_target = id_ex_.pc + static_cast<uint32_t>(id_ex_.imm);
                }
                break;
            }

            case 0x03: // LOAD
                next_ex_mem.alu_result = a + static_cast<uint32_t>(id_ex_.imm);
                break;

            case 0x23: // STORE
                next_ex_mem.alu_result = a + static_cast<uint32_t>(id_ex_.imm);
                next_ex_mem.store_data = b;
                next_ex_mem.reg_write = false;
                break;

            case 0x13: { // OP-IMM
                switch (id_ex_.funct3) {
                    case 0x0: next_ex_mem.alu_result = a + static_cast<uint32_t>(id_ex_.imm); break; // ADDI
                    case 0x2: next_ex_mem.alu_result = (static_cast<int32_t>(a) < id_ex_.imm) ? 1u : 0u; break; // SLTI
                    case 0x3: next_ex_mem.alu_result = (a < static_cast<uint32_t>(id_ex_.imm)) ? 1u : 0u; break; // SLTIU
                    case 0x4: next_ex_mem.alu_result = a ^ static_cast<uint32_t>(id_ex_.imm); break; // XORI
                    case 0x6: next_ex_mem.alu_result = a | static_cast<uint32_t>(id_ex_.imm); break; // ORI
                    case 0x7: next_ex_mem.alu_result = a & static_cast<uint32_t>(id_ex_.imm); break; // ANDI
                    case 0x1: {
                        uint32_t shamt = (id_ex_.inst >> 20) & 0x1f;
                        next_ex_mem.alu_result = a << shamt; // SLLI
                        break;
                    }
                    case 0x5: {
                        uint32_t shamt = (id_ex_.inst >> 20) & 0x1f;
                        if (id_ex_.funct7 == 0x00) {
                            next_ex_mem.alu_result = a >> shamt; // SRLI
                        } else if (id_ex_.funct7 == 0x20) {
                            next_ex_mem.alu_result = static_cast<uint32_t>(static_cast<int32_t>(a) >> shamt); // SRAI
                        } else {
                            throw std::runtime_error("Unsupported shift-imm funct7");
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Unsupported op-imm funct3");
                }
                break;
            }

            case 0x33: { // OP
                switch (id_ex_.funct3) {
                    case 0x0:
                        if (id_ex_.funct7 == 0x00) next_ex_mem.alu_result = a + b;      // ADD
                        else if (id_ex_.funct7 == 0x20) next_ex_mem.alu_result = a - b; // SUB
                        else throw std::runtime_error("Unsupported ADD/SUB funct7");
                        break;
                    case 0x1: next_ex_mem.alu_result = a << (b & 0x1f); break; // SLL
                    case 0x2: next_ex_mem.alu_result = (static_cast<int32_t>(a) < static_cast<int32_t>(b)) ? 1u : 0u; break; // SLT
                    case 0x3: next_ex_mem.alu_result = (a < b) ? 1u : 0u; break; // SLTU
                    case 0x4: next_ex_mem.alu_result = a ^ b; break; // XOR
                    case 0x5:
                        if (id_ex_.funct7 == 0x00) next_ex_mem.alu_result = a >> (b & 0x1f); // SRL
                        else if (id_ex_.funct7 == 0x20) next_ex_mem.alu_result = static_cast<uint32_t>(static_cast<int32_t>(a) >> (b & 0x1f)); // SRA
                        else throw std::runtime_error("Unsupported SRL/SRA funct7");
                        break;
                    case 0x6: next_ex_mem.alu_result = a | b; break; // OR
                    case 0x7: next_ex_mem.alu_result = a & b; break; // AND
                    default:
                        throw std::runtime_error("Unsupported op funct3");
                }
                break;
            }

            case 0x73: // SYSTEM
                next_ex_mem.reg_write = false;
                if (id_ex_.inst == 0x00000073 || id_ex_.inst == 0x00100073) {
                    next_ex_mem.halt = true;
                } else {
                    throw std::runtime_error("Unsupported system instruction");
                }
                break;

            default:
                throw std::runtime_error("Unsupported opcode in EX");
        }
    }

    next_ex_mem.control_taken = control_taken;
    next_ex_mem.control_target = control_target;

    // ---------------- Hazard detect ----------------
    bool stall = false;
    if (!control_taken && if_id_.valid && id_ex_.valid && id_ex_.mem_read && id_ex_.rd != 0) {
        uint8_t next_rs1 = rs1_of(if_id_.inst);
        uint8_t next_rs2 = rs2_of(if_id_.inst);

        if (uses_rs1(if_id_.inst) && next_rs1 == id_ex_.rd) {
            stall = true;
        }
        if (uses_rs2(if_id_.inst) && next_rs2 == id_ex_.rd) {
            stall = true;
        }
    }

    if (trace_) {
        if (stall) {
            std::cout << "    hazard: load-use stall inserted\n";
        }
        if (control_taken) {
            std::cout << "    control: flush, new pc=0x"
                      << std::hex << std::setw(8) << std::setfill('0')
                      << control_target << std::dec << "\n";
        }
    }

    // ---------------- ID / IF ----------------
    IDEX next_id_ex{};
    IFID next_if_id{};
    uint32_t next_pc = pc_;

    if (control_taken) {
        next_id_ex = IDEX{};
        next_if_id = IFID{};
        next_pc = control_target;
    } else if (stall) {
        next_id_ex = IDEX{};   // bubble
        next_if_id = if_id_;   // hold
        next_pc = pc_;         // hold PC
    } else {
        decode_to_idex(if_id_, next_id_ex);

        next_if_id.valid = true;
        next_if_id.pc = pc_;
        next_if_id.inst = fetch32(pc_);
        next_pc = pc_ + 4;

        if (trace_) {
            std::cout << "    IF: fetched pc=0x"
                      << std::hex << std::setw(8) << std::setfill('0') << next_if_id.pc
                      << " inst=0x" << std::setw(8) << next_if_id.inst
                      << std::dec << "\n";
        }
    }

    // ---------------- Commit ----------------
    mem_wb_ = next_mem_wb;
    ex_mem_ = next_ex_mem;
    id_ex_ = next_id_ex;
    if_id_ = next_if_id;
    pc_ = next_pc;

    regs_[0] = 0;
    cycle_++;
}
