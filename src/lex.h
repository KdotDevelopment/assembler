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

    //64-bit registers (1 - 16)
    K_RAX, K_RCX, K_RDX, K_RBX,
    K_RSP, K_RBP, K_RSI, K_RDI,
    K_R8,  K_R9,  K_R10, K_R11,
    K_R12, K_R13, K_R14, K_R15,

    //32-bit registers (17 - 32)
    K_EAX,  K_ECX,  K_EDX,  K_EBX,
    K_ESP,  K_EBP,  K_ESI,  K_EDI,
    K_R8D,  K_R9D,  K_R10D, K_R11D,
    K_R12D, K_R13D, K_R14D, K_R15D,

    //16-bit registers (33 - 48)
	K_AX,   K_CX,   K_DX,   K_BX,
    K_SP,   K_BP,   K_SI,   K_DI,
    K_R8W,  K_R9W,  K_R10W, K_R11W,
    K_R12W, K_R13W, K_R14W, K_R15W,

    //8-bit registers (49 - 68)
    K_AL,   K_CL,   K_DL,   K_BL,
    K_AH,   K_CH,   K_DH,   K_BH,
    K_SPL,  K_BPL,  K_SIL,  K_DIL,
    K_R8B,  K_R9B,  K_R10B, K_R11B,
    K_R12B, K_R13B, K_R14B, K_R15B,
    
    //Opcodes
    K_MOV, //69

    //Shared Opcodes
    K_ADD,
    K_OR,
    K_ADC,
    K_SBB,
    K_AND,
    K_SUB,
    K_XOR,
    K_CMP,

    K_MUL,
    K_DIV,
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