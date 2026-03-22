#pragma once
#include <chrono>
#include <cstdint>
#include <vector>
#include "memory.h"

class Bus {
public:
    static constexpr uint32_t RAM_BASE = 0x80000000;
    static constexpr uint32_t RAM_SIZE = 1024 * 1024; // 1 MB

    static constexpr uint32_t UART_TX     = 0x10000000;
    static constexpr uint32_t UART_STATUS = 0x10000004;
    static constexpr uint32_t SIM_HALT    = 0x10000008;

    static constexpr uint32_t KBD_STATUS  = 0x10000010;
    static constexpr uint32_t KBD_DATA    = 0x10000014;

    static constexpr uint32_t TIMER_LOW   = 0x10000018;
    static constexpr uint32_t TIMER_HIGH  = 0x1000001c;

    Bus();

    uint8_t  load8(uint32_t addr);
    uint16_t load16(uint32_t addr);
    uint32_t load32(uint32_t addr);

    void store8(uint32_t addr, uint8_t value);
    void store16(uint32_t addr, uint16_t value);
    void store32(uint32_t addr, uint32_t value);

    void load_program(uint32_t addr, const std::vector<uint8_t>& data);

    bool halted() const { return halted_; }

private:
    void poll_keyboard();
    uint64_t read_timer_ms() const;

    Memory ram_;
    bool halted_ = false;

    bool kbd_valid_ = false;
    uint8_t kbd_data_ = 0;

    std::chrono::steady_clock::time_point start_time_;
};
