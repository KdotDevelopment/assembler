#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

instruction_t parse_two_arg(parser_t *parser) {
	//expecting to be on the first arg
	instruction_t args;

	if(parser->lexer->tokens[parser->pos]->keyword >= 1 && parser->lexer->tokens[parser->pos]->keyword <= 25) {
		args.arg_1.reg = parser->lexer->tokens[parser->pos]->keyword;
	}else {
		printf("Invalid register \"%s\" after opcode on line %d\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}

	parser->pos++; //after arg, to comma
	if(parser->lexer->tokens[parser->pos]->token != T_COMMA) {
		printf("Expected comma after first argument on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	parser->pos++; //after comma, to arg2

	if(parser->lexer->tokens[parser->pos]->keyword >= 1 && parser->lexer->tokens[parser->pos]->keyword <= 25) {
		args.arg_2.reg = parser->lexer->tokens[parser->pos]->keyword;
	}else if(parser->lexer->tokens[parser->pos]->token == T_INTLIT) {
		args.arg_2.intlit = parser->lexer->tokens[parser->pos]->int_value;
	}else {
		printf("Invalid register \"%s\" or intlit %d after comma on line %d\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->int_value, parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}

	parser->pos++; //past last arg, to newline
}

instruction_t parse_line(parser_t *parser) {
	instruction_t instr;
	instruction_t args;

	//We are expecting that each newline starts with an opcode
	switch(parser->lexer->tokens[parser->pos]->keyword) {
		case K_MOV:
			instr.opcode = K_MOV;
			parser->pos++; //past opcode
			args = parse_two_arg(parser);
			instr.arg_1 = args.arg_1;
			instr.arg_2 = args.arg_2;
			break;
		case K_ADD:
			instr.opcode = K_ADD;
			parser->pos++; //past opcode
			args = parse_two_arg(parser);
			instr.arg_1 = args.arg_1;
			instr.arg_2 = args.arg_2;
			break;
		case K_SYSCALL:
			break;
		default:
			printf("Invalid opcode \"%s\" on line %d\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->line_num);
			exit(1);
	}

	return instr;
}

void parse(parser_t *parser) {
    size_t max_instruction_count = 128;
	parser->pos = 0;
	parser->instructions = (instruction_t **)malloc(max_instruction_count * sizeof(instruction_t *));

	instruction_t current_instruction;

	//get after "main:"
	while(strcmp(parser->lexer->tokens[parser->pos]->ident_value, "main") != 0 || parser->lexer->tokens[parser->pos + 1]->token != T_COLON) {
		parser->pos++;
	}

	parser->pos++; //past main
	parser->pos++; //past :
	parser->pos++; //past \n

	//this begins the instruction part
	while(parser->lexer->tokens[parser->pos]->token != T_EOF) {
		if(parser->instruction_index > max_instruction_count) {
			max_instruction_count *= 2;
			parser->instructions = (instruction_t **)realloc(parser->instructions, max_instruction_count * sizeof(instruction_t *));
		}
		instruction_t current_instruction = parse_line(parser);
		parser->pos++; //past newline, into next line

		instruction_t *permenant_instruction = (instruction_t *)malloc(sizeof(instruction_t));
		permenant_instruction->opcode = current_instruction.opcode;
		permenant_instruction->arg_1 = current_instruction.arg_1;
		permenant_instruction->arg_2 = current_instruction.arg_2;

		parser->instructions[parser->instruction_index++] = permenant_instruction;
	}
}