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
    if(number == 0) return 0;
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
    code_gen->byte_pos += sizeof(syscall_bytes);
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
    uint8_t prefix = 0;

    //32 bit memory prefix (both cannot be memory)
    if(instruction->operand_1.flags & OP_MEMORY && get_register_size(instruction->operand_1.reg) == 4) {
        prefix = 0x67;
        fwrite(&prefix, sizeof(prefix), 1, code_gen->out);
        code_gen->byte_pos += sizeof(prefix);
    }
    if(instruction->operand_2.flags & OP_MEMORY && get_register_size(instruction->operand_2.reg) == 4) {
        if(prefix != 0x67) {
            prefix = 0x67;
            fwrite(&prefix, sizeof(prefix), 1, code_gen->out);
            code_gen->byte_pos += sizeof(prefix);
        }
    }

    //16 bit prefix
    if(instruction->operand_1.size == 2 || instruction->operand_2.size == 2) {
        uint8_t prefix = 0x66;
        fwrite(&prefix, sizeof(prefix), 1, code_gen->out);
        code_gen->byte_pos += sizeof(prefix);
        if(!is_extended_register(instruction->operand_2.reg)) return;
    }
}

void gen_rex(instruction_t *instruction, code_gen_t *code_gen) {
    uint8_t rex = 0b01000000; //0b0100WRXB, W = is 64?, R = src reg is extended?, X = index reg used?, B = dest reg extended?
    instruction_t current_instr = *instruction; //make a copy so we can modify

    //make sure the memory operand is always #1
    if(current_instr.operand_2.flags & OP_MEMORY) {
        operand_t mem_operand;
        mem_operand = current_instr.operand_2;
        current_instr.operand_2 = current_instr.operand_1;
        current_instr.operand_1 = mem_operand;
    }

    //dest register
    if(is_extended_register(current_instr.operand_1.reg) && current_instr.operand_1.flags & OP_REGISTER) {
        if(!(current_instr.operand_1.reg >= K_SPL && current_instr.operand_1.reg <= K_DIL)) {
            rex |= 0b0001;
        }
    }

    //src register
    if(is_extended_register(current_instr.operand_2.reg) && current_instr.operand_2.flags & OP_REGISTER) {
        if(!(current_instr.operand_2.reg >= K_SPL && current_instr.operand_2.reg <= K_DIL)) {
            rex |= 0b0100;
        }
    }

    //dest reg 64?
    if(current_instr.operand_1.size == 8 && current_instr.operand_1.flags & OP_REGISTER) {
        rex |= 0b1000;
    }

    //src reg 64?
    if(current_instr.operand_2.size == 8 && current_instr.operand_2.flags & OP_REGISTER) {
        rex |= 0b1000;
    }

    //is index register extended?
    if(is_extended_register(current_instr.operand_2.index_reg)) {
        rex |= 0b0010;
    }

    if (rex != 0b01000000) {
        fwrite(&rex, sizeof(rex), 1, code_gen->out);
        code_gen->byte_pos += sizeof(rex);
    }
}

//writes Mod-reg-r/m byte    reg_opcode = if opcode can mean multiple things
uint8_t gen_modrm(instruction_t *instruction, uint8_t reg_opcode, code_gen_t *code_gen) {
    uint8_t modrm = 0b00000000; // [0 0] MOD, [0 0 0] Reg/Opcode, [0 0 0] R/M

    instruction_t current_instr = *instruction;

    //memory with no displacement
    if(current_instr.operand_1.flags & OP_MEMORY) {
        modrm |= 0b00000000; //keep first two zero, basically
        if(current_instr.operand_1.index_reg != 0 || current_instr.operand_1.mem_scale > 1) {
            modrm |= 0b100; //sib byte follows
        }
    }

    if(current_instr.operand_2.flags & OP_MEMORY) {
        modrm |= 0b00000000; //keep first two zero, basically
        if(current_instr.operand_2.index_reg != 0 || current_instr.operand_2.mem_scale > 1) {
            modrm |= 0b100; //sib byte follows
        }
    }

    //must also check for xBP registers because technically they have a displacement of zero
    if(get_int_size(current_instr.operand_2.mem_displacement) == 1 || (get_register_number(current_instr.operand_2.reg) == 5 && current_instr.operand_2.mem_displacement == 0 && current_instr.operand_2.flags & OP_MEMORY)) {
        modrm |= 0b01000000;
    }

    if(get_int_size(current_instr.operand_2.mem_displacement) == 4) {
        modrm |= 0b10000000;
    }

    //must also check for xBP registers because technically they have a displacement of zero
    if(get_int_size(current_instr.operand_1.mem_displacement) == 1 || (get_register_number(current_instr.operand_1.reg) == 5 && current_instr.operand_1.mem_displacement == 0 && current_instr.operand_1.flags & OP_MEMORY)) {
        modrm |= 0b01000000;
    }

    if(get_int_size(current_instr.operand_1.mem_displacement) == 4) {
        modrm |= 0b10000000;
    }

    if(get_int_size(current_instr.operand_2.mem_displacement) == 8) {
        printf("Integer memory address displacement must be 32-bit or below on line %ld\n", instruction->line_num);
        exit(1);
    }

    if(current_instr.operand_1.flags & OP_REGISTER && !(current_instr.operand_2.flags & OP_MEMORY)) {
        modrm |= 0b11000000;
    }

    if(current_instr.operand_1.flags & OP_MEMORY || current_instr.operand_2.flags & OP_MEMORY) {
        operand_t mem_operand;
        operand_t other_operand;

        if(current_instr.operand_1.flags & OP_MEMORY) {
            mem_operand = current_instr.operand_1;
            other_operand = current_instr.operand_2;
        }
        if(current_instr.operand_2.flags & OP_MEMORY) {
            mem_operand = current_instr.operand_2;
            other_operand = current_instr.operand_1;
        }

        if(other_operand.flags & OP_REGISTER) {
            modrm |= (get_register_number(other_operand.reg) << 3);
        }

        //Memory operand must always go in the R/M slot
        if(reg_opcode) {
            modrm |= gen_reg_opcode(&current_instr);
        }else if(!(modrm & 0b100)) {
            modrm |= (get_register_number(mem_operand.reg));
        }

        fwrite(&modrm, sizeof(modrm), 1, code_gen->out);
        code_gen->byte_pos += sizeof(modrm);

        if(get_register_number(current_instr.operand_1.reg) == 5 && current_instr.operand_1.mem_displacement == 0 && current_instr.operand_1.flags & OP_MEMORY) {
            uint8_t zero = 0;
            fwrite(&zero, sizeof(zero), 1, code_gen->out);
            code_gen->byte_pos += sizeof(zero);
        }

        if(get_register_number(current_instr.operand_2.reg) == 5 && current_instr.operand_2.mem_displacement == 0 && current_instr.operand_2.flags & OP_MEMORY) {
            uint8_t zero = 0;
            fwrite(&zero, sizeof(zero), 1, code_gen->out);
            code_gen->byte_pos += sizeof(zero);
        }

        return modrm;
    }

    //src
    if((current_instr.operand_2.flags & OP_REGISTER || current_instr.operand_2.flags & OP_MEMORY) && !reg_opcode) {
        modrm |= (get_register_number(current_instr.operand_2.reg) << 3);
    }

    //add/sub/etc
    if(reg_opcode) {
        modrm |= gen_reg_opcode(&current_instr);
    }

    //dest
    if(!(modrm & 0b100)) {
        modrm |= (get_register_number(current_instr.operand_1.reg));
    }

    fwrite(&modrm, sizeof(modrm), 1, code_gen->out);
    code_gen->byte_pos += sizeof(modrm);
    //printf("modrm: %d\n", modrm);

    return modrm;
}

void gen_sib(instruction_t *instruction, code_gen_t *code_gen) {
    uint8_t sib = 0b00000000; //[0 0] scale (1,2,4,8), [0 0 0] index reg, [0 0 0] base reg

    if(!(instruction->operand_1.flags & OP_MEMORY) && !(instruction->operand_2.flags & OP_MEMORY)) return;
    if((instruction->operand_1.flags & OP_MEMORY) && (instruction->operand_2.flags & OP_MEMORY)) {
        printf("Cannot have two memory addresses for one instruction on line %ld\n", instruction->line_num);
        exit(1);
    }

    operand_t operand;

    uint8_t gen_zero_disp = 0; //should it generate all zero displacement?

    if(instruction->operand_1.flags & OP_MEMORY) operand = instruction->operand_1;
    if(instruction->operand_2.flags & OP_MEMORY) operand = instruction->operand_2;

    //xBP registers do not need SIB byte if it is just a displacement
    if(get_register_number(operand.reg) == 5 && operand.mem_scale == 1) return;

    switch(operand.mem_scale) {
        case 1:
            sib |= (0b00 << 6);
            break;
        case 2:
            sib |= (0b01 << 6);
            break;
        case 4:
            sib |= (0b10 << 6);
            break;
        case 8:
            sib |= (0b11 << 6);
            break;
    }

    //index register
    if(operand.index_reg > 0) {
        sib |= (get_register_number(operand.index_reg) << 3);
    }else {
        sib |= (0b100 << 3);
    }

    //base register
    if(operand.mem_scale > 1 && operand.index_reg == 0) {
        sib |= 0b101;
        sib &= (0b11000111);
        sib |= (get_register_number(operand.reg) << 3);
        gen_zero_disp = operand.mem_displacement ? 0 : 1;
    }else {
        sib |= (get_register_number(operand.reg));
    }

    //printf("sib: %d\n", sib);

    fwrite(&sib, sizeof(sib), 1, code_gen->out);
    code_gen->byte_pos += sizeof(sib);
    if(gen_zero_disp) {
        uint32_t zeros = 0;
        fwrite(&zeros, sizeof(zeros), 1, code_gen->out);
        code_gen->byte_pos += sizeof(zeros);
    }
}

void gen_displacement(instruction_t *instruction, code_gen_t *code_gen) {
    int64_t displacement;
    if(instruction->operand_1.mem_displacement > 0 || instruction->operand_1.mem_displacement < 0) displacement = instruction->operand_1.mem_displacement;
    if(instruction->operand_2.mem_displacement > 0 || instruction->operand_2.mem_displacement < 0) displacement = instruction->operand_2.mem_displacement;

    uint8_t size = get_int_size(displacement);
    if(size == 0) size = 1;
    fwrite(&displacement, size, 1, code_gen->out);
    code_gen->byte_pos += size;
}

void gen_from_instruction(machine_instruction_t *machine_code, instruction_t *instruction, code_gen_t *code_gen) {
    uint8_t opcode;
    opcode = machine_code->opcode;
    if(machine_code->flags & F_ADD_REG) opcode |= get_register_number(instruction->operand_1.reg);
    if(machine_code->flags & F_TWO_BYTE) {
        fwrite(&two_byte_opcode, sizeof(two_byte_opcode), 1, code_gen->out);
        code_gen->byte_pos += sizeof(two_byte_opcode);
    }

    fwrite(&opcode, sizeof(opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(opcode);

    uint8_t modrm = 0;
    if(machine_code->flags & F_MODRM) modrm = gen_modrm(instruction, machine_code->flags & F_REG_OPCODE, code_gen);
    if(instruction->operand_1.index_reg > 0 || instruction->operand_1.mem_scale > 1) machine_code->flags |= F_SIB;
    if(instruction->operand_2.index_reg > 0 || instruction->operand_2.mem_scale > 1) machine_code->flags |= F_SIB;
    if(machine_code->flags & F_SIB || modrm & 0b100) gen_sib(instruction, code_gen);
    if(instruction->operand_1.mem_displacement > 0 || instruction->operand_1.mem_displacement < 0) gen_displacement(instruction, code_gen);
    if(instruction->operand_2.mem_displacement > 0 || instruction->operand_2.mem_displacement < 0) gen_displacement(instruction, code_gen);
}

//mov rax, 9 -> 48 c7 c0 09 00 00 00
//mov r8, 9  -> 49 c7 c0 09 00 00 00
//mov r8d, 9 ->    41 b8 09 00 00 00
//mov r8w, 9 -> 66 41 b8 09 00
//mov r8b, 9 ->    41 b0 09

void gen_mov(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);

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
        machine_instruction_t instr = mov_reg_reg[get_size_index(instruction->operand_2.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_mem) {
        machine_instruction_t instr = mov_reg_mem[get_size_index(instruction->operand_1.size)];
        //instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_imm) {
        machine_instruction_t instr = mov_mem_imm[get_size_index(instruction->operand_2.size)];
        //instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_2_imm) {
        int64_t intlit = instruction->operand_2.intlit;
        int size = 0;
        if(op_1_mem) size = instruction->operand_2.size;
        else size = instruction->operand_1.size;
        if(size > 4) size = 4;
        switch(instruction->operand_1.size) {
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, size, 1, code_gen->out);
                code_gen->byte_pos += size;
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, size, 1, code_gen->out);
                code_gen->byte_pos += size;
                break;
            case 4:
            case 8: //just because its 8 doesnt mean we put all 8 bytes in -- max 4
                intlit = (int32_t)intlit;
                fwrite(&intlit, size, 1, code_gen->out);
                code_gen->byte_pos += size;
                break;
        }
    }
}

//add, or, adc, sbb, and, sub, xor, cmp
void gen_arith(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);

    uint8_t opcode;
    uint8_t op_1_reg = instruction->operand_1.flags & OP_REGISTER;
    uint8_t op_1_mem = instruction->operand_1.flags & OP_MEMORY;
    uint8_t op_2_reg = instruction->operand_2.flags & OP_REGISTER;
    uint8_t op_2_mem = instruction->operand_2.flags & OP_MEMORY;
    uint8_t op_2_imm = instruction->operand_2.flags & OP_INTLIT;

    if(op_1_reg && op_2_reg) {
        machine_instruction_t instr = arith_reg_reg[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_imm && instruction->operand_2.size == 1) {
        machine_instruction_t instr = arith_reg_imm[get_size_index(instruction->operand_1.size)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    //16, 32, or 64 bit imm (64 just makes 32 anynway)
    if(op_1_reg && op_2_imm && instruction->operand_2.size > 1) {
        machine_instruction_t instr = arith_reg_imm[get_size_index(instruction->operand_1.size + 4)];
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_reg) {
        machine_instruction_t instr = arith_reg_reg[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        //instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_reg && op_2_mem) {
        machine_instruction_t instr = arith_reg_mem[get_size_index(instruction->operand_1.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        //instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(op_1_mem && op_2_imm) {
        machine_instruction_t instr = arith_reg_imm[get_size_index(instruction->operand_2.size)];
        instr.opcode += (8 * (instruction->opcode - K_ADD));
        //instr.flags |= F_SIB;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    if(instruction->operand_2.flags & OP_INTLIT) {
        int64_t intlit = instruction->operand_2.intlit;
        uint8_t int_size = get_int_size(intlit);
        switch(int_size) {
            case 0:
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, 1, 1, code_gen->out);
                code_gen->byte_pos++;
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, 2, 1, code_gen->out);
                code_gen->byte_pos += 2;
                break;
            case 4:
            case 8:
                intlit = (int32_t)intlit;
                fwrite(&intlit, 4, 1, code_gen->out);
                code_gen->byte_pos += 4;
                break;
        }
    }
}

//test, not, neg, mul, imul, div, idiv
void gen_arith_2(instruction_t *instruction, code_gen_t *code_gen) {
    int reg_size = get_register_size(instruction->operand_1.reg);

    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);

    uint8_t opcode;
    uint8_t op_1_reg = instruction->operand_1.flags & OP_REGISTER;
    uint8_t op_1_mem = instruction->operand_1.flags & OP_MEMORY;
    uint8_t op_2_reg = instruction->operand_2.flags & OP_REGISTER;
    uint8_t op_2_mem = instruction->operand_2.flags & OP_MEMORY;
    uint8_t op_2_imm = instruction->operand_2.flags & OP_INTLIT;

    if(instruction->operand_1.size == 1) {
        machine_instruction_t instr = arith_2_8;
        gen_from_instruction(&instr, instruction, code_gen);
    }
    
    if(instruction->operand_1.size > 1) {
        machine_instruction_t instr = arith_2_16;
        gen_from_instruction(&instr, instruction, code_gen);
    }

    //only test accepts imm for operand 2
    if(instruction->operand_2.flags & OP_INTLIT && instruction->opcode == K_TEST) {
        int64_t intlit = instruction->operand_2.intlit;
        uint8_t int_size = get_int_size(intlit);
        switch(int_size) {
            case 0:
            case 1:
                intlit = (int8_t)intlit;
                fwrite(&intlit, 1, 1, code_gen->out);
                code_gen->byte_pos++;
                break;
            case 2:
                intlit = (int16_t)intlit;
                fwrite(&intlit, 2, 1, code_gen->out);
                code_gen->byte_pos += 2;
                break;
            case 4:
            case 8:
                intlit = (int32_t)intlit;
                fwrite(&intlit, 4, 1, code_gen->out);
                code_gen->byte_pos += 4;
                break;
        }
    }
}

void gen_set(instruction_t *instruction, code_gen_t *code_gen) {
    machine_instruction_t instr;

    instr = set_reg;
    instr.opcode += instruction->opcode - K_SETO; //adds appropriate val to 0F90, keyword enum is in right order
    //if(instruction->operand_2.flags & OP_MEMORY) instr.flags |= F_SIB;
    gen_from_instruction(&instr, instruction, code_gen);
}

void gen_ret(code_gen_t *code_gen) {
    fwrite(&ret_opcode, sizeof(ret_opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(ret_opcode);
}

void gen_movzx(instruction_t *instruction, code_gen_t *code_gen) {
    machine_instruction_t instr;
    if(instruction->operand_2.size == 1) instr = movzx_8;
    if(instruction->operand_2.size == 2) instr = movzx_16;
    //if(instruction->operand_2.flags & OP_MEMORY) instr.flags |= F_SIB;
    
    //for some reason, the movzx MODRM is flipped around?????
    operand_t swap_op = instruction->operand_2;
    instruction->operand_2 = instruction->operand_1;
    instruction->operand_1 = swap_op;

    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);
    
    gen_from_instruction(&instr, instruction, code_gen);
}

void gen_push(instruction_t *instruction, code_gen_t *code_gen) {
    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);
    gen_from_instruction(&push_reg, instruction, code_gen);
}

void gen_pop(instruction_t *instruction, code_gen_t *code_gen) {
    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);
    gen_from_instruction(&pop_reg, instruction, code_gen);
}

void gen_lea(instruction_t *instruction, code_gen_t *code_gen) {
    gen_prefix(instruction, code_gen);
    gen_rex(instruction, code_gen);
    gen_from_instruction(&lea_mem, instruction, code_gen);
}

void handle_label_decl(instruction_t *instruction, code_gen_t *code_gen) {
    int symbol_index = find_symbol(instruction->operand_1.label_name, code_gen->parser->symbol_table);
    if(symbol_index == -1) {
        printf("An error has occured when processing label symbols\n");
    }

    symbol_entry_t *symbol = code_gen->parser->symbol_table->table[symbol_index];

    symbol->address = code_gen->byte_pos;

    for(int i = 0; i < code_gen->unresolved_next_free; i++) {
        unresolved_label_t *label = code_gen->unresolved_labels[i];
        if(label->symbol == symbol) {
            uint64_t current_pos = code_gen->byte_pos + 0x1000;

            fseek(code_gen->out, label->file_pos, 0);

            int32_t offset = symbol->address - (label->file_pos - 0x1000 + 4); //+ 4 is + <32bit offset>
            
            fwrite(&offset, sizeof(offset), 1, code_gen->out);
            
            fseek(code_gen->out, 0, SEEK_END);
        }
    }
}

void gen_label_offset(machine_instruction_t machine_instr, instruction_t *instruction, operand_t *operand, code_gen_t *code_gen) {
    int symbol_index = find_symbol(operand->label_name, code_gen->parser->symbol_table);
    if(symbol_index == -1) {
        printf("Label %s on line %ld has not been defined in this program\n", operand->label_name, instruction->line_num);
    }

    int32_t address = code_gen->parser->symbol_table->table[symbol_index]->address;
    
    if(address == -1) {
        //this means the label isn't before this instruction, so the address hasn't been determined yet
        //now we must remember this and come back to it once the label is defined
        
        if(code_gen->unresolved_next_free++ > code_gen->unresolved_label_size) {
            code_gen->unresolved_label_size += 256;
            code_gen->unresolved_labels = (unresolved_label_t **)realloc(code_gen->unresolved_labels, code_gen->unresolved_label_size * sizeof(unresolved_label_t *));
        }
        
        unresolved_label_t *label = (unresolved_label_t *)malloc(sizeof(unresolved_label_t));
        label->file_pos = 0x1000 + code_gen->byte_pos; //0x1000 is where the machine code starts in the ELF file, the jmp opcode is already accounted for
        label->symbol = code_gen->parser->symbol_table->table[symbol_index];
        code_gen->unresolved_labels[code_gen->unresolved_next_free - 1] = label;

        int32_t offset = 0xFFFFFFFF;
        fwrite(&offset, sizeof(offset), 1, code_gen->out);

        code_gen->byte_pos += sizeof(offset);
    }else {
        int32_t offset = address - (code_gen->byte_pos + 3); //+ 3 is <32bit offset> - 1
        if(machine_instr.flags & F_TWO_BYTE) offset -= 1; //must go back one more
        fwrite(&offset, sizeof(offset), 1, code_gen->out);
        printf("%d\n", address);

        code_gen->byte_pos += sizeof(offset);
    }
}

void gen_jmp(instruction_t *instruction, code_gen_t *code_gen) {
    fwrite(&jmp_opcode.opcode, sizeof(jmp_opcode.opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(jmp_opcode.opcode);
    
    gen_label_offset(jmp_opcode, instruction, &instruction->operand_1, code_gen);
}

void gen_call(instruction_t *instruction, code_gen_t *code_gen) {
    fwrite(&call_opcode.opcode, sizeof(call_opcode.opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(call_opcode.opcode);
    
    gen_label_offset(call_opcode, instruction, &instruction->operand_1, code_gen);
}

void gen_jmp_compare(instruction_t *instruction, code_gen_t *code_gen) {
    machine_instruction_t instr;

    instr = jmp_compare;
    instr.opcode += instruction->opcode - K_JO; //adds appropriate val to 0F80, keyword enum is in right order
    
    fwrite(&two_byte_opcode, sizeof(two_byte_opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(two_byte_opcode);
    fwrite(&instr.opcode, sizeof(instr.opcode), 1, code_gen->out);
    code_gen->byte_pos += sizeof(instr.opcode);
    
    gen_label_offset(jmp_compare, instruction, &instruction->operand_1, code_gen);
}

//TODO: symbol table, jmp, jX (synonyms), call
void generate_code(code_gen_t *code_gen) {
    code_gen->unresolved_label_size = 128;
    code_gen->unresolved_next_free = 0;
    code_gen->unresolved_labels = (unresolved_label_t **)malloc(sizeof(unresolved_label_t) * code_gen->unresolved_label_size);

    for(int i = 0; i < code_gen->parser->instruction_index; i++) {
        instruction_t *current_instruction = code_gen->parser->instructions[i];

        switch(current_instruction->opcode) {
            case K_LABEL:
                handle_label_decl(current_instruction, code_gen);
                break;
            case K_MOV:
                gen_mov(current_instruction, code_gen);
                break;
            case K_MOVZX:
                gen_movzx(current_instruction, code_gen);
                break;
            case K_LEA:
                gen_lea(current_instruction, code_gen);
                break;
            case K_ADD:
            case K_XOR:
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
            case K_JMP:
                gen_jmp(current_instruction, code_gen);
                break;
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
                gen_jmp_compare(current_instruction, code_gen);
                break;
            case K_CALL:
                gen_call(current_instruction, code_gen);
                break;
            case K_PUSH:
                gen_push(current_instruction, code_gen);
                break;
            case K_POP:
                gen_pop(current_instruction, code_gen);
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