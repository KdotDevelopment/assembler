#include "gen.h"

#include <stdlib.h> 

//inputs keyword enum
uint8_t is_extended_register(int reg) {
    if(reg >= K_R8 && reg <= K_R15) return 1;
    if(reg >= K_R8D && reg <= K_R15D) return 1;
    if(reg >= K_R8W && reg <= K_R15W) return 1;
    if(reg >= K_SPL && reg <= K_R15B) return 1;
    return 0;
}

//inputs keyword enum
uint8_t get_register_number(int reg) {
    uint8_t size = get_register_size(reg);
    uint8_t is_extended = is_extended_register(reg);

    if(reg == K_SPL || reg == K_BPL || reg == K_SIL || reg == K_DIL) return reg - K_SPL + 4;
    if(size == 1 && !is_extended) return reg - K_AL;
    if(size == 1 && is_extended) return reg - K_R8B;
    if(size == 2 && !is_extended) return reg - K_AX;
    if(size == 2 && is_extended) return reg - K_R8W;
    if(size == 4 && !is_extended) return reg - K_EAX;
    if(size == 4 && is_extended) return reg - K_R8D;
    if(size == 8 && !is_extended) return reg - K_RAX;
    if(size == 8 && is_extended) return reg - K_R8;
}

//returns bytes
uint8_t get_int_size(int64_t number) {
	if(number < 128 && number >= -128) return 1;
	if(number < 32768 && number >= -32768) return 2;
	if(number < 2147483648 && number >= -2147483648) return 4;
	if(number < 9223372036854775807 && number >= -9223372036854775807) return 8;
}

//converts [1 2 4 8] to [0 1 2 3]
uint8_t get_size_index(uint8_t size) {
    switch(size) {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
    }
}

void gen_syscall(code_gen_t *code_gen) {
    fwrite(&syscall_bytes, sizeof(syscall_bytes), 1, code_gen->out);
}

//returns uint8 with mask 0b00111000
uint8_t gen_reg_opcode(instruction_t *instruction) {
    uint8_t reg = 0b00000000;

    if(instruction->opcode >= K_ADD && instruction->opcode <= K_CMP) {
        reg |= (instruction->opcode - K_ADD) << 3; //the keyword enums are in the right orders
    }

    if(instruction->opcode >= K_TEST && instruction->opcode <= K_IDIV) {
        reg |= ((instruction->opcode + 1) - K_TEST) << 3; //+1 because there are two seemingly identical test instructions...???
    }

    return reg;
}

void gen_prefix(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = instruction->operand_1.size;
    uint8_t rex = 0b01000000; //0b0100WRXB, W = is 64?, R = src reg is extended?, X = index reg used?, B = dest reg extended?
    
    //if(instruction->operand_1.flags & OP_REGISTER && instruction->operand_1.size == 1 && !is_extended_register(instruction->operand_1.reg)) return;

    //16 bit prefix
    if(instruction->operand_1.flags & OP_REGISTER && instruction->operand_1.size == 2) {
        rex = 0x66;
        fwrite(&rex, sizeof(rex), 1, code_gen->out);
        return;
    }

    //dest register
    if(is_extended_register(instruction->operand_1.reg) && instruction->operand_1.flags & OP_REGISTER) {
        if(!(instruction->operand_1.reg >= K_SPL && instruction->operand_1.reg <= K_DIL)) {
            rex |= 0b0001;
        }
    }

    //src register
    if(is_extended_register(instruction->operand_2.reg) && instruction->operand_2.flags & OP_REGISTER) {
        if(!(instruction->operand_2.reg >= K_SPL && instruction->operand_2.reg <= K_DIL)) {
            rex |= 0b0100;
        }
    }

    //dest reg 64?
    if(instruction->operand_1.size == 8 && instruction->operand_1.flags & OP_REGISTER) {
        rex |= 0b1000;
        //if(instruction->operand_2.size != 8 && instruction->operand_2.flags & OP_REGISTER) {
        //    printf("Cannot mix 64 bit and non-64 bit operands on line %ld\n", instruction->line_num);
        //    exit(1);
        //}
    }

    //src reg 64?
    if(instruction->operand_2.size == 8 && instruction->operand_2.flags & OP_REGISTER) {
        //if(!(rex & 0b1000) && instruction->operand_1.flags & OP_REGISTER) {
        //    printf("Cannot mix 64 bit and non-64 bit operands on line %ld\n", instruction->line_num);
        //    exit(1);
        //}
        rex |= 0b1000;
    }

    //X not implemented -- grammar for mov [rax + r8 * 4], rbx (for example) is not supported right now.

    fwrite(&rex, sizeof(rex), 1, code_gen->out);
    //printf("rex: %d\n", rex);
}

//writes Mod-reg-r/m byte    reg_opcode = if opcode can mean multiple things
void gen_modrm(instruction_t *instruction, uint8_t reg_opcode, code_gen_t *code_gen) {
    uint8_t modrm = 0b00000000; // [0 0] MOD, [0 0 0] Reg/Opcode, [0 0 0] R/M

    instruction_t current_instr = *instruction;

    if(current_instr.operand_2.flags & OP_MEMORY && current_instr.operand_2.mem_offset == 0) {
        modrm |= 0b00000000; //keep first two zero, basically
    }

    if(current_instr.operand_2.flags & OP_MEMORY && get_int_size(current_instr.operand_2.mem_offset) == 1) {
        modrm |= 0b01000000;
    }

    if(current_instr.operand_2.flags & OP_MEMORY && get_int_size(current_instr.operand_2.mem_offset) == 4) {
        modrm |= 0b10000000;
    }

    if(current_instr.operand_1.flags & OP_REGISTER) {
        modrm |= 0b11000000;
    }

    //src
    if(current_instr.operand_2.flags & OP_REGISTER && !reg_opcode) {
        modrm |= (get_register_number(current_instr.operand_2.reg) << 3);
    }

    //add/sub/etc
    if(reg_opcode) {
        modrm |= gen_reg_opcode(&current_instr);
    }

    //dest
    if(current_instr.operand_1.flags & OP_REGISTER) {
        modrm |= (get_register_number(current_instr.operand_1.reg));
    }

    fwrite(&modrm, sizeof(modrm), 1, code_gen->out);
    //printf("mrrm: %d\n", mrrm);
}

//NOT DONE
void gen_sib(instruction_t *instruction, code_gen_t *code_gen) {

}

void gen_from_instruction(machine_instruction_t *machine_code, instruction_t *instruction, code_gen_t *code_gen) {
    uint8_t opcode;
    opcode = machine_code->opcode;
    if(machine_code->flags & F_ADD_REG) opcode |= get_register_number(instruction->operand_1.reg);
    if(machine_code->flags & F_TWO_BYTE) fwrite(&two_byte_opcode, sizeof(two_byte_opcode), 1, code_gen->out);
    fwrite(&opcode, sizeof(opcode), 1, code_gen->out);
    if(machine_code->flags & F_SIB) gen_sib(instruction, code_gen);
    if(machine_code->flags & F_MODRM) gen_modrm(instruction, machine_code->flags & F_REG_OPCODE, code_gen);
}

//mov rax, 9 -> 48 c7 c0 09 00 00 00
//mov r8, 9  -> 49 c7 c0 09 00 00 00
//mov r8d, 9 ->    41 b8 09 00 00 00
//mov r8w, 9 -> 66 41 b8 09 00
//mov r8b, 9 ->    41 b0 09

void gen_mov(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);

    uint8_t opcode;
    uint8_t op_1_reg = instruction->operand_1.flags & OP_REGISTER;
    uint8_t op_1_mem = instruction->operand_1.flags & OP_MEMORY;
    uint8_t op_2_reg = instruction->operand_2.flags & OP_REGISTER;
    uint8_t op_2_mem = instruction->operand_2.flags & OP_MEMORY;
    uint8_t op_2_imm = instruction->operand_2.flags & OP_INTLIT;

    if(op_1_reg && op_2_reg) {
        machine_instruction_t instr = mov_reg_reg[get_size_index(instruction->operand_2.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_imm) {
        machine_instruction_t instr = mov_reg_imm[get_size_index(instruction->operand_1.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_reg) {
        machine_instruction_t instr = mov_mem_reg[get_size_index(instruction->operand_2.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_mem) {
        machine_instruction_t instr = mov_reg_mem[get_size_index(instruction->operand_1.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_imm) {
        machine_instruction_t instr = mov_mem_imm[get_size_index(instruction->operand_2.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_2_imm) {
        int64_t intlit = instruction->operand_2.intlit;
        switch(instruction->operand_1.size) {
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, 1, 1, code_gen->out);
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, 2, 1, code_gen->out);
                break;
            case 4:
            case 8: //just because its 8 doesnt mean we put all 8 bytes in -- max 4
                intlit = (int32_t)intlit;
                fwrite(&intlit, 4, 1, code_gen->out);
                break;
        }
    }
}

//add, or, adc, sbb, and, sub, xor, cmp
void gen_arith(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);

    uint8_t opcode;
    uint8_t op_1_reg = instruction->operand_1.flags & OP_REGISTER;
    uint8_t op_1_mem = instruction->operand_1.flags & OP_MEMORY;
    uint8_t op_2_reg = instruction->operand_2.flags & OP_REGISTER;
    uint8_t op_2_mem = instruction->operand_2.flags & OP_MEMORY;
    uint8_t op_2_imm = instruction->operand_2.flags & OP_INTLIT;

    if(op_1_reg && op_2_reg) {
        machine_instruction_t instr = arith_mem_reg[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_imm && instruction->operand_2.size == 1) {
        machine_instruction_t instr = arith_reg_imm[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    //16, 32, or 64 bit imm (64 just makes 32 anynway)
    if(op_1_reg && op_2_imm && instruction->operand_2.size > 1) {
        machine_instruction_t instr = arith_reg_imm[get_size_index(instruction->operand_2.size + 4)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_reg) {
        machine_instruction_t instr = arith_mem_reg[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_mem) {
        machine_instruction_t instr = arith_mem_reg[get_size_index(instruction->operand_1.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_imm) {
        machine_instruction_t instr = arith_mem_imm[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(instruction->operand_2.flags & OP_INTLIT) {
        int64_t intlit = instruction->operand_2.intlit;
        uint8_t int_size = get_int_size(intlit);
        switch(int_size) {
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, 1, 1, code_gen->out);
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, 2, 1, code_gen->out);
                break;
            case 4:
            case 8:
                intlit = (int32_t)intlit;
                fwrite(&intlit, 4, 1, code_gen->out);
                break;
        }
    }
}

//test, not, neg, mul, imul, div, idiv
void gen_arith_2(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);

    uint8_t opcode;
    uint8_t op_1_reg = instruction->operand_1.flags & OP_REGISTER;
    uint8_t op_1_mem = instruction->operand_1.flags & OP_MEMORY;
    uint8_t op_2_reg = instruction->operand_2.flags & OP_REGISTER;
    uint8_t op_2_mem = instruction->operand_2.flags & OP_MEMORY;
    uint8_t op_2_imm = instruction->operand_2.flags & OP_INTLIT;

    if(instruction->operand_1.size == 1) {
        machine_instruction_t instr = arith_2_8;
        if(op_1_mem) instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }
    
    if(instruction->operand_1.size > 1) {
        machine_instruction_t instr = arith_2_16;
        if(op_1_mem) instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    //only test accepts imm for operand 2
    if(instruction->operand_2.flags & OP_INTLIT && instruction->opcode == K_TEST) {
        int64_t intlit = instruction->operand_2.intlit;
        uint8_t int_size = get_int_size(intlit);
        switch(int_size) {
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, 1, 1, code_gen->out);
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, 2, 1, code_gen->out);
                break;
            case 4:
            case 8:
                intlit = (int32_t)intlit;
                fwrite(&intlit, 4, 1, code_gen->out);
                break;
        }
    }
}

void gen_set(instruction_t *instruction, code_gen_t *code_gen) {
    machine_instruction_t instr;

    instr = set_reg;
    instr.opcode += instruction->opcode - K_SETO; //adds appropriate val to 0F90, keyword enum is in right order
    if(instruction->operand_2.flags & OP_MEMORY) instr.flags |= F_SIB;
    gen_from_instruction(&instr, instruction, code_gen);
}

void gen_ret(code_gen_t *code_gen) {
    fwrite(&ret_opcode, sizeof(ret_opcode), 1, code_gen->out);
}

void gen_movzx(instruction_t *instruction, code_gen_t *code_gen) {
    machine_instruction_t instr;
    if(instruction->operand_2.size == 1) instr = movzx_8;
    if(instruction->operand_2.size == 2) instr = movzx_16;
    if(instruction->operand_2.flags & OP_MEMORY) instr.flags |= F_SIB;
    
    //for some reason, the movzx MODRM is flipped around?????
    operand_t swap_op = instruction->operand_2;
    instruction->operand_2 = instruction->operand_1;
    instruction->operand_1 = swap_op;

    gen_prefix(instruction, code_gen);
    
    gen_from_instruction(&instr, instruction, code_gen);
}

//BROKEN: movzx - for some reason its modrm is flipped??
//TODO: push, pop, memory addresses SIB, symbol table, jmp, jX (synonyms), call

void generate_code(code_gen_t *code_gen) {
    for(int i = 0; i < code_gen->parser->instruction_index; i++) {
        instruction_t *current_instruction = code_gen->parser->instructions[i];

        switch(current_instruction->opcode) {
            case K_MOV:
                gen_mov(current_instruction, code_gen);
                break;
            case K_MOVZX:
                gen_movzx(current_instruction, code_gen);
                break;
            case K_ADD:
            case K_OR:
            case K_ADC:
            case K_SBB:
            case K_AND:
            case K_SUB:
            case K_CMP:
                gen_arith(current_instruction, code_gen);
                break;
            case K_TEST:
            case K_NEG:
            case K_NOT:
            case K_MUL:
            case K_IMUL:
            case K_DIV:
            case K_IDIV:
                gen_arith_2(current_instruction, code_gen);
                break;
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
                gen_set(current_instruction, code_gen);
                break;
            case K_SYSCALL:
                gen_syscall(code_gen);
                break;
            case K_RET:
                gen_ret(code_gen);
                break;
            default:
                printf("Code gen error: unknown opcode %d\n", current_instruction->opcode);
                exit(1);
        }
    }
}