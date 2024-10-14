#pragma once

typedef struct {
    uint8_t e_ident[16];    // ELF Identification
    uint16_t e_type;        // Object file type (2 for executable)
    uint16_t e_machine;     // Target architecture (0x3E for x86_64)
    uint32_t e_version;     // Version
    uint64_t e_entry;       // Entry point address
    uint64_t e_phoff;       // Program header table offset
    uint64_t e_shoff;       // Section header table offset
    uint32_t e_flags;       // Flags (not used)
    uint16_t e_ehsize;      // ELF header size
    uint16_t e_phentsize;   // Program header size
    uint16_t e_phnum;       // Number of program headers
    uint16_t e_shentsize;   // Section header size (not used)
    uint16_t e_shnum;       // Number of section headers (not used)
    uint16_t e_shstrndx;    // Section header string table index (not used)
} header_t;

typedef struct {
    uint32_t p_type;        // Segment type (1 for loadable)
    uint32_t p_flags;       // Segment flags (5 for read+execute)
    uint64_t p_offset;      // Offset of segment in file
    uint64_t p_vaddr;       // Virtual address in memory
    uint64_t p_paddr;       // Physical address (not used)
    uint64_t p_filesz;      // Size of segment in file
    uint64_t p_memsz;       // Size of segment in memory
    uint64_t p_align;       // Alignment
} program_header_t;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} section_header_t;