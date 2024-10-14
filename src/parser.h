#pragma once

#include "lex.h"

#include <stdint.h>

#define OP_REGISTER 0x01
#define OP_INTLIT 0x02
#define OP_MEMORY 0x04
#define OP_INVALID 0x08

typedef struct operand_t {
    uint8_t flags;
    union {
        int reg;
        int intlit;
    };
    int64_t mem_offset; // -X[reg] (includes sign)
} operand_t;

//this is basically every line of code
typedef struct instruction_t {
    int opcode; //uses keywords from lex.h
    operand_t operand_1;
    operand_t operand_2;
} instruction_t;

typedef struct parser_t {
	lexer_t *lexer;
	size_t pos; //which token we are on
    size_t instruction_index; //the current instruction being worked on
	//symbol_table_t *symbol_table;
    instruction_t **instructions;
} parser_t;

void parse(parser_t *parser);