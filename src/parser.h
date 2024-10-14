#pragma once

#include "lex.h"

//this is basically every line of code
typedef struct instruction_t {
    int opcode; //uses keywords from lex.h
    union {
        int reg;
        int intlit;
    } arg_1;
    
    union {
        int reg;
        int intlit;
    } arg_2;
} instruction_t;

typedef struct parser_t {
	lexer_t *lexer;
	size_t pos; //which token we are on
    size_t instruction_index; //the current instruction being worked on
	//symbol_table_t *symbol_table;
    instruction_t **instructions;
} parser_t;

void parse(parser_t *parser);