#pragma once

#include "parser.h"

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
#define F_ADD_REG 0x02
#define F_SIB 0x08
#define F_REG_OPCODE 0x10 //modrm reg byte opcode modifier

typedef struct {
    parser_t *parser;
    FILE *out;
    size_t instruction_pos; //which instruction we are on
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

static machine_instruction_t mov_mem_reg[] = {
    //r1                       //r2                       //r4                       //r8
    { 0x88, F_MODRM | F_SIB }, { 0x89, F_MODRM | F_SIB }, { 0x89, F_MODRM | F_SIB }, { 0x89, F_MODRM | F_SIB }
};

static machine_instruction_t mov_reg_mem[] = {
    //reg size
    { 0x8A, F_MODRM | F_SIB }, { 0x8B, F_MODRM | F_SIB }, { 0x8B, F_MODRM | F_SIB }, { 0x8B, F_MODRM | F_SIB }
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

static machine_instruction_t arith_mem_imm[] = {
    { 0x80, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x83, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x83, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x83, F_MODRM | F_REG_OPCODE | F_SIB }, //reg 1 2 4 8 + imm8
    { 0x80, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x81, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x81, F_MODRM | F_REG_OPCODE | F_SIB }, { 0x81, F_MODRM | F_REG_OPCODE | F_SIB } //+ imm16/32 (first is invalid -- never used)
};

static machine_instruction_t arith_reg_reg[] = {
    { 0x00, F_MODRM }, { 0x01, F_MODRM }, { 0x01, F_MODRM }, { 0x01, F_MODRM }
};

static machine_instruction_t arith_mem_reg[] = {
    { 0x00, F_MODRM | F_SIB }, { 0x01, F_MODRM | F_SIB }, { 0x01, F_MODRM | F_SIB }, { 0x01, F_MODRM | F_SIB }
};

static machine_instruction_t arith_reg_mem[] = {
    { 0x02, F_MODRM | F_SIB }, { 0x03, F_MODRM | F_SIB }, { 0x03, F_MODRM | F_SIB }, { 0x03, F_MODRM | F_SIB }
};