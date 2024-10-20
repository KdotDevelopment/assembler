#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct {
    char *name;
    uint32_t address; //basically, address
} symbol_entry_t;

typedef struct {
    uint64_t instruction_pointer; //position in the simulated memory
    uint64_t table_size; //memory allocated to table
    uint64_t next_free; //next available position in table
    symbol_entry_t **table;
} symbol_table_t;

int find_symbol(char *s, symbol_table_t *symbol_table);
int create_symbol(char *name, uint64_t address, symbol_table_t *symbol_table);