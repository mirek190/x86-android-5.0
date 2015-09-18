/*
 * * backtrace dump tool
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
 * */
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "symbols.h"
//#include <linux/elf.h>
#include <elf.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/syscall.h>
#include <stdio.h>

static long qcompar64(const void *a, const void *b)
{
	return ((struct symbol*)a)->addr - ((struct symbol*)b)->addr;
}

// Compare func for bsearch
static int bcompar64(const void *addr, const void *element)
{
	struct symbol *symbol = (struct symbol*)element;


	if((unsigned long)addr < symbol->addr) {
		return -1;
	}

	if((unsigned long)addr - symbol->addr >= symbol->size) {
		return 1;
	}

	return 0;
}

/*
 *  Create a symbol table from a given file
 *
 *  Parameters:
 *      filename - Filename to process
 *
 *  Returns:
 *      A newly-allocated SymbolTable structure, or NULL if error.
 *      Free symbol table with symbol_table_free()
 */
struct symbol_table *symbol_tables_create64(const char *filename )
{
	struct symbol_table *table = NULL;

	// Open the file, and map it into memory
	struct stat sb;
	long length;
	char *base = NULL;
	bool isELF32 = false;
	int fd = open(filename, O_RDONLY);

	if(fd < 0) {
		goto out;
	}

	fstat(fd, &sb);
	length = sb.st_size;

	base = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);

	if(!base) {
		goto out_close;
	}
	// Parse the file header
	Elf64_Ehdr*	hdr = (Elf64_Ehdr*)base;
	Elf64_Shdr*	shdr = (Elf64_Shdr*)(base + hdr->e_shoff);
	Elf64_Phdr*    pdr = (Elf64_Phdr*)(base + hdr->e_phoff);
	// Search for the dynamic symbols section
	int sym_idx = -1;
	int dynsym_idx = -1;
	int i;

	for(i = 0; i < hdr->e_shnum; i++) {
		if(shdr[i].sh_type == SHT_SYMTAB ) {
			sym_idx = i;
		}
		if(shdr[i].sh_type == SHT_DYNSYM ) {
			dynsym_idx = i;
		}
	}
	if ((dynsym_idx == -1) && (sym_idx == -1)) {
		goto out_unmap;
	}

	table = malloc(sizeof(struct symbol_table));
	if(!table) {
		goto out_unmap;
	}
	table->name = strdup(filename);
	table->num_symbols = 0;
	Elf64_Sym *dynsyms = NULL;
	Elf64_Sym *syms = NULL;
	int dynnumsyms = 0;
	int numsyms = 0;
	char *dynstr = NULL;
	char *str = NULL;
	if (dynsym_idx != -1) {
		dynsyms = (Elf64_Sym*)(base + shdr[dynsym_idx].sh_offset);
		dynnumsyms = shdr[dynsym_idx].sh_size / shdr[dynsym_idx].sh_entsize;
		int dynstr_idx = shdr[dynsym_idx].sh_link;
		dynstr = base + shdr[dynstr_idx].sh_offset;
	}

	if (sym_idx != -1) {
		syms = (Elf64_Sym*)(base + shdr[sym_idx].sh_offset);
		numsyms = shdr[sym_idx].sh_size / shdr[sym_idx].sh_entsize;
		int str_idx = shdr[sym_idx].sh_link;
		str = base + shdr[str_idx].sh_offset;
	}

	int symbol_count = 0;
	int dynsymbol_count = 0;

	if (dynsym_idx != -1) {
		// Iterate through the dynamic symbol table, and count how many symbols
		// are actually defined
		for(i = 0; i < dynnumsyms; i++) {
			if(dynsyms[i].st_shndx != SHN_UNDEF) {
				dynsymbol_count++;
			}
		}
	}

	if (sym_idx != -1) {
		// Iterate through the symbol table, and count how many symbols
		// are actually defined
		for(i = 0; i < numsyms; i++) {
			if((syms[i].st_shndx != SHN_UNDEF) &&
					(strlen(str+syms[i].st_name)) &&
					(syms[i].st_value != 0) && (syms[i].st_size != 0)) {
				symbol_count++;
			}
		}
	}

	// Now, create an entry in our symbol table structure for each symbol...
	table->num_symbols += symbol_count + dynsymbol_count;
	table->symbols = NULL;
	table->symbols = malloc(table->num_symbols * sizeof(struct symbol));
	if(!table->symbols) {
		free(table);
		table = NULL;
		goto out_unmap;
	}


	int j = 0;
	if (dynsym_idx != -1) {
		// ...and populate them
		for(i = 0; i < dynnumsyms; i++) {
			if(dynsyms[i].st_shndx != SHN_UNDEF) {
				table->symbols[j].name = strdup(dynstr + dynsyms[i].st_name);
				table->symbols[j].addr = dynsyms[i].st_value ;
				table->symbols[j].size = dynsyms[i].st_size;
				j++;
			}
		}
	}

	if (sym_idx != -1) {
		// ...and populate them
		for(i = 0; i < numsyms; i++) {
			if((syms[i].st_shndx != SHN_UNDEF) &&
					(strlen(str+syms[i].st_name)) &&
					(syms[i].st_value != 0) && (syms[i].st_size != 0)) {
				table->symbols[j].name = strdup(str + syms[i].st_name);
				table->symbols[j].addr = syms[i].st_value ;
				table->symbols[j].size = syms[i].st_size;
				j++;
			}
		}
	}

	// Sort the symbol table entries, so they can be bsearched later
	qsort(table->symbols, table->num_symbols, sizeof(struct symbol), qcompar64);

out_unmap:
	munmap(base, length);

out_close:
	close(fd);

out:
	return table;
}

/*
 * Free a symbol table
 *
 * Parameters:
 *     table - Table to free
 */
void symbol_tables_free64(struct symbol_table *table)
{
	int i;
	if(!table) {
		return;
	}

	for(i=0; i<table->num_symbols; i++) {
		free(table->symbols[i].name);
	}

	free(table->symbols);
	free(table);
}

/*
 * Search for an address in the symbol table
 *
 * Parameters:
 *      table - Table to search in
 *      addr - Address to search for.
 *
 * Returns:
 *      A pointer to the Symbol structure corresponding to the
 *      symbol which contains this address, or NULL if no symbol
 *      contains it.
 */
const struct symbol *symbol_tables_lookup64(struct symbol_table *table,
			unsigned long addr)
{
	if(!table)
		return NULL;
	return bsearch((void*)addr, table->symbols,
			table->num_symbols,
			sizeof(struct symbol), bcompar64);
}



/* Find the containing map info for the pc */
const mapinfo *pc_to_mapsinfo64(mapinfo *mi, unsigned long pc, unsigned long *rel_pc)
{
	*rel_pc = pc;
	while (mi) {
		if((pc >= mi->start) && (pc < mi->end)){
			/*
			 * Only calculate the relative offset for
			 * shared libraries
			 */
			if (strstr(mi->name, ".so")) {
				*rel_pc -= mi->start;
			}
			return mi;
		}
		mi = mi->next;
	}
	return NULL;
}


int unwind_backtrace_with_stack64( unsigned long eip[], unsigned int ebp, mapinfo *map)
{
	unsigned int stack_level = 0;
	unsigned int stack_depth = 0;
	unsigned long rel_pc;
	unsigned long stack_ptr;
	unsigned int stack_content;
	const mapinfo *mi;
	const struct symbol* sym = 0;
	unsigned long ip;


	while (stack_level < ebp) {
		ip = eip[stack_level];
		mi = pc_to_mapsinfo64(map, ip, &rel_pc);
		/* See if we can determine what symbol this stack frame resides in */
		if (mi != 0 && mi->symbols != 0) {
			sym = symbol_tables_lookup64(mi->symbols, rel_pc);
		}
		if (sym) {
			printf("[%016lx]  %s (%s+%u)\n", rel_pc, mi ? mi->name : "", sym->name, rel_pc - sym->addr);
		} else {
			printf("[%016lx]  %s\n", rel_pc, mi ? mi->name : "");
		}
		stack_level++;
		if (stack_level >= STACK_DEPTH )
		{
				break;
			}
		//eip++;
	}


	stack_level = 0;

	return 1;
}


