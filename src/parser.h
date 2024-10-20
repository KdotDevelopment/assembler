#pragma once

#include "lex.h"
#include "symbol.h"

#include <stdint.h>

#define OP_REGISTER 0x01
#define OP_INTLIT 0x02
#define OP_MEMORY 0x04
#define OP_LABEL 0x08
#define OP_8BIT 0x10
#define OP_16BIT 0x20
#define OP_32BIT 0x40
#define OP_64BIT 0x80
#define OP_INVALID 0x100

typedef struct operand_t {
    uint16_t flags;

    int reg; //also base for memory
    int64_t intlit;
    char *label_name; //if operand is a label

    int64_t mem_displacement; // -X[reg] (includes sign)
    uint8_t mem_scale;        // [reg * X] (includes sign)
    int index_reg; // disp_reg[reg]
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
	symbol_table_t *symbol_table;
    instruction_t **instructions;
} parser_t;

void parse(parser_t *parser);
uint8_t get_register_size(int reg);