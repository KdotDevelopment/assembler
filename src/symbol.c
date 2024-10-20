#include "symbol.h"

#include <stdlib.h>
#include <string.h>

int find_symbol(char *s, symbol_table_t *symbol_table) {
	for(int i = 0; i < symbol_table->next_free; i++) {
		if(*s == *symbol_table->table[i]->name && !strcmp(s, symbol_table->table[i]->name)) {
			return i;
		}
	}
	return -1;
}

int next_symbol_pos(symbol_table_t *symbol_table) {
	int p;
	if((p = symbol_table->next_free++) >= symbol_table->table_size) {
        symbol_table->table_size += 256;
        symbol_table->table = (symbol_entry_t **)realloc(symbol_table, symbol_table->table_size * sizeof(symbol_entry_t *));
	}
	return p;
}

//set address to -1 if unknown
int create_symbol(char *name, uint64_t address, symbol_table_t *symbol_table) {
	int symbol;

	//if it already exists
	if((symbol = find_symbol(name, symbol_table) != -1)) {
		printf("Symbol %s already exists!\n", name);
		exit(1);
	}

	symbol = next_symbol_pos(symbol_table);
    symbol_entry_t *entry = (symbol_entry_t *)malloc(sizeof(symbol_entry_t));
    entry->name = name;
    entry->address = address;
	symbol_table->table[symbol] = entry;

	return symbol;
}