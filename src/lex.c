#include "lex.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int chr_pos(char *s, int c) {
	char *p;

	p = strchr(s, c);
	return (p ? p - s : -1);
}

int get_next_char(lexer_t *lexer) {
	int c;
	
	c = fgetc(lexer->in_file);
	//if(c == '\n') lexer->line_num++;
	return c;
}

int skip_char(lexer_t *lexer) {
	int c;

	c = get_next_char(lexer);
	while(c == ' ' || c == '\t' || c == '\r' || c == '\f') c = get_next_char(lexer); // skips over stuff that doesn't matter
	//ungetc(c, lexer->in_file);

	return c;
}

int64_t scan_int(lexer_t *lexer) {
	int k, val = 0;

	while((k = chr_pos("0123456789", lexer->character)) >= 0) {
		val = val * 10 + k;
		lexer->character = get_next_char(lexer);
	}

	ungetc(lexer->character, lexer->in_file);
	return val;
}

int scan_ident(lexer_t *lexer, token_t *token) {
	size_t index = 1;

	token->ident_value[0] = lexer->character; //we already know the first one is an ident

	while(isalpha(lexer->character) || isdigit(lexer->character) || lexer->character == '_') {
		if(index > MAX_IDENT_LENGTH - 1) {
			token->ident_value[index] = 0;
			printf("Indentifier %s on line %ld is over %d characters\n", token->ident_value, lexer->line_num, MAX_IDENT_LENGTH);
			exit(1);
		}
		//printf("%c", lexer->character);
		lexer->character = get_next_char(lexer);
		token->ident_value[index] = (char)lexer->character;
		index++;
	}
	token->ident_value[index - 1] = 0;

	ungetc(lexer->character, lexer->in_file);
	return 1;
}

void get_keyword(token_t *token) { //takes the ident string and converts it to a keyword if applicable
	if(token->token != T_IDENT) return;
	else if(!strcmp(token->ident_value, "rax")) token->keyword = K_RAX;
	else if(!strcmp(token->ident_value, "eax")) token->keyword = K_EAX;
	else if(!strcmp(token->ident_value, "ax")) token->keyword = K_AX;
	else if(!strcmp(token->ident_value, "ah")) token->keyword = K_AH;
	else if(!strcmp(token->ident_value, "al")) token->keyword = K_AL;

	else if(!strcmp(token->ident_value, "rcx")) token->keyword = K_RCX;
	else if(!strcmp(token->ident_value, "ecx")) token->keyword = K_ECX;
	else if(!strcmp(token->ident_value, "cx")) token->keyword = K_CX;
	else if(!strcmp(token->ident_value, "ch")) token->keyword = K_CH;
	else if(!strcmp(token->ident_value, "cl")) token->keyword = K_CL;

	else if(!strcmp(token->ident_value, "rdx")) token->keyword = K_RDX;
	else if(!strcmp(token->ident_value, "edx")) token->keyword = K_EDX;
	else if(!strcmp(token->ident_value, "dx")) token->keyword = K_DX;
	else if(!strcmp(token->ident_value, "dh")) token->keyword = K_DH;
	else if(!strcmp(token->ident_value, "dl")) token->keyword = K_DL;

	else if(!strcmp(token->ident_value, "rbx")) token->keyword = K_RBX;
	else if(!strcmp(token->ident_value, "ebx")) token->keyword = K_EBX;
	else if(!strcmp(token->ident_value, "bx")) token->keyword = K_BX;
	else if(!strcmp(token->ident_value, "bh")) token->keyword = K_BH;
	else if(!strcmp(token->ident_value, "bl")) token->keyword = K_BL;

	else if(!strcmp(token->ident_value, "rsp")) token->keyword = K_RSP;
	else if(!strcmp(token->ident_value, "esp")) token->keyword = K_ESP;
	else if(!strcmp(token->ident_value, "sp")) token->keyword = K_SP;
	else if(!strcmp(token->ident_value, "spl")) token->keyword = K_SPL;

    else if(!strcmp(token->ident_value, "rbp")) token->keyword = K_RBP;
	else if(!strcmp(token->ident_value, "ebp")) token->keyword = K_EBP;
	else if(!strcmp(token->ident_value, "bp")) token->keyword = K_BP;
	else if(!strcmp(token->ident_value, "bpl")) token->keyword = K_BPL;

	else if(!strcmp(token->ident_value, "rsi")) token->keyword = K_RSI;
	else if(!strcmp(token->ident_value, "esi")) token->keyword = K_ESI;
	else if(!strcmp(token->ident_value, "si")) token->keyword = K_SI;
	else if(!strcmp(token->ident_value, "sil")) token->keyword = K_SIL;

	else if(!strcmp(token->ident_value, "rdi")) token->keyword = K_RDI;
	else if(!strcmp(token->ident_value, "edi")) token->keyword = K_EDI;
	else if(!strcmp(token->ident_value, "di")) token->keyword = K_DI;
	else if(!strcmp(token->ident_value, "dil")) token->keyword = K_DIL;

	else if(!strcmp(token->ident_value, "r8")) token->keyword = K_R8;
	else if(!strcmp(token->ident_value, "r8d")) token->keyword = K_R8D;
	else if(!strcmp(token->ident_value, "r8w")) token->keyword = K_R8W;
	else if(!strcmp(token->ident_value, "r8b")) token->keyword = K_R8B;

	else if(!strcmp(token->ident_value, "r9")) token->keyword = K_R9;
	else if(!strcmp(token->ident_value, "r9d")) token->keyword = K_R9D;
	else if(!strcmp(token->ident_value, "r9w")) token->keyword = K_R9W;
	else if(!strcmp(token->ident_value, "r9b")) token->keyword = K_R9B;

	else if(!strcmp(token->ident_value, "r10")) token->keyword = K_R10;
	else if(!strcmp(token->ident_value, "r10d")) token->keyword = K_R10D;
	else if(!strcmp(token->ident_value, "r10w")) token->keyword = K_R10W;
	else if(!strcmp(token->ident_value, "r10b")) token->keyword = K_R10B;

    else if(!strcmp(token->ident_value, "r11")) token->keyword = K_R11;
	else if(!strcmp(token->ident_value, "r11w")) token->keyword = K_R11W;
    else if(!strcmp(token->ident_value, "r11d")) token->keyword = K_R11D;
    else if(!strcmp(token->ident_value, "r11b")) token->keyword = K_R11B;

	else if(!strcmp(token->ident_value, "r12")) token->keyword = K_R12;
	else if(!strcmp(token->ident_value, "r12w")) token->keyword = K_R12W;
    else if(!strcmp(token->ident_value, "r12d")) token->keyword = K_R12D;
    else if(!strcmp(token->ident_value, "r12b")) token->keyword = K_R12B;

	else if(!strcmp(token->ident_value, "r13")) token->keyword = K_R13;
	else if(!strcmp(token->ident_value, "r13w")) token->keyword = K_R13W;
    else if(!strcmp(token->ident_value, "r13d")) token->keyword = K_R13D;
    else if(!strcmp(token->ident_value, "r13b")) token->keyword = K_R13B;

	else if(!strcmp(token->ident_value, "r14")) token->keyword = K_R14;
	else if(!strcmp(token->ident_value, "r14w")) token->keyword = K_R14W;
    else if(!strcmp(token->ident_value, "r14d")) token->keyword = K_R14D;
    else if(!strcmp(token->ident_value, "r14b")) token->keyword = K_R14B;

	else if(!strcmp(token->ident_value, "r15")) token->keyword = K_R15;
	else if(!strcmp(token->ident_value, "r15w")) token->keyword = K_R15W;
    else if(!strcmp(token->ident_value, "r15d")) token->keyword = K_R15D;
    else if(!strcmp(token->ident_value, "r15b")) token->keyword = K_R15B;
    
    else if(!strcmp(token->ident_value, "mov")) token->keyword = K_MOV;

    else if(!strcmp(token->ident_value, "add")) token->keyword = K_ADD;
	else if(!strcmp(token->ident_value, "or")) token->keyword = K_OR;
	else if(!strcmp(token->ident_value, "adc")) token->keyword = K_ADC;
	else if(!strcmp(token->ident_value, "sbb")) token->keyword = K_SBB;
	else if(!strcmp(token->ident_value, "and")) token->keyword = K_AND;
    else if(!strcmp(token->ident_value, "sub")) token->keyword = K_SUB;
	else if(!strcmp(token->ident_value, "xor")) token->keyword = K_XOR;
	else if(!strcmp(token->ident_value, "cmp")) token->keyword = K_CMP;

	else if(!strcmp(token->ident_value, "test")) token->keyword = K_TEST;
    else if(!strcmp(token->ident_value, "not")) token->keyword = K_NOT;
	else if(!strcmp(token->ident_value, "neg")) token->keyword = K_NEG;
	else if(!strcmp(token->ident_value, "mul")) token->keyword = K_MUL;
	else if(!strcmp(token->ident_value, "imul")) token->keyword = K_IMUL;
	else if(!strcmp(token->ident_value, "div")) token->keyword = K_DIV;
	else if(!strcmp(token->ident_value, "idiv")) token->keyword = K_IDIV;

	else if(!strcmp(token->ident_value, "seto")) token->keyword = K_SETO;
	else if(!strcmp(token->ident_value, "setno")) token->keyword = K_SETNO;
	else if(!strcmp(token->ident_value, "setb")) token->keyword = K_SETB;
	else if(!strcmp(token->ident_value, "setnae")) token->keyword = K_SETB;
	else if(!strcmp(token->ident_value, "setc")) token->keyword = K_SETB;
	else if(!strcmp(token->ident_value, "setnb")) token->keyword = K_SETNB;
	else if(!strcmp(token->ident_value, "setae")) token->keyword = K_SETNB;
	else if(!strcmp(token->ident_value, "setnc")) token->keyword = K_SETNB;
	else if(!strcmp(token->ident_value, "setz")) token->keyword = K_SETZ;
	else if(!strcmp(token->ident_value, "sete")) token->keyword = K_SETZ;
	else if(!strcmp(token->ident_value, "setnz")) token->keyword = K_SETNZ;
	else if(!strcmp(token->ident_value, "setne")) token->keyword = K_SETNZ;
	else if(!strcmp(token->ident_value, "setbe")) token->keyword = K_SETBE;
	else if(!strcmp(token->ident_value, "setna")) token->keyword = K_SETBE;
	else if(!strcmp(token->ident_value, "seta")) token->keyword = K_SETA;
	else if(!strcmp(token->ident_value, "setnbe")) token->keyword = K_SETA;
	else if(!strcmp(token->ident_value, "sets")) token->keyword = K_SETS;
	else if(!strcmp(token->ident_value, "setns")) token->keyword = K_SETNS;
	else if(!strcmp(token->ident_value, "setp")) token->keyword = K_SETP;
	else if(!strcmp(token->ident_value, "setpe")) token->keyword = K_SETP;
	else if(!strcmp(token->ident_value, "setnp")) token->keyword = K_SETNP;
	else if(!strcmp(token->ident_value, "setpo")) token->keyword = K_SETNP;
	else if(!strcmp(token->ident_value, "setl")) token->keyword = K_SETL;
	else if(!strcmp(token->ident_value, "setnge")) token->keyword = K_SETL;
	else if(!strcmp(token->ident_value, "setge")) token->keyword = K_SETGE;
	else if(!strcmp(token->ident_value, "setnl")) token->keyword = K_SETGE;
	else if(!strcmp(token->ident_value, "setle")) token->keyword = K_SETLE;
	else if(!strcmp(token->ident_value, "setng")) token->keyword = K_SETLE;
	else if(!strcmp(token->ident_value, "setg")) token->keyword = K_SETG;
	else if(!strcmp(token->ident_value, "setnle")) token->keyword = K_SETG;

	else if(!strcmp(token->ident_value, "jo")) token->keyword = K_JO;
	else if(!strcmp(token->ident_value, "jno")) token->keyword = K_JNO;
	else if(!strcmp(token->ident_value, "jb")) token->keyword = K_JB;
	else if(!strcmp(token->ident_value, "jnae")) token->keyword = K_JB;
	else if(!strcmp(token->ident_value, "jc")) token->keyword = K_JB;
	else if(!strcmp(token->ident_value, "jnb")) token->keyword = K_JNB;
	else if(!strcmp(token->ident_value, "jae")) token->keyword = K_JNB;
	else if(!strcmp(token->ident_value, "jnc")) token->keyword = K_JNB;
	else if(!strcmp(token->ident_value, "jz")) token->keyword = K_JZ;
	else if(!strcmp(token->ident_value, "je")) token->keyword = K_JZ;
	else if(!strcmp(token->ident_value, "jnz")) token->keyword = K_JNZ;
	else if(!strcmp(token->ident_value, "jne")) token->keyword = K_JNZ;
	else if(!strcmp(token->ident_value, "jbe")) token->keyword = K_JBE;
	else if(!strcmp(token->ident_value, "jna")) token->keyword = K_JBE;
	else if(!strcmp(token->ident_value, "ja")) token->keyword = K_JA;
	else if(!strcmp(token->ident_value, "jnbe")) token->keyword = K_JA;
	else if(!strcmp(token->ident_value, "js")) token->keyword = K_JS;
	else if(!strcmp(token->ident_value, "jns")) token->keyword = K_JNS;
	else if(!strcmp(token->ident_value, "jp")) token->keyword = K_JP;
	else if(!strcmp(token->ident_value, "jpe")) token->keyword = K_JP;
	else if(!strcmp(token->ident_value, "jnp")) token->keyword = K_JNP;
	else if(!strcmp(token->ident_value, "jpo")) token->keyword = K_JNP;
	else if(!strcmp(token->ident_value, "jl")) token->keyword = K_JL;
	else if(!strcmp(token->ident_value, "jnge")) token->keyword = K_JL;
	else if(!strcmp(token->ident_value, "jge")) token->keyword = K_JGE;
	else if(!strcmp(token->ident_value, "jnl")) token->keyword = K_JGE;
	else if(!strcmp(token->ident_value, "jle")) token->keyword = K_JLE;
	else if(!strcmp(token->ident_value, "jng")) token->keyword = K_JLE;
	else if(!strcmp(token->ident_value, "jg")) token->keyword = K_JG;
	else if(!strcmp(token->ident_value, "jnle")) token->keyword = K_JG;

    else if(!strcmp(token->ident_value, "movzx")) token->keyword = K_MOVZX;
    else if(!strcmp(token->ident_value, "call")) token->keyword = K_CALL;
    else if(!strcmp(token->ident_value, "push")) token->keyword = K_PUSH;
    else if(!strcmp(token->ident_value, "pop")) token->keyword = K_POP;
    else if(!strcmp(token->ident_value, "ret")) token->keyword = K_RET;
    else if(!strcmp(token->ident_value, "syscall")) token->keyword = K_SYSCALL;
    else if(!strcmp(token->ident_value, "BYTE")) token->keyword = K_BYTE;
    else if(!strcmp(token->ident_value, "WORD")) token->keyword = K_WORD;
    else if(!strcmp(token->ident_value, "DWORD")) token->keyword = K_DWORD;
    else if(!strcmp(token->ident_value, "QWORD")) token->keyword = K_QWORD;

	else token->keyword = K_IDENT; //Must be a user defined variable name
}

token_t scan(lexer_t *lexer) {
	lexer->character = skip_char(lexer); //this will also go to the next character automagically
	token_t token;
	memset(token.ident_value, 0, MAX_IDENT_LENGTH);
	token.int_value = -1;
	char next_char;
	switch(lexer->character) {
		case -1:
			token.token = T_EOF;
			break;
        case '\n':
            lexer->line_num++;
            token.token = T_NEWLINE;
            break;
		case '-':
			token.token = T_MINUS;
			break;
        case '.':
			token.token = T_PERIOD;
			break;
        case '[':
            token.token = T_LBRACKET;
            break;
        case ']':
            token.token = T_RBRACKET;
            break;
		case ',':
			token.token = T_COMMA;
			break;
        case ':':
			token.token = T_COLON;
			break;
		case ';':
			token.token = T_SEMICOLON;
			break;
		default:
			if(isdigit(lexer->character)) {
				token.int_value = scan_int(lexer);
				token.token = T_INTLIT;
				break;
			}
			if(isalpha(lexer->character) || lexer->character == '_') {
				scan_ident(lexer, &token);
				token.token = T_IDENT;
				break;
			}

			printf("Unrecognized character \'%c\' on line %ld\n", lexer->character, lexer->line_num);
			exit(1);
	}
	token.keyword = -1;
	get_keyword(&token);
	token.line_num = lexer->line_num;
	return token; //successful
}

void clean_tokens(lexer_t *lexer) {
	for(int i = 0; i < lexer->token_count; i++) {
		free(lexer->tokens[i]);
	}
}

void lex(lexer_t *lexer) {
    size_t max_token_count = 128;
	lexer->line_num = 1;
	lexer->tokens = (token_t **)malloc(max_token_count * sizeof(token_t *));

	token_t current_token;
	lexer->token_index = 0;
	while(current_token.token != T_EOF) {
		if(lexer->token_index > max_token_count) {
			max_token_count *= 2;
			lexer->tokens = (token_t **)realloc(lexer->tokens, max_token_count * sizeof(token_t *));
		}
		lexer->token_count = max_token_count;

		token_t *permenant_token = malloc(sizeof(token_t));

		current_token = scan(lexer);

		//skip comments
		if(current_token.token == T_SEMICOLON) {
			while(lexer->character != '\n') {
				lexer->character = skip_char(lexer);
				//current_token = scan(lexer);
			}
			ungetc('\n', lexer->in_file);
			lexer->line_num++;
			continue;
		}

		strcpy(permenant_token->ident_value, current_token.ident_value);
		permenant_token->int_value = current_token.int_value;
		permenant_token->keyword = current_token.keyword;
		permenant_token->token = current_token.token;
		permenant_token->line_num = current_token.line_num;

		lexer->tokens[lexer->token_index++] = permenant_token;
		//printf("%d - %d . %d - %s :: %d\n", current_token.token, current_token.keyword, current_token.int_value, current_token.ident_value, current_token.line_num);
	}
}