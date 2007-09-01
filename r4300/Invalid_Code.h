/* Invalid_Code.h - Uses 1/8th the memory as the char hash table
   by Mike Slegeir for Mupen64-GC
 */

#ifndef INVALID_CODE_H
#define INVALID_CODE_H

int inline invalid_code_get(int block_num);
void inline invalid_code_set(int block_num, int value);

#endif
