#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//Checks if the next set of tokens represent a memory address token (format -X[reg] or [reg]) and returns an operand if so
operand_t memory_operand(parser_t *parser) {
	operand_t operand;
	token_t *current_token = parser->lexer->tokens[parser->pos];
	int token_index = parser->pos; // we cannot use the real one because it needs to revert back if unsuccessful.

	//-X[reg] X[reg] or [reg] all acceptable
	if(current_token->token != T_MINUS && current_token->token != T_LBRACKET && current_token->token != T_INTLIT) {
		operand.flags = OP_INVALID;
		return operand;
	}

	if(current_token->token == T_MINUS) {
		token_index++;
		current_token = parser->lexer->tokens[token_index];
		if(current_token->token == T_INTLIT) {
			operand.mem_offset = current_token->int_value * -1;
			token_index++; //hopefully to '[' of register
			current_token = parser->lexer->tokens[token_index];
		}else {
			printf("Unexpected '-' in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
			exit(1);
		}
	}else if(current_token->token == T_INTLIT) {
		operand.mem_offset = current_token->int_value;
		token_index++; //hopefully to '[' of register
		current_token = parser->lexer->tokens[token_index];
	}

	if(current_token->token != T_LBRACKET) {
		operand.flags = OP_INVALID;
		return operand;
	}

	operand.mem_offset = 0;

	token_index++; // '[' to reg name
	current_token = parser->lexer->tokens[token_index];

	//is a register
	if(current_token->keyword >= K_R8 && current_token->keyword <= K_AL) {
		operand.reg = current_token->keyword;
	}else {
		printf("Expected register within brackets in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
		exit(1);
	}

	token_index++; //reg name to ']'
	current_token = parser->lexer->tokens[token_index];

	if(current_token->token != T_RBRACKET) {
		printf("Expected right square bracket after register name in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
		exit(1);
	}

	parser->pos = token_index;
	operand.flags = OP_MEMORY;

	return operand;
}

//Flags: OP_REGISTER, OP_INTLIT, OP_MEMORY
//Use like OP_REGISTER | OP_INTLIT to accept both register and integer literal
operand_t parse_operand(parser_t *parser, uint8_t flags) {
	operand_t operand;
	token_t *token = parser->lexer->tokens[parser->pos];

	uint8_t flag_register = flags & OP_REGISTER;
	uint8_t flag_intlit = (flags & OP_INTLIT) >> 1;
	uint8_t flag_memory = (flags & OP_MEMORY) >> 2;

	if((operand = memory_operand(parser)).flags != OP_INVALID) {
		if(!flag_memory) {
			printf("Invalid memory operand and opcode combination on line %d\n", token->line_num);
			exit(1);
		}
	}else if(token->keyword >= K_RAX && token->keyword <= K_R15B) {
		if(!flag_register) {
			printf("Invalid register operand \"%s\" and opcode combination on line %d\n", token->ident_value, token->line_num);
			exit(1);
		}
		operand.reg = token->keyword;
		operand.flags = OP_REGISTER;
	}else if(token->token == T_INTLIT) {
		if(!flag_intlit) {
			printf("Invalid intlit operand %d and opcode combination on line %d\n", token->int_value, token->line_num);
			exit(1);
		}
		operand.intlit = token->int_value;
		operand.flags = OP_INTLIT;
	}else {
		printf("Invalid operand \"%s\" and opcode combination on line %d\n", token->ident_value, token->line_num);
		exit(1);
	}
	return operand;
}

void require_comma(parser_t *parser) {
	if(parser->lexer->tokens[parser->pos]->token != T_COMMA) {
		printf("Excepted comma after first operand on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	parser->pos++;
}

void require_newline(parser_t *parser) {
	if(parser->lexer->tokens[parser->pos]->token != T_NEWLINE) {
		printf("Excepted newline on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	parser->pos++;
}

instruction_t parse_mov(parser_t *parser) {
	instruction_t instr;

	instr.opcode = K_MOV;
	parser->pos++; //past opcode
	instr.operand_1 = parse_operand(parser, OP_REGISTER | OP_MEMORY);
	parser->pos++;
	require_comma(parser);
	instr.operand_2 = parse_operand(parser, OP_REGISTER | OP_MEMORY | OP_INTLIT);
	parser->pos++;

	return instr;
}

instruction_t parse_add(parser_t *parser) {
	instruction_t instr;

	instr.opcode = K_ADD;
	parser->pos++; //past opcode
	instr.operand_1 = parse_operand(parser, OP_REGISTER | OP_MEMORY);
	parser->pos++;
	require_comma(parser);
	instr.operand_2 = parse_operand(parser, OP_REGISTER | OP_MEMORY | OP_INTLIT);
	parser->pos++;

	return instr;
}

instruction_t parse_syscall(parser_t *parser) {
	instruction_t instr;

	instr.opcode = K_SYSCALL;
	instr.operand_1.reg = 0;
	instr.operand_2.reg = 0;

	parser->pos++;

	return instr;
}
 
instruction_t parse_line(parser_t *parser) {
	//We are expecting that each newline starts with an opcode
	switch(parser->lexer->tokens[parser->pos]->keyword) {
		case K_MOV:
			return parse_mov(parser);
		case K_ADD:
			return parse_add(parser);
		case K_SYSCALL:
			return parse_syscall(parser);
		default:
			printf("Invalid opcode \"%s\" on line %d\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->line_num);
			exit(1);
	}
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
		memset(&current_instruction, 0, sizeof(current_instruction));
		instruction_t current_instruction = parse_line(parser);

		instruction_t *permenant_instruction = (instruction_t *)malloc(sizeof(instruction_t));
		permenant_instruction->opcode = current_instruction.opcode;
		permenant_instruction->operand_1 = current_instruction.operand_1;
		permenant_instruction->operand_2 = current_instruction.operand_2;

		printf("%d - %d - %d\n", current_instruction.opcode, current_instruction.operand_1.reg, current_instruction.operand_2.reg);

		parser->instructions[parser->instruction_index++] = permenant_instruction;

		if(parser->lexer->tokens[parser->pos]->token == T_EOF) continue;
		if(parser->lexer->tokens[parser->pos]->token == T_SEMICOLON) {
			while(parser->lexer->tokens[parser->pos]->token != T_NEWLINE) parser->pos++;
		}
		require_newline(parser);
	}
}