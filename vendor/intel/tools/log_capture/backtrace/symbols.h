 /*                        
  *                        * * backtrace dump tool
  *                        ** Copyright 2006, The Android Open Source Project
  *                        ** 
  *                        ** Licensed under the Apache License, Version 2.0 (the "License");
  *                        ** you may not use this file except in compliance with the License.
  *                        ** You may obtain a copy of the License at
  *                        **                        
  *                        **     http://www.apache.org/licenses/LICENSE-2.0
  *                        ** 
  *                        ** Unless required by applicable law or agreed to in writing, software
  *                        ** distributed under the License is distributed on an "AS IS" BASIS,
  *                        ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  *                        ** See the License for the specific language governing permissions and
  *                        ** limitations under the License.
  *                        * */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#define STACK_CONTENT_DEPTH 64
#define STACK_DEPTH 64
#define LOG_NDEBUG			0
#define TASK_NUM  			500
#define MAX_STACK_LENGTH		32
#define MAX_KERNEL_STACK_DEPTH 		20
#define MAX_KERNELSTACK_SIZE 		5120 //20*256, 4k


#define PATH_LENGTH			256
#define NAME_LENGTH			20

#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
		(ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
		(ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
		(ehdr).e_ident[EI_MAG3] == ELFMAG3)

typedef struct mapinfo {
    struct mapinfo *next;
    unsigned long start;
    unsigned long end;
    unsigned long  exidx_start;
    unsigned long exidx_end;
    struct symbol_table *symbols;
    char name[256];
} mapinfo;

struct symbol {
	unsigned long addr;
	unsigned long size;
	char *name;
};

struct symbol_table {
	struct symbol *symbols;
	long num_symbols;
	char *name;
};
//32 bit
struct symbol_table *symbol_tables_create(const char *filename);
void symbol_tables_free(struct symbol_table *table);
const struct symbol *symbol_tables_lookup(struct symbol_table *table, unsigned long addr);
/* Find the containing map for the pc */
extern const mapinfo *pc_to_mapsinfo (mapinfo *mi, unsigned long pc, unsigned long *rel_pc);

//64 bit
struct symbol_table *symbol_tables_create64(const char *filename);
void symbol_tables_free64(struct symbol_table *table);
const struct symbol *symbol_tables_lookup64(struct symbol_table *table, unsigned long addr);
/* Find the containing map for the pc */
extern const mapinfo *pc_to_mapsinfo64(mapinfo *mi, unsigned long pc, unsigned long *rel_pc);

/////////////////////////////////////

int unwind_backtrace_with_stack( unsigned long eip[], unsigned int ebp, mapinfo *map);

int unwind_backtrace_with_stack_file( unsigned long eip[],unsigned long ebp,mapinfo *map, char* buf[]);

int unwind_backtrace_with_stack64( unsigned long eip[], unsigned int ebp, mapinfo *map);
#endif
