#include "elf_loader.h"
#include "bus.h"

#include <elf.h>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

static std::vector<uint8_t> read_all_bytes(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );
}

bool is_elf_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return false;
    }

    unsigned char ident[4] = {0, 0, 0, 0};
    ifs.read(reinterpret_cast<char*>(ident), 4);
    if (ifs.gcount() != 4) {
        return false;
    }

    return ident[0] == 0x7f &&
           ident[1] == 'E' &&
           ident[2] == 'L' &&
           ident[3] == 'F';
}

LoadedImage load_elf_into_bus(const std::string& path, Bus& bus) {
    std::vector<uint8_t> bytes = read_all_bytes(path);

    if (bytes.size() < sizeof(Elf32_Ehdr)) {
        throw std::runtime_error("ELF file too small");
    }

    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(bytes.data());

    if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
          ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
          ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
          ehdr->e_ident[EI_MAG3] == ELFMAG3)) {
        throw std::runtime_error("Not a valid ELF file");
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        throw std::runtime_error("Only ELF32 is supported");
    }

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        throw std::runtime_error("Only little-endian ELF is supported");
    }

    if (ehdr->e_machine != EM_RISCV) {
        throw std::runtime_error("ELF machine is not RISC-V");
    }

    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        throw std::runtime_error("ELF has no program headers");
    }

    size_t ph_end = static_cast<size_t>(ehdr->e_phoff) +
                    static_cast<size_t>(ehdr->e_phnum) * sizeof(Elf32_Phdr);
    if (ph_end > bytes.size()) {
        throw std::runtime_error("ELF program header table out of range");
    }

    const Elf32_Phdr* phdrs =
        reinterpret_cast<const Elf32_Phdr*>(bytes.data() + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; ++i) {
        const Elf32_Phdr& ph = phdrs[i];

        if (ph.p_type != PT_LOAD) {
            continue;
        }

        if (ph.p_memsz < ph.p_filesz) {
            throw std::runtime_error("ELF segment memsz < filesz");
        }

        size_t seg_file_end = static_cast<size_t>(ph.p_offset) +
                              static_cast<size_t>(ph.p_filesz);
        if (seg_file_end > bytes.size()) {
            throw std::runtime_error("ELF segment exceeds file size");
        }

        std::vector<uint8_t> segment(static_cast<size_t>(ph.p_memsz), 0);

        for (size_t j = 0; j < ph.p_filesz; ++j) {
            segment[j] = bytes[static_cast<size_t>(ph.p_offset) + j];
        }

        uint32_t load_addr = ph.p_vaddr;
        bus.load_program(load_addr, segment);
    }

    LoadedImage image;
    image.entry = ehdr->e_entry;
    return image;
}
