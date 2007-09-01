/* Invalid_Code.c - Uses 1/8th the memory as the char hash table
   by Mike Slegeir for Mupen64-GC
 */

#include "Invalid_Code.h"

static char invalid_code[0x100000/8];

int inline invalid_code_get(int block_num){
	return invalid_code[block_num>>3] & (1<<(block_num&0x7));
}

void inline invalid_code_set(int block_num, int value){
	if(value) invalid_code[block_num>>3] |=  (1<<(block_num&0x7));
	else      invalid_code[block_num>>3] &= ~(1<<(block_num&0x7));
}
