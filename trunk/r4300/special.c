/**
 * Mupen64 - special.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include "r4300.h"
#include "interupt.h"
#include "../gc_memory/memory.h"
#include "ops.h"
#include "exception.h"
#include "macros.h"

void NOP()
{
   PC++;
}

void SLL()
{
   rrd32 = (unsigned long)(rrt32) << rsa;
   sign_extended(rrd);
   PC++;
}

void SRL()
{
   rrd32 = (unsigned long)rrt32 >> rsa;
   sign_extended(rrd);
   PC++;
}

void SRA()
{
   rrd32 = (signed long)rrt32 >> rsa;
   sign_extended(rrd);
   PC++;
}

void SLLV()
{
   rrd32 = (unsigned long)(rrt32) << (rrs32&0x1F);
   sign_extended(rrd);
   PC++;
}

void SRLV()
{
   rrd32 = (unsigned long)rrt32 >> (rrs32 & 0x1F);
   sign_extended(rrd);
   PC++;
}

void SRAV()
{
   rrd32 = (signed long)rrt32 >> (rrs32 & 0x1F);
   sign_extended(rrd);
   PC++;
}

void JR()
{
   local_rs32 = irs32;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   jump_to(local_rs32);
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void JALR()
{
   unsigned long long int *dest = PC->f.r.rd;
   local_rs32 = rrs32;
   PC++;
   delay_slot=1;
   PC->ops();
   update_count();
   delay_slot=0;
   if (!skip_jump)
     {
	*dest = PC->addr;
	sign_extended(*dest);
	
	jump_to(local_rs32);
     }
   last_addr = PC->addr;
   if (next_interupt <= Count) gen_interupt();
}

void SYSCALL()
{
   Cause = 8 << 2;
   exception_general();
}

void SYNC()
{
   PC++;
}

void MFHI()
{
   rrd = hi;
   PC++;
}

void MTHI()
{
   hi = rrs;
   PC++;
}

void MFLO()
{
   rrd = lo;
   PC++;
}

void MTLO()
{
   lo = rrs;
   PC++;
}

void DSLLV()
{
   rrd = rrt << (rrs32&0x3F);
   PC++;
}

void DSRLV()
{
   rrd = (unsigned long long)rrt >> (rrs32 & 0x3F);
   PC++;
}

void DSRAV()
{
   rrd = (long long)rrt >> (rrs32 & 0x3F);
   PC++;
}

void MULT()
{
   long long int temp;
   temp = rrs * rrt;
   hi = temp >> 32;
   lo = temp;
   sign_extended(lo);
   PC++;
}

void MULTU()
{
   unsigned long long int temp;
   temp = (unsigned long)rrs * (unsigned long long)((unsigned long)rrt);
   hi = (long long)temp >> 32;
   lo = temp;
   sign_extended(lo);
   PC++;
}

void DIV()
{
   if (rrt32)
     {
	lo = rrs32 / rrt32;
	hi = rrs32 % rrt32;
	sign_extended(lo);
	sign_extended(hi);
     }
   else printf("div\n");
   PC++;
}

void DIVU()
{
   if (rrt32)
     {
	lo = (unsigned long)rrs32 / (unsigned long)rrt32;
	hi = (unsigned long)rrs32 % (unsigned long)rrt32;
	sign_extended(lo);
	sign_extended(hi);
     }
   else printf("divu\n");
   PC++;
}

void DMULT()
{
   unsigned long long int op1, op2, op3, op4;
   unsigned long long int result1, result2, result3, result4;
   unsigned long long int temp1, temp2, temp3, temp4;
   int sign = 0;
   
   if (rrs < 0)
     {
	op2 = -rrs;
	sign = 1 - sign;
     }
   else op2 = rrs;
   if (rrt < 0)
     {
	op4 = -rrt;
	sign = 1 - sign;
     }
   else op4 = rrt;
   
   op1 = op2 & 0xFFFFFFFF;
   op2 = (op2 >> 32) & 0xFFFFFFFF;
   op3 = op4 & 0xFFFFFFFF;
   op4 = (op4 >> 32) & 0xFFFFFFFF;
   
   temp1 = op1 * op3;
   temp2 = (temp1 >> 32) + op1 * op4;
   temp3 = op2 * op3;
   temp4 = (temp3 >> 32) + op2 * op4;
   
   result1 = temp1 & 0xFFFFFFFF;
   result2 = temp2 + (temp3 & 0xFFFFFFFF);
   result3 = (result2 >> 32) + temp4;
   result4 = (result3 >> 32);
   
   lo = result1 | (result2 << 32);
   hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
   if (sign)
     {
	hi = ~hi;
	if (!lo) hi++;
	else lo = ~lo + 1;
     }
   PC++;
}

void DMULTU()
{
   unsigned long long int op1, op2, op3, op4;
   unsigned long long int result1, result2, result3, result4;
   unsigned long long int temp1, temp2, temp3, temp4;
   
   op1 = rrs & 0xFFFFFFFF;
   op2 = (rrs >> 32) & 0xFFFFFFFF;
   op3 = rrt & 0xFFFFFFFF;
   op4 = (rrt >> 32) & 0xFFFFFFFF;
   
   temp1 = op1 * op3;
   temp2 = (temp1 >> 32) + op1 * op4;
   temp3 = op2 * op3;
   temp4 = (temp3 >> 32) + op2 * op4;
   
   result1 = temp1 & 0xFFFFFFFF;
   result2 = temp2 + (temp3 & 0xFFFFFFFF);
   result3 = (result2 >> 32) + temp4;
   result4 = (result3 >> 32);
   
   lo = result1 | (result2 << 32);
   hi = (result3 & 0xFFFFFFFF) | (result4 << 32);
   
   PC++;
}

void DDIV()
{
   if (rrt)
     {
	lo = (long long int)rrs / (long long int)rrt;
	hi = (long long int)rrs % (long long int)rrt;
     }
//   else printf("ddiv\n");
   PC++;
}

void DDIVU()
{
   if (rrt)
     {
	lo = (unsigned long long int)rrs / (unsigned long long int)rrt;
	hi = (unsigned long long int)rrs % (unsigned long long int)rrt;
     }
//   else printf("ddivu\n");
   PC++;
}

void ADD()
{
   rrd32 = rrs32 + rrt32;
   sign_extended(rrd);
   PC++;
}

void ADDU()
{
   rrd32 = rrs32 + rrt32;
   sign_extended(rrd);
   PC++;
}

void SUB()
{
   rrd32 = rrs32 - rrt32;
   sign_extended(rrd);
   PC++;
}

void SUBU()
{
   rrd32 = rrs32 - rrt32;
   sign_extended(rrd);
   PC++;
}

void AND()
{
   rrd = rrs & rrt;
   PC++;
}

void OR()
{
   rrd = rrs | rrt;
   PC++;
}

void XOR()
{
   rrd = rrs ^ rrt;
   PC++;
}

void NOR()
{
   rrd = ~(rrs | rrt);
   PC++;
}

void SLT()
{
   if (rrs < rrt) rrd = 1;
   else rrd = 0;
   PC++;
}

void SLTU()
{
   if ((unsigned long long)rrs < (unsigned long long)rrt) 
     rrd = 1;
   else rrd = 0;
   PC++;
}

void DADD()
{
   rrd = rrs + rrt;
   PC++;
}

void DADDU()
{
   rrd = rrs + rrt;
   PC++;
}

void DSUB()
{
   rrd = rrs - rrt;
   PC++;
}

void DSUBU()
{
   rrd = rrs - rrt;
   PC++;
}

void TEQ()
{
   if (rrs == rrt)
     {
	printf("trap exception in teq\n");
	stop=1;
     }
   PC++;
}

void DSLL()
{
   rrd = rrt << rsa;
   PC++;
}

void DSRL()
{
   rrd = (unsigned long long)rrt >> rsa;
   PC++;
}

void DSRA()
{
   rrd = rrt >> rsa;
   PC++;
}

void DSLL32()
{
   rrd = rrt << (32+rsa);
   PC++;
}

void DSRL32()
{
   rrd = (unsigned long long int)rrt >> (32+rsa);
   PC++;
}

void DSRA32()
{
   rrd = (signed long long int)rrt >> (32+rsa);
   PC++;
}
