#include "bus.h"
#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

Bus::Bus()
    : ram_(RAM_BASE, RAM_SIZE),
      start_time_(std::chrono::steady_clock::now()) {}

uint64_t Bus::read_timer_ms() const {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now() - start_time_).count()
    );
}

void Bus::poll_keyboard() {
    if (kbd_valid_) {
        return;
    }

    unsigned char ch = 0;
    ssize_t n = ::read(STDIN_FILENO, &ch, 1);

    if (n == 1) {
        kbd_data_ = ch;
        kbd_valid_ = true;
        return;
    }

    if (n == 0 || (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        ::usleep(1000);
        return;
    }

    if (n < 0) {
        return;
    }
}

uint8_t Bus::load8(uint32_t addr) {
    if (ram_.contains(addr)) return ram_.load8(addr);

    if (addr == UART_STATUS) return 1;
    if (addr == SIM_HALT) return halted_ ? 1 : 0;

    if (addr == KBD_STATUS) {
        poll_keyboard();
        return kbd_valid_ ? 1 : 0;
    }

    if (addr == KBD_DATA) {
        poll_keyboard();
        uint8_t value = kbd_valid_ ? kbd_data_ : 0;
        kbd_valid_ = false;
        return value;
    }

    if (addr == TIMER_LOW) {
        return static_cast<uint8_t>(read_timer_ms() & 0xffu);
    }

    if (addr == TIMER_HIGH) {
        return static_cast<uint8_t>((read_timer_ms() >> 32) & 0xffu);
    }

    throw std::runtime_error("Unsupported load8 address");
}

uint16_t Bus::load16(uint32_t addr) {
    if (ram_.contains(addr)) return ram_.load16(addr);

    if (addr == KBD_STATUS) {
        poll_keyboard();
        return kbd_valid_ ? 1 : 0;
    }

    if (addr == KBD_DATA) {
        poll_keyboard();
        uint16_t value = kbd_valid_ ? kbd_data_ : 0;
        kbd_valid_ = false;
        return value;
    }

    if (addr == TIMER_LOW) {
        return static_cast<uint16_t>(read_timer_ms() & 0xffffu);
    }

    if (addr == TIMER_HIGH) {
        return static_cast<uint16_t>((read_timer_ms() >> 32) & 0xffffu);
    }

    throw std::runtime_error("Unsupported load16 address");
}

uint32_t Bus::load32(uint32_t addr) {
    if (ram_.contains(addr)) return ram_.load32(addr);

    if (addr == UART_STATUS) return 1;
    if (addr == SIM_HALT) return halted_ ? 1u : 0u;

    if (addr == KBD_STATUS) {
        poll_keyboard();
        return kbd_valid_ ? 1u : 0u;
    }

    if (addr == KBD_DATA) {
        poll_keyboard();
        uint32_t value = kbd_valid_ ? static_cast<uint32_t>(kbd_data_) : 0u;
        kbd_valid_ = false;
        return value;
    }

    if (addr == TIMER_LOW) {
        return static_cast<uint32_t>(read_timer_ms() & 0xffffffffu);
    }

    if (addr == TIMER_HIGH) {
        return static_cast<uint32_t>((read_timer_ms() >> 32) & 0xffffffffu);
    }

    throw std::runtime_error("Unsupported load32 address");
}

void Bus::store8(uint32_t addr, uint8_t value) {
    if (ram_.contains(addr)) {
        ram_.store8(addr, value);
        return;
    }

    if (addr == UART_TX) {
        std::cout << static_cast<char>(value);
        std::cout.flush();
        return;
    }

    if (addr == SIM_HALT) {
        halted_ = (value != 0);
        return;
    }

    throw std::runtime_error("Unsupported store8 address");
}

void Bus::store16(uint32_t addr, uint16_t value) {
    if (ram_.contains(addr)) {
        ram_.store16(addr, value);
        return;
    }

    throw std::runtime_error("Unsupported store16 address");
}

void Bus::store32(uint32_t addr, uint32_t value) {
    if (ram_.contains(addr)) {
        ram_.store32(addr, value);
        return;
    }

    if (addr == UART_TX) {
        std::cout << static_cast<char>(value & 0xff);
        std::cout.flush();
        return;
    }

    if (addr == SIM_HALT) {
        halted_ = (value != 0);
        return;
    }

    throw std::runtime_error("Unsupported store32 address");
}

void Bus::load_program(uint32_t addr, const std::vector<uint8_t>& data) {
    ram_.load_blob(addr, data);
}
