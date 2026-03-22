#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <cstdint>

#include "bus.h"
#include "cpu.h"
#include "elf_loader.h"

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );
}

class TerminalRawMode {
public:
    explicit TerminalRawMode(bool enable) {
        if (!enable) return;
        if (!::isatty(STDIN_FILENO)) return;

        if (::tcgetattr(STDIN_FILENO, &old_term_) != 0) {
            throw std::runtime_error("tcgetattr failed");
        }

        int flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags < 0) {
            throw std::runtime_error("fcntl(F_GETFL) failed");
        }
        old_flags_ = flags;

        struct termios raw = old_term_;
        raw.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
            throw std::runtime_error("tcsetattr failed");
        }

        if (::fcntl(STDIN_FILENO, F_SETFL, old_flags_ | O_NONBLOCK) != 0) {
            ::tcsetattr(STDIN_FILENO, TCSANOW, &old_term_);
            throw std::runtime_error("fcntl(F_SETFL) failed");
        }

        enabled_ = true;
    }

    ~TerminalRawMode() {
        if (!enabled_) return;
        ::tcsetattr(STDIN_FILENO, TCSANOW, &old_term_);
        ::fcntl(STDIN_FILENO, F_SETFL, old_flags_);
    }

    TerminalRawMode(const TerminalRawMode&) = delete;
    TerminalRawMode& operator=(const TerminalRawMode&) = delete;

private:
    bool enabled_ = false;
    struct termios old_term_{};
    int old_flags_ = 0;
};

int main(int argc, char** argv) {
    try {
        bool trace = false;
        bool dump_regs = false;
        bool raw_kbd = false;
        uint64_t max_cycles = 1000000;
        std::string program_path;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--trace") {
                trace = true;
            } else if (arg == "--dump-regs") {
                dump_regs = true;
            } else if (arg == "--raw-kbd") {
                raw_kbd = true;
            } else if (arg == "--max-cycles") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value after --max-cycles\n";
                    return 1;
                }
                max_cycles = std::stoull(argv[++i]);
            } else if (program_path.empty()) {
                program_path = arg;
            } else {
                std::cerr << "Usage: rv32i_sim [--trace] [--dump-regs] [--raw-kbd] [--max-cycles N] <program.bin|program.elf>\n";
                return 1;
            }
        }

        if (program_path.empty()) {
            std::cerr << "Usage: rv32i_sim [--trace] [--dump-regs] [--raw-kbd] [--max-cycles N] <program.bin|program.elf>\n";
            return 1;
        }

        if (raw_kbd && max_cycles == 1000000) {
            max_cycles = 1000000000ULL;
        }

        TerminalRawMode raw_mode(raw_kbd);

        Bus bus;
        CPU cpu(bus);
        cpu.set_trace(trace);

        uint32_t entry = Bus::RAM_BASE;

        if (is_elf_file(program_path)) {
            LoadedImage image = load_elf_into_bus(program_path, bus);
            entry = image.entry;
            std::cout << "[LOADER] ELF loaded, entry = 0x"
                      << std::hex << entry << std::dec << "\n";
        } else {
            std::vector<uint8_t> prog = read_file(program_path);
            bus.load_program(entry, prog);
            std::cout << "[LOADER] Raw binary loaded at 0x"
                      << std::hex << entry << std::dec << "\n";
        }

        if (raw_kbd) {
            std::cout << "[HOST] raw keyboard mode enabled (press guest-defined quit key to exit)\n";
        }

        cpu.reset(entry);
        cpu.run(max_cycles);

        if (dump_regs) {
            cpu.dump_regs();
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[SIM ERROR] " << e.what() << "\n";
        return 1;
    }
}
