#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "elf.h"
#include "lex.h"
#include "parser.h"
#include "gen.h"

char buffer[24];
char *get_file_name(char *full_name) {
	int i = 0;
	while(*full_name != 0 && *full_name != '.') {
		buffer[i] = *full_name;
		i++;
		full_name++;
	}

	return buffer;
}

char *add_file_ext(char *base_name, char *ext) {
	char *new_file_name = malloc(sizeof(base_name) + strlen(ext));
	int name_length = strlen(base_name);
	strcpy(new_file_name, base_name);
	strncat(new_file_name, ext, sizeof(ext));
}

// Define the machine code (x86-64)
uint8_t machine_code[] = { 0x49, 0xC7, 0xC0, 0x09, 0x00, 0x00, 0x00, 0x49, 0x83, 0xC0, 0x0A, 0x49, 0x83, 0xC0, 0x05, 0x4C, 0x89, 0xC7, 0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00 }  ;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Usage: rasm <filename>\n");
		return 1;
	}

    lexer_t lexer;

    if((lexer.in_file = fopen(argv[1], "r")) == NULL) {
		printf("Unable to open file: %s\n", argv[1]);
		return 1;
	}

    lexer.out_file = fopen(add_file_ext(get_file_name(argv[1]), ".o"), "w"); // w = writes to a file, clears first

    header_t elf_header;
    memset(&elf_header, 0, sizeof(header_t));
    memcpy(elf_header.e_ident, "\x7f" "ELF" "\x02\x01\x01", 7); // ELF magic and class
    elf_header.e_type = 2;            // Executable file
    elf_header.e_machine = 0x3E;      // x86-64
    elf_header.e_version = 1;         // Version 1
    elf_header.e_entry = 0x401000;      // Entry point (where the code starts in memory)
    elf_header.e_phoff = sizeof(header_t); // Program header follows ELF header
    elf_header.e_shoff = 0x1028; //magic number???
    elf_header.e_ehsize = sizeof(header_t);
    elf_header.e_phentsize = 56;//sizeof(program_header_t);
    elf_header.e_phnum = 2;           // Two program headers
    elf_header.e_shentsize = sizeof(section_header_t);
    elf_header.e_shnum = 3;
    elf_header.e_shstrndx = 2; //index of section header string table

    // Program header
    program_header_t prog_header;
    memset(&prog_header, 0, sizeof(program_header_t));
    prog_header.p_type = 1;           // Loadable segment
    prog_header.p_flags = 4;          // Read + execute
    prog_header.p_offset = 0x0;  // Segment starts at file start
    prog_header.p_vaddr = 0x400000;   // Virtual address (typical base for executables)
    prog_header.p_paddr = 0x400000;   // Physical address (same for executable)
    prog_header.p_filesz = sizeof(elf_header) + 2 * sizeof(prog_header); // File size
    prog_header.p_memsz = sizeof(elf_header) + 2 * sizeof(prog_header); // Memory size
    prog_header.p_align = 0x1000;

    program_header_t prog_header2;
    memset(&prog_header2, 0, sizeof(program_header_t));
    prog_header2.p_type = 1;           // Loadable segment
    prog_header2.p_flags = 5;          // Read + write
    prog_header2.p_offset = 0x1000;
    prog_header2.p_vaddr = 0x401000;   // Virtual address (typical base for executables)
    prog_header2.p_paddr = 0x401000;   // Physical address (same for executable)
    prog_header2.p_filesz = sizeof(machine_code); // File size
    prog_header2.p_memsz = sizeof(machine_code); // Memory size
    prog_header2.p_align = 0x0;

    // Write the ELF header
    fwrite(&elf_header, sizeof(elf_header), 1, lexer.out_file);

    // Write the Program header
    fwrite(&prog_header, sizeof(prog_header), 1, lexer.out_file);

    fwrite(&prog_header2, sizeof(prog_header2), 1, lexer.out_file);

	fseek(lexer.out_file, 0x1000, 0);

    //make the machine code
    lex(&lexer);

    parser_t parser;
    parser.lexer = &lexer;

    parse(&parser);

    code_gen_t code_gen;
    code_gen.parser = &parser;
    code_gen.out = lexer.out_file;
    code_gen.instruction_pos = 0;

    //fwrite(machine_code, sizeof(machine_code), 1, lexer.out_file);

    generate_code(&code_gen);

    clean_tokens(&lexer);

    // Close the file
    fclose(lexer.out_file);

	return 0;
}