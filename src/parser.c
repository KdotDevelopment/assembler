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
	memset(&operand, 0, sizeof(operand));
	token_t *current_token = parser->lexer->tokens[parser->pos];
	int token_index = parser->pos; // we cannot use the real one because it needs to revert back if unsuccessful.

	operand.mem_scale = 1; //default, obviously zero isn't right

	//scale
	if(current_token->token == T_MINUS) {
		token_index++;
		current_token = parser->lexer->tokens[token_index];
		if(current_token->token == T_INTLIT) {
			operand.mem_displacement = current_token->int_value * -1;
			token_index++; //hopefully to '[' of register
			current_token = parser->lexer->tokens[token_index];
		}else {
			printf("Unexpected '-' in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
			exit(1);
		}
	}else if(current_token->token == T_INTLIT) {
		operand.mem_displacement = current_token->int_value;
		token_index++; //hopefully to '[' of register
		current_token = parser->lexer->tokens[token_index];
	}else if(current_token->keyword >= K_RAX && current_token->keyword <= K_R15D) { //non-negative displacement may be a register too
		operand.index_reg = current_token->keyword;
		token_index++; //hopefully to '[' of register
		current_token = parser->lexer->tokens[token_index];
		//register but then no [ means this isn't a memory thing
		if(current_token->token != T_LBRACKET) {
			operand.flags = OP_INVALID;
			return operand;
		}
	}

	if(current_token->token != T_LBRACKET) {
		operand.flags = OP_INVALID;
		return operand;
	}

	token_index++; // '[' to reg name
	current_token = parser->lexer->tokens[token_index];

	//is a register 32 or 64bit
	if(current_token->keyword >= K_RAX && current_token->keyword <= K_R15D) {
		operand.reg = current_token->keyword;
		if(get_register_size(operand.index_reg) != get_register_size(operand.reg) && operand.index_reg != 0) { //base and index must be same size to add
			printf("%d, %d\n", get_register_size(operand.index_reg), get_register_size(operand.reg));
			printf("Index register and base register must be of same size on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
			exit(1);
		}
	}else {
		printf("Expected 32/64-bit register within brackets in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
		exit(1);
	}

	token_index++; //reg name to ']' or '*'
	current_token = parser->lexer->tokens[token_index];

	//scale value or end disp[reg * scale] or disp[reg]
	if(current_token->token == T_STAR) {
		token_index++; //to hopefully intlit
		current_token = parser->lexer->tokens[token_index];
		if(current_token->token != T_INTLIT) {
			printf("Expected integer after multiplication operator in memory operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
			exit(1);
		}
		if(current_token->int_value != 1 && current_token->int_value != 2 && current_token->int_value != 4 && current_token->int_value != 8) {
			printf("Scale must be 1, 2, 4 or 8 in operand on line %d\n", parser->lexer->tokens[token_index - 1]->line_num);
			exit(1);
		}
		operand.mem_scale = current_token->int_value;
		token_index++; //to hopefully ]
		current_token = parser->lexer->tokens[token_index];
	}
	
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

void require_colon(parser_t *parser) {
	if(parser->lexer->tokens[parser->pos]->token != T_COLON) {
		printf("Excepted colon after identifier on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}
	parser->pos++;
}

//Flags: OP_REGISTER, OP_INTLIT, OP_MEMORY
//Use like OP_REGISTER | OP_INTLIT to accept both register and integer literal
operand_t parse_operand(parser_t *parser, uint16_t flags) {
	operand_t operand;
	memset(&operand, 0, sizeof(operand_t));

	//check for byte word dword qword and set size (otherwise size is set automatically)
	uint8_t force_size = parse_size_modifier(parser);

	token_t *token = parser->lexer->tokens[parser->pos];

	uint16_t flag_register = flags & OP_REGISTER;
	uint16_t flag_intlit = (flags & OP_INTLIT) >> 1;
	uint16_t flag_memory = (flags & OP_MEMORY) >> 2;
	uint16_t flag_label = (flags & OP_LABEL) >> 3;

	if((operand = memory_operand(parser)).flags != OP_INVALID) {
		if(!flag_memory) {
			printf("Invalid memory operand and opcode combination on line %d\n", token->line_num);
			exit(1);
		}
		operand.size = 4; //address is 4 bytes
	}else if(token->keyword == K_IDENT) {
		if(!flag_label) {
			printf("Invalid label operand and opcode combination on line %d\n", token->line_num);
			exit(1);
		}
		memset(&operand, 0, sizeof(operand));
		operand.flags = OP_LABEL;
		operand.label_name = token->ident_value;
		operand.size = force_size;
	}else if(token->token == T_SINGLE_QUOTE) { //character value
		if(!flag_intlit) {
			printf("Invalid imm (character) operand %d and opcode combination on line %d\n", token->int_value, token->line_num);
			exit(1);
		}
		parser->pos++;
		token_t *token = parser->lexer->tokens[parser->pos];
		if(token->keyword != K_IDENT) {
			printf("Expected character after single quote on line %d\n", token->line_num);
			exit(1);
		}
		if(strlen(token->ident_value) > 1) {
			printf("Expected only one character after single quote on line %d\n", token->line_num);
			exit(1);
		}
		memset(&operand, 0, sizeof(operand));
		operand.intlit = (uint8_t)token->ident_value[0]; //convert first character to uint8 (ascii number)
		operand.flags = OP_INTLIT;
		operand.size = force_size;
		parser->pos++;
		if(parser->lexer->tokens[parser->pos]->token != T_SINGLE_QUOTE) {
			printf("Excepted single quote after character on line %d\n", parser->lexer->tokens[parser->pos]->line_num);
			exit(1);
		}
	}else if(token->keyword >= K_RAX && token->keyword <= K_R15B) {
		if(!flag_register) {
			printf("Invalid register operand \"%s\" and opcode combination on line %d\n", token->ident_value, token->line_num);
			exit(1);
		}
		memset(&operand, 0, sizeof(operand));
		operand.reg = token->keyword;
		operand.flags = OP_REGISTER;
		operand.size = force_size;
	}else if(token->token == T_INTLIT) {
		if(!flag_intlit) {
			printf("Invalid imm operand %d and opcode combination on line %d\n", token->int_value, token->line_num);
			exit(1);
		}
		memset(&operand, 0, sizeof(operand));
		operand.intlit = token->int_value;
		operand.flags = OP_INTLIT;
		operand.size = force_size;
	}else {
		printf("Invalid operand \"%s\" and opcode combination on line %d\n", token->ident_value, token->line_num);
		exit(1);
	}
	return operand;
}

instruction_t parse_no_operands(parser_t *parser, int opcode) {
	instruction_t instr;

	instr.opcode = opcode;
	instr.operand_1.reg = 0;
	instr.operand_2.reg = 0;

	parser->pos++;

	return instr;
}

void is_operand_valid(parser_t *parser, operand_t *operand, uint16_t flags) {
	if(operand->size == 0) set_operand_size(operand);
	uint8_t size = operand->size;
	uint8_t must_check = flags & 0b11110000; //filter only size flags -- if present we must check all of them. if not, do not check size at all

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

instruction_t parse_instruction(parser_t *parser, int opcode, uint16_t operand_1_flags, uint16_t operand_2_flags) {
	instruction_t instr;

	instr.opcode = opcode;
	parser->pos++; //past opcode
	instr.operand_1 = parse_operand(parser, operand_1_flags & 0b00001111); //last three bits are reg, mem, imm, label, rest are amount of bits

	is_operand_valid(parser, &instr.operand_1, operand_1_flags);

	parser->pos++;

	if(operand_2_flags & OP_INVALID) {
		operand_t invalid_op;
		memset(&invalid_op, 0, sizeof(operand_t));
		invalid_op.flags = OP_INVALID;
		instr.operand_2 = invalid_op;
		return instr;
	}

	require_comma(parser);
	instr.operand_2 = parse_operand(parser, operand_2_flags & 0b00001111); //last four bits are reg, mem, imm, label, rest are amount of bits

	is_operand_valid(parser, &instr.operand_2, operand_2_flags);

	parser->pos++;

	return instr;
}

//we are going to create the symbol now as a first pass
instruction_t parse_identifier(parser_t *parser) {
	instruction_t instr;

	instr.opcode = -2;

	if(find_symbol(parser->lexer->tokens[parser->pos]->ident_value, parser->symbol_table) != -1) {
		printf("Label name \"%s\" on line %d already exists\n", parser->lexer->tokens[parser->pos]->ident_value, parser->lexer->tokens[parser->pos]->line_num);
		exit(1);
	}

	create_symbol(parser->lexer->tokens[parser->pos]->ident_value, -1, parser->symbol_table);

	operand_t operand;
	operand.label_name = parser->lexer->tokens[parser->pos]->ident_value;
	operand.flags = OP_LABEL;
	instr.operand_1 = operand;

	parser->pos++; //past the identifier
	require_colon(parser);

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
		case K_LEA:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_16BIT | OP_32BIT | OP_64BIT, OP_MEMORY);
		case K_MOV:
		case K_ADD:
		case K_OR:
		case K_ADC:
		case K_SBB:
		case K_AND:
		case K_SUB:
		case K_XOR:
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
		case K_JMP:
		case K_JO:
		case K_JNO:
		case K_JB:
		case K_JNB:
		case K_JZ:
		case K_JNZ:
		case K_JBE:
		case K_JA:
		case K_JS:
		case K_JNS:
		case K_JP:
		case K_JNP:
		case K_JL:
		case K_JGE:
		case K_JLE:
		case K_JG:
		case K_CALL:
			return parse_instruction(parser, keyword, OP_LABEL, OP_INVALID);
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
		case K_PUSH:
		case K_POP:
			return parse_instruction(parser, keyword, OP_REGISTER | OP_16BIT | OP_32BIT | OP_64BIT, OP_INVALID);
		case K_SYSCALL:
		case K_RET:
			return parse_no_operands(parser, keyword);
		case K_IDENT:
			return parse_identifier(parser);
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
	memset(parser->instructions, 0, max_instruction_count * sizeof(instruction_t *));

	//instruction_t current_instruction;

	//get to the first label
	while(strcmp(parser->lexer->tokens[parser->pos]->ident_value, "text") != 0) {
		parser->pos++;
	}

	//parser->pos++; //past main
	//parser->pos++; //past :
	parser->pos++; //past \n

	//this begins the instruction part
	while(parser->lexer->tokens[parser->pos]->token != T_EOF) {
		//Handle comments
		if(parser->lexer->tokens[parser->pos]->token == T_SEMICOLON) {
			while(parser->lexer->tokens[parser->pos]->token != T_NEWLINE) parser->pos++;
		}
		if(parser->instruction_index > max_instruction_count) {
			max_instruction_count += 256;
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