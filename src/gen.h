#pragma once

#include "parser.h"
#include "symbol.h"

#include <stdint.h>
#include <stdio.h>

#define OP_REG8 0x01
#define OP_REG16 0x02
#define OP_REG32 0x04
#define OP_REG64 0x08
#define OP_IMM8 0x10
#define OP_IMM16 0x20
#define OP_IMM32 0x40
#define OP_IMM64 0x80
#define OP_MEM 0x100

#define F_NONE 0x0
#define F_MODRM 0x01
#define F_ADD_REG 0x02 //adds the register value to the opcode itself
#define F_SIB 0x04
#define F_REG_OPCODE 0x08 //modrm reg byte opcode modifier
#define F_TWO_BYTE 0x10   //adds 0x0F before opcode

//this stores all labels found in jmps that didn't already get set with an address.
typedef struct {
    uint64_t file_pos;
    symbol_entry_t *symbol;
} unresolved_label_t;

typedef struct {
    parser_t *parser;
    FILE *out;
    size_t instruction_pos; //which instruction we are on
    size_t byte_pos; //number of bytes written total

    size_t unresolved_label_size; //size of table
    int64_t unresolved_next_free;
    unresolved_label_t **unresolved_labels;
} code_gen_t;

typedef struct {
    uint8_t opcode;
    uint8_t flags;
} machine_instruction_t;

void generate_code(code_gen_t *code_gen);

static uint8_t syscall_bytes[] = { 0x0F, 0x05 };

//register size (1, 2, 4, 8 -> 0, 1, 2, 3)
static machine_instruction_t mov_reg_imm[] = {
    //r1                 //r2                 //r4                 //r8
    { 0xB0, F_ADD_REG }, { 0xB8, F_ADD_REG }, { 0xB8, F_ADD_REG }, { 0xC7, F_MODRM }
};

static machine_instruction_t mov_reg_reg[] = {
    //r1               //r2               //r4               //r8
    { 0x88, F_MODRM }, { 0x89, F_MODRM }, { 0x89, F_MODRM }, { 0x89, F_MODRM }
};

static machine_instruction_t mov_reg_mem[] = {
    //reg size
    { 0x8A, F_MODRM }, { 0x8B, F_MODRM }, { 0x8B, F_MODRM }, { 0x8B, F_MODRM }
};

//imm size (1, 2, 4, 8 -> 0, 1, 2, 3) (byte word dword qword)
static machine_instruction_t mov_mem_imm[] = {
    //imm1             //imm2             //imm4             //imm8
    { 0xC6, F_MODRM }, { 0xC7, F_MODRM }, { 0xC7, F_MODRM }, { 0xC7, F_MODRM }
};

//technically AL could use 04 and remove a byte... but whatever
//generic arithmatic opcode
static machine_instruction_t arith_reg_imm[] = {
    { 0x80, F_MODRM | F_REG_OPCODE }, { 0x83, F_MODRM | F_REG_OPCODE }, { 0x83, F_MODRM | F_REG_OPCODE }, { 0x83, F_MODRM | F_REG_OPCODE }, //reg 1 2 4 8 + imm8
    { 0x80, F_MODRM | F_REG_OPCODE }, { 0x81, F_MODRM | F_REG_OPCODE }, { 0x81, F_MODRM | F_REG_OPCODE }, { 0x81, F_MODRM | F_REG_OPCODE } //+ imm16/32 (first is invalid -- never used)
};

//also mem_reg
static machine_instruction_t arith_reg_reg[] = {
    { 0x00, F_MODRM }, { 0x01, F_MODRM }, { 0x01, F_MODRM }, { 0x01, F_MODRM }
};

static machine_instruction_t arith_reg_mem[] = {
    { 0x02, F_MODRM }, { 0x03, F_MODRM }, { 0x03, F_MODRM }, { 0x03, F_MODRM }
};

static uint8_t ret_opcode = 0xC3;

static machine_instruction_t arith_2_8 = { 0xF6, F_MODRM | F_REG_OPCODE };
static machine_instruction_t arith_2_16 = { 0xF7, F_MODRM | F_REG_OPCODE };

static machine_instruction_t push_reg = { 0x50, F_ADD_REG };
static machine_instruction_t pop_reg = { 0x58, F_ADD_REG };

static machine_instruction_t lea_mem = { 0x8D, F_MODRM };

static machine_instruction_t jmp_opcode = { 0xE9, F_NONE };
static machine_instruction_t call_opcode = { 0xE8, F_NONE };

//2-byte opcodes:
static uint8_t two_byte_opcode = 0x0F;
static machine_instruction_t set_reg = { 0x90, F_MODRM | F_TWO_BYTE };
static machine_instruction_t jmp_compare = { 0x80, F_TWO_BYTE };
static machine_instruction_t movzx_8 = { 0xB6, F_MODRM | F_TWO_BYTE };
static machine_instruction_t movzx_16 = { 0xB7, F_MODRM | F_TWO_BYTE };