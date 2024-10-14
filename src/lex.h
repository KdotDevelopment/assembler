#pragma once

#include <stdio.h>
#include <stdint.h>

#define MAX_IDENT_LENGTH 32

typedef struct token_t {
	int token;
	int int_value;
	int keyword;
	char ident_value[MAX_IDENT_LENGTH];
	int line_num;
} token_t;

typedef struct lexer_t {
	FILE *in_file;
	FILE *out_file;
	size_t line_num;
	int8_t character;
	size_t token_count;
	size_t token_index;
	token_t **tokens;
} lexer_t;

//Characters or Combinations
enum {
	T_MINUS,
    T_PERIOD,
	T_LBRACKET,
    T_RBRACKET,
    T_COMMA,
    T_COLON,
    T_SEMICOLON,
	T_INTLIT,
    T_IDENT,
    T_NEWLINE,
	T_EOF
};

//Keywords
enum {
	K_IDENT, //This is for user-made variable names

    //Register names (1-32)
    K_RAX, K_EAX, K_AX, K_AL,
    K_RCX, K_ECX, K_CX, K_CL,
    K_RDX, K_EDX, K_DX, K_DL,
    K_RBX, K_EBX, K_BX, K_BL,
    K_RSI, K_ESI, K_SI, K_SIL,
    K_RDI, K_EDI, K_DI, K_DIL,
    K_RSP, K_ESP, K_SP, K_SPL,
    K_RBP, K_EBP, K_BP, K_BPL,

    //Extended Registers (33 - 65)
	K_R8,  K_R8D,  K_R8W,  K_R8B,
    K_R9,  K_R9D,  K_R9W,  K_R9B,
    K_R10, K_R10D, K_R10W, K_R10B,
    K_R11, K_R11D, K_R11W, K_R11B,
    K_R12, K_R12D, K_R12W, K_R12B,
    K_R13, K_R13D, K_R13W, K_R13B,
    K_R14, K_R14D, K_R14W, K_R14B,
    K_R15, K_R15D, K_R15W, K_R15B,

    //Opcodes
    K_MOV, //24
    K_ADD,
    K_SUB,
    K_MUL,
    K_DIV,
    K_CMP,
    K_SETZ,
    K_MOVZX,
    K_SETNE,
    K_SETL,
    K_SETLE,
    K_SETG,
    K_SETGE,
    K_CALL,
    K_PUSH,
    K_POP,
    K_RET,
    K_SYSCALL,

    //Types
    K_BYTE,
    K_WORD,
    K_DWORD,
    K_QWORD
};

void lex(lexer_t *lexer);
void clean_tokens(lexer_t *lexer);