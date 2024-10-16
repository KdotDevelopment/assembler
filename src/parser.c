#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//takes in register keyword, outputs num of bytes
uint8_t get_register_size(int reg) {
    if(reg >= K_RAX && reg <= K_R15) return 8;
    if(reg >= K_EAX && reg <= K_R15D) return 4;
    if(reg >= K_AX && reg <= K_R15W) return 2;
    if(reg >= K_AL && reg <= K_R15B) return 1;
}

void set_operand_size(operand_t *operand) {
	if(!operand) return;

	if(operand->flags & OP_REGISTER || operand->flags & OP_MEMORY) {
		operand->size = get_register_size(operand->reg);
		return;
	}

	if(operand->flags & OP_INTLIT) {
		if(operand->intlit < 128 && operand->intlit >= -128) operand->size = 1; return;
		if(operand->intlit < 32768 && operand->intlit >= -32768) operand->size = 2; return;
		if(operand->intlit < 2147483648 && operand->intlit >= -2147483648) operand->size = 4; return;
		if(operand->intlit < 9223372036854775807 && operand->intlit >= -9223372036854775807) operand->size = 8; return;
	}
}

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
	if(current_token->keyword >= K_RAX && current_token->keyword <= K_R15D) {
		operand.reg = current_token->keyword;
	}else {
		printf("Expected 32/64-bit register within brackets in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
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

//BYTE WORD DWORD QWORD
uint8_t parse_size_modifier(parser_t *parser) {
	int keyword = parser->lexer->tokens[parser->pos]->keyword;

	switch(keyword) {
		case K_BYTE:
			parser->pos++;
			return 1;
		case K_WORD:
			parser->pos++;
			return 2;
		case K_DWORD:
			parser->pos++;
			return 4;
		case K_QWORD:
			parser->pos++;
			return 8;
		default:
			return 0;
	}
}

//Flags: OP_REGISTER, OP_INTLIT, OP_MEMORY
//Use like OP_REGISTER | OP_INTLIT to accept both register and integer literal
operand_t parse_operand(parser_t *parser, uint8_t flags) {
	operand_t operand;

	//check for byte word dword qword and set size (otherwise size is set automatically)
	uint8_t force_size = parse_size_modifier(parser);

	token_t *token = parser->lexer->tokens[parser->pos];

	uint8_t flag_register = flags & OP_REGISTER;
	uint8_t flag_intlit = (flags & OP_INTLIT) >> 1;
	uint8_t flag_memory = (flags & OP_MEMORY) >> 2;

	if((operand = memory_operand(parser)).flags != OP_INVALID) {
		if(!flag_memory) {
			printf("Invalid memory operand and opcode combination on line %d\n", token->line_num);
			exit(1);
		}
		operand.size = force_size;
	}else if(token->keyword >= K_RAX && token->keyword <= K_R15B) {
		if(!flag_register) {
			printf("Invalid register operand \"%s\" and opcode combination on line %d\n", token->ident_value, token->line_num);
			exit(1);
		}
		operand.reg = token->keyword;
		operand.flags = OP_REGISTER;
		operand.size = force_size;
	}else if(token->token == T_INTLIT) {
		if(!flag_intlit) {
			printf("Invalid intlit operand %d and opcode combination on line %d\n", token->int_value, token->line_num);
			exit(1);
		}
		operand.intlit = token->int_value;
		operand.flags = OP_INTLIT;
		operand.size = force_size;
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

instruction_t parse_no_operands(parser_t *parser, int opcode) {
	instruction_t instr;

	instr.opcode = opcode;
	instr.operand_1.reg = 0;
	instr.operand_2.reg = 0;

	parser->pos++;

	return instr;
}

void is_operand_valid(parser_t *parser, operand_t *operand, uint8_t flags) {
	set_operand_size(operand);
	uint8_t size = operand->size;
	uint8_t must_check = flags & 0b01111000; //filter only size flags -- if present we must check all of them. if not, do not check size at all

	if(!must_check) return;

	if(size == 1 && !(flags & OP_8BIT)) {
		printf("8-bit operand not allowed in operand on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	if(size == 2 && !(flags & OP_16BIT)) {
		printf("16-bit operand not allowed in operand on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	if(size == 4 && !(flags & OP_32BIT)) {
		printf("32-bit operand not allowed in operand on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	if(size == 8 && !(flags & OP_64BIT)) {
		printf("64-bit operand not allowed in operand on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
}

instruction_t parse_instruction(parser_t *parser, int opcode, uint8_t operand_1_flags, uint8_t operand_2_flags) {
	instruction_t instr;

	instr.opcode = opcode;
	parser->pos++; //past opcode
	instr.operand_1 = parse_operand(parser, operand_1_flags & 0b00000111); //last three bits are reg, mem, imm, rest are amount of bits

	is_operand_valid(parser, &instr.operand_1, operand_1_flags);

	parser->pos++;

	if(operand_2_flags & OP_INVALID) {
		operand_t invalid_op;
		invalid_op.flags = OP_INVALID;
		instr.operand_2 = invalid_op;
		return instr;
	}

	require_comma(parser);
	instr.operand_2 = parse_operand(parser, operand_2_flags & 0b00000111); //last three bits are reg, mem, imm, rest are amount of bits

	is_operand_valid(parser, &instr.operand_2, operand_2_flags);

	parser->pos++;

	return instr;
}

instruction_t parse_line(parser_t *parser) {
	//We are expecting that each newline starts with an opcode
	if(parser->lexer->tokens[parser->pos]->token != T_IDENT) {
		instruction_t invalid;
		invalid.opcode = -1;
		return invalid;
	}

	int keyword = parser->lexer->tokens[parser->pos]->keyword;

	//Xbit flags are strict if any are used, if none are defined then any amount is allowed
	switch(keyword) {
		case K_MOVZX:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_MEMORY | OP_16BIT | OP_32BIT | OP_64BIT, OP_REGISTER | OP_MEMORY | OP_8BIT | OP_16BIT);
		case K_MOV:
		case K_ADD:
		case K_OR:
		case K_ADC:
		case K_SBB:
		case K_AND:
		case K_SUB:
		case K_CMP:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_MEMORY, OP_REGISTER | OP_MEMORY | OP_INTLIT);
		case K_SETO:
		case K_SETNO:
		case K_SETB:
		case K_SETNB:
		case K_SETZ:
		case K_SETNZ:
		case K_SETBE:
		case K_SETA:
		case K_SETS:
		case K_SETNS:
		case K_SETP:
		case K_SETNP:
		case K_SETL:
		case K_SETGE:
		case K_SETLE:
		case K_SETG:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_8BIT, OP_INVALID);
		case K_TEST:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_MEMORY, OP_INTLIT);
		case K_NEG:
		case K_NOT:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_MEMORY, OP_INVALID);
		case K_MUL:
		case K_IMUL:
		case K_DIV:
		case K_IDIV:
			return parse_instruction(parser, keyword, OP_REGISTER, OP_INVALID);
		case K_SYSCALL:
		case K_RET:
			return parse_no_operands(parser, keyword);
		case K_IDENT:
			parser->pos++;
			break;
		default:
			printf("Invalid opcode \"%s\" on line %d\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->line_num);
			exit(1);
	}
}

void parse(parser_t *parser) {
    size_t max_instruction_count = 128;
	parser->pos = 0;
	parser->instructions = (instruction_t **)malloc(max_instruction_count * sizeof(instruction_t *));

	//instruction_t current_instruction;

	//get after "main:"
	while(strcmp(parser->lexer->tokens[parser->pos]->ident_value, "main") != 0 || parser->lexer->tokens[parser->pos + 1]->token != T_COLON) {
		parser->pos++;
	}

	parser->pos++; //past main
	parser->pos++; //past :
	parser->pos++; //past \n

	//this begins the instruction part
	while(parser->lexer->tokens[parser->pos]->token != T_EOF) {
		//Handle comments
		if(parser->lexer->tokens[parser->pos]->token == T_SEMICOLON) {
			while(parser->lexer->tokens[parser->pos]->token != T_NEWLINE) parser->pos++;
		}
		if(parser->instruction_index > max_instruction_count) {
			max_instruction_count *= 2;
			parser->instructions = (instruction_t **)realloc(parser->instructions, max_instruction_count * sizeof(instruction_t *));
		}
		//memset(&current_instruction, 0, sizeof(current_instruction));
		size_t line_num = parser->lexer->tokens[parser->pos]->line_num;
		instruction_t current_instruction = parse_line(parser);

		if(current_instruction.opcode == -1) {
			parser->pos++;
			continue;
		}

		instruction_t *permenant_instruction = (instruction_t *)malloc(sizeof(instruction_t));
		permenant_instruction->opcode = current_instruction.opcode;
		permenant_instruction->operand_1 = current_instruction.operand_1;
		permenant_instruction->operand_2 = current_instruction.operand_2;

		if(current_instruction.operand_1.size == 0) set_operand_size(&permenant_instruction->operand_1);
		if(current_instruction.operand_2.size == 0) set_operand_size(&permenant_instruction->operand_2);
		permenant_instruction->line_num = line_num;

		//printf("%d - %d - %d\n", current_instruction.opcode, current_instruction.operand_1.reg, current_instruction.operand_2.reg);

		parser->instructions[parser->instruction_index++] = permenant_instruction;

		if(parser->lexer->tokens[parser->pos]->token == T_EOF) continue;

		require_newline(parser);
	}
}