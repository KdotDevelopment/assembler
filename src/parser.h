#pragma once

#include "lex.h"

#include <stdint.h>

#define OP_REGISTER 0x01
#define OP_INTLIT 0x02
#define OP_MEMORY 0x04
#define OP_8BIT 0x08
#define OP_16BIT 0x10
#define OP_32BIT 0x20
#define OP_64BIT 0x40
#define OP_INVALID 0x80

typedef struct operand_t {
    uint8_t flags;
    union {
        uint8_t reg; //also base for memory
        int64_t intlit;
    };
    int64_t mem_offset; // -X[reg] (includes sign) (aka mem displacement)
    int64_t mem_scale;  //   [reg * X] (includes sign)
    uint8_t index_reg;
    uint8_t size; //size in bytes of this operand
} operand_t;

//this is basically every line of code
typedef struct instruction_t {
    int opcode; //uses keywords from lex.h
    operand_t operand_1;
    operand_t operand_2;
    size_t line_num;
} instruction_t;

typedef struct parser_t {
	lexer_t *lexer;
	size_t pos; //which token we are on
    size_t instruction_index; //the current instruction being worked on
	//symbol_table_t *symbol_table;
    instruction_t **instructions;
} parser_t;

void parse(parser_t *parser);
uint8_t get_register_size(int reg);