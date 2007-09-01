/*
 * debugger/decoder.c
 * 
 * Debugger for Mupen64 - davFr
 * Copyright (C) 2002 davFr - robind@esiee.fr
 *
 * Mupen64 is copyrighted (C) 2002 Hacktarux
 * Mupen64 homepage: http://mupen64.emulation64.com
 *         email address: hacktarux@yahoo.fr
 * 
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence.
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

#include "decoder.h"

static long int	mot;
static char *op;
static char *args;





static void RESERV(){
	sprintf(op, "[%.2lX] RESERV: Instruction Inconnue => Mail Us!",(mot>>26)&0x3F);
	sprintf(args, "0x%.8lX",mot);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ SPECIAL ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void SLL()
{
	if(mot==0){
		sprintf(op, "[00] NOP");
		sprintf(args, " ");
	}
	else{
		sprintf(op, "[00] SLL");
		sprintf(args, "reg%ld, reg%ld, %ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
	}
}

static void SRL(){
	sprintf(op, "[00] SRL");
	sprintf(args, "reg%ld, reg%ld, %ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void SRA(){
	sprintf(op, "[00] SRA");
	sprintf(args, "reg%ld, reg%ld, %ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void SLLV(){
	sprintf(op, "[00] SLLV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>21)&0x1F);
}

static void SRLV(){
	sprintf(op, "[00] SRLV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>21)&0x1F);
}

static void SRAV(){
	sprintf(op, "[00] SRAV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>21)&0x1F);

}

static void JR(){
	sprintf(op, "[00] JR");
	sprintf(args, "reg%ld", (mot>>21)&0x1F);
}

static void JALR(){
	sprintf(op, "[00] JALR");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>11)&0x1F);
}

static void SYSCALL(){
	sprintf(op, "[00] SYSCALL");
	sprintf(args, "0x%.5lX", (mot>>6)&0xFFFFF);
}

static void BREAK(){
	sprintf(op, "[00] BREAK");
	sprintf(args, "0x%.5lX", (mot>>6)&0xFFFFF);
}

static void SYNC(){
	sprintf(op, "[00] SYNC");
	sprintf(args, "%ld", (mot>>6)&0x1F);
}

static void MFHI(){
	sprintf(op, "[00] MFHI");
	sprintf(args, "reg%ld", (mot>>11)&0x1F);
}

static void MTHI(){
	sprintf(op, "[00] MTHI");
	sprintf(args, "reg%ld", (mot>>21)&0x1F);
}

static void MFLO(){
	sprintf(op, "[00] MFLO");
	sprintf(args, "reg%ld", (mot>>11)&0x1F);
}

static void MTLO(){
	sprintf(op, "[00] MTLO");
	sprintf(args, "reg%ld", (mot>>21)&0x1F);
}

static void DSLLV(){
	sprintf(op, "[00] DSLLV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>21)&0x1F);
}

static void DSRLV(){
	sprintf(op, "[00] DSRLV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSRAV(){
	sprintf(op, "[00] DSRAV");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void MULT(){
	sprintf(op, "[00] MULT");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void MULTU(){
	sprintf(op, "[00] MULTU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DIV(){
	sprintf(op, "[00] DIV");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DIVU(){
	sprintf(op, "[00] DIVU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DMULT(){
	sprintf(op, "[00] DMULTU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DMULTU(){
	sprintf(op, "[00] DMULTU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DDIV(){
	sprintf(op, "[00] DDIV");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DDIVU(){
	sprintf(op, "[00] DDIVU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void ADD(){
	sprintf(op, "[00] ADD");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void ADDU(){
	sprintf(op, "[00] ADDU");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void SUB()
{
	if ((mot>>16)&0x1F){
		sprintf(op, "[00] SUB");
		sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
	}
	else{
		sprintf(op, "[00] NEG");
		sprintf(args, "reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
	}
}

static void SUBU(){
	if ((mot>>16)&0x1F){
		sprintf(op, "[00] SUBU");
		sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
	}
	else{
		sprintf(op, "[00] NEGU");
		sprintf(args, "reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
	}
}

static void AND(){
	sprintf(op, "[00] AND");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void OR(){
	sprintf(op, "[00] OR");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void XOR(){
	sprintf(op, "[00] XOR");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void NOR(){
	sprintf(op, "[00] NOR");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void SLT(){
	sprintf(op, "[00] SLT");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void SLTU(){
	sprintf(op, "[00] SLTU");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DADD(){
	sprintf(op, "[00] DADD");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DADDU(){
	sprintf(op, "[00] DADDU");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DSUB(){
	sprintf(op, "[00] DSUB");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DSUBU(){
	sprintf(op, "[00] DSUBU");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TGE(){
	sprintf(op, "[00] TGE");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TGEU(){
	sprintf(op, "[00] TGEU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TLT(){
	sprintf(op, "[00] TLT");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TLTU(){
	sprintf(op, "[00] TLTU");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TEQ(){
	sprintf(op, "[00] TEQ");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void TNE(){
	sprintf(op, "[00] TNE");
	sprintf(args, "reg%ld, reg%ld", (mot>>21)&0x1F, (mot>>16)&0x1F);
}

static void DSLL(){
	sprintf(op, "[00] DSLL");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSRL(){
	sprintf(op, "[00] DSRL");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSRA(){
	sprintf(op, "[00] DSRA");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSLL32(){
	sprintf(op, "[00] DSLL32");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSRL32(){
	sprintf(op, "[00] DSRL32");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void DSRA32(){
	sprintf(op, "[00] DSRA32");
	sprintf(args, "reg%ld, reg%ld, reg%ld", (mot>>11)&0x1F, (mot>>16)&0x1F, (mot>>6)&0x1F);
}

static void special()
{
//	sprintf(op, "[00] SPECIAL");
	
	switch( mot & 0x3F)
	{
		case 0x00: SLL();	break;
		case 0x02: SRL();	break;
		case 0x03: SRA();	break;
		case 0x04: SLLV();	break;
		case 0x06: SRLV();	break;
		case 0x07: SRAV();	break;
		case 0x08: JR();	break;
		case 0x09: JALR();	break;
		case 0x0C: SYSCALL();	break;
		case 0x0D: BREAK();	break;
		case 0x0F: SYNC();	break;
		case 0x10: MFHI();	break;
		case 0x11: MTHI();	break;
		case 0x12: MFLO();	break;
		case 0x13: MTLO();	break;
		case 0x14: DSLLV();	break;
		case 0x16: DSRLV();	break;
		case 0x17: DSRAV();	break;
		case 0x18: MULT();	break;
		case 0x19: MULTU();	break;
		case 0x1A: DIV();	break;
		case 0x1B: DIVU();	break;
		case 0x1C: DMULT();	break;
		case 0x1D: DMULTU();	break;
		case 0x1E: DDIV();	break;
		case 0x1F: DDIVU();	break;
		case 0x20: ADD();	break;
		case 0x21: ADDU();	break;
		case 0x22: SUB();	break;
		case 0x23: SUBU();	break;
		case 0x24: AND();	break;
		case 0x25: OR();	break;
		case 0x26: XOR();	break;
		case 0x27: NOR();	break;
		case 0x2A: SLT();	break;
		case 0x2B: SLTU();	break;
		case 0x2C: DADD();	break;
		case 0x2D: DADDU();	break;
		case 0x2E: DSUB();	break;
		case 0x2F: DSUBU();	break;
		case 0x30: TGE();	break;
		case 0x31: TGEU();	break;
		case 0x32: TLT();	break;
		case 0x33: TLTU();	break;
		case 0x34: TEQ();	break;
		case 0x36: TNE();	break;
		case 0x38: DSLL();	break;
		case 0x3A: DSRL();	break;
		case 0x3B: DSRA();	break;
		case 0x3C: DSLL32();	break;
		case 0x3E: DSRL32();	break;
		case 0x3F: DSRA32();	break;
		default :  RESERV(); //just to be sure
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ REGIMM ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void BLTZ(){
	sprintf(op, "[01] BLTZ");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGEZ(){
	sprintf(op, "[01] BGEZ");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BLTZL(){
	sprintf(op, "[01] BLTZL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGEZL(){
	sprintf(op, "[01] BGEZL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TGEI(){
	sprintf(op, "[01] TGEI");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TGEIU(){
	sprintf(op, "[01] TGEIU");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TLTI(){
	sprintf(op, "[01] TLTI");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TLTIU(){
	sprintf(op, "[01] TLTIU");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TEQI(){
	sprintf(op, "[01] TEQI");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void TNEI(){
	sprintf(op, "[01] TNEI");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BLTZAL(){
	sprintf(op, "[01] BLTZAL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGEZAL(){
	sprintf(op, "[01] BGEZAL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BLTZALL(){
	sprintf(op, "[01] BLTZALL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGEZALL(){
	sprintf(op, "[01] BGEZALL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void regimm()
{
//	sprintf(op, "[01] REGIMM");
	
	switch( (mot>>16) & 0x1F)
	{
		case 0x00: BLTZ();	break;
		case 0x01: BGEZ();	break;
		case 0x02: BLTZL();	break;
		case 0x03: BGEZL();	break;
		case 0x08: TGEI();	break;
		case 0x09: TGEIU();	break;
		case 0x0A: TLTI();	break;
		case 0x0B: TLTIU();	break;
		case 0x0C: TEQI();	break;
		case 0x0E: TNEI();	break;
		case 0x10: BLTZAL();	break;
		case 0x11: BGEZAL();	break;
		case 0x12: BLTZALL();	break;
		case 0x13: BGEZALL();	break;
		default: RESERV();
		}
}


//]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ ... ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[

static void J(){
	sprintf(op, "[02] J");
	sprintf(args, "0x%lX", mot&0x3FFFFFF);
}

static void JAL(){
	sprintf(op, "[03] JAL");
	sprintf(args, "0x%lX", mot&0x3FFFFFF);
}

static void BEQ(){
	sprintf(op, "[04] BEQ");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>21)&0x1F, (mot>>16)&0x1F, mot&0xFFFF);
}

static void BNE(){
	sprintf(op, "[05] BNE");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>21)&0x1F, (mot>>16)&0x1F, mot&0xFFFF);
}

static void BLEZ(){
	sprintf(op, "[06] BLEZ");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGTZ(){
	sprintf(op, "[07] BGTZ");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void ADDI(){
	sprintf(op, "[08] ADDI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void ADDIU(){
	sprintf(op, "[09] ADDIU");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void SLTI(){
	sprintf(op, "[0A] SLTI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void SLTIU(){
	sprintf(op, "[0B] SLTIU");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void ANDI(){
	sprintf(op, "[0C] ANDI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void ORI(){
	sprintf(op, "[0D] ORI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void XORI(){
	sprintf(op, "[0E] XORI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void LUI(){
	sprintf(op, "[0F] LUI");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>16)&0x1F, mot&0xFFFF);
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ cop0 ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void MFC0(){
	sprintf(op, "[10] MFC0");
	sprintf(args, "reg%ld, reg%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}	

static void MTC0(){
	sprintf(op, "[10] MTC0");
	sprintf(args, "reg%ld, reg%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ tlb ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void TLBR(){
	sprintf(op, "[10] TLBR");
	sprintf(args, " ");
}	
	
static void TLBWI(){
	sprintf(op, "[10] TLBWI");
	sprintf(args, " ");
}	

static void TLBWR(){
	sprintf(op, "[10] TLBWR");
	sprintf(args, " ");
}	

static void TLBP(){
	sprintf(op, "[10] TLBP");
	sprintf(args, " ");
}	

static void ERET(){
	sprintf(op, "[10] ERET");
	sprintf(args, " ");
}	

static void tlb()
{
//	sprintf(op, "[10] tlb");
	
	switch( mot & 0x3F)
	{
		case 0x01: TLBR()	; break;
		case 0x02: TLBWI()	; break;
		case 0x06: TLBWR()	; break;
		case 0x08: TLBP()	; break;
		case 0x18: ERET()	; break;
		default: RESERV();
	}
}

static void cop0()
{
//	sprintf(op, "[10] COP0");
	
	switch( (mot>>21) & 0x1F)
	{
		case 0x00: MFC0()	; break;
		case 0x04: MTC0()	; break;
		case 0x10: tlb()	; break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ cop1 ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void MFC1(){
	sprintf(op, "[11] MFC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}

static void DMFC1(){
	sprintf(op, "[11] DMFC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}

static void CFC1(){
	sprintf(op, "[11] CFC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}

static void MTC1(){
	sprintf(op, "[11] MTC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}

static void DMTC1(){
	sprintf(op, "[11] DMTC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}

static void CTC1(){
	sprintf(op, "[11] CTC1");
	sprintf(args, "reg%ld, fgr%ld", (mot>>16)&0x1F, (mot>>11)&0x1F);
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ BC ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void BC1F(){
	sprintf(op, "[11] BC1F");
	sprintf(args, "0x%.4lX", mot&0xFFFF);
}

static void BC1T(){
	sprintf(op, "[11] BC1T");
	sprintf(args, "0x%.4lX", mot&0xFFFF);
	
}

static void BC1FL(){
	sprintf(op, "[11] BC1FL");
	sprintf(args, "0x%.4lX", mot&0xFFFF);
	
}

static void BC1TL(){
	sprintf(op, "[11] BC1TL");
	sprintf(args, "0x%.4lX", mot&0xFFFF);
}

static void BC()
{
//	sprintf(op, "[11] BC");

	switch( (mot>>16) & 3)
	{
		case 0x00: BC1F();	break;
		case 0x01: BC1T();	break;
		case 0x02: BC1FL();	break;
		case 0x03: BC1TL();	break;
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ S ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void ADD_S(){
	sprintf(op, "[11] ADD.S");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void SUB_S(){
	sprintf(op, "[11] SUB.S");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void MUL_S(){
	sprintf(op, "[11] MUL.S");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void DIV_S(){
	sprintf(op, "[11] DIV.S");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void SQRT_S(){
	sprintf(op, "[11] SQRT.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ABS_S(){
	sprintf(op, "[11] ABS.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void MOV_S(){
	sprintf(op, "[11] MOV.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void NEG_S(){
	sprintf(op, "[11] NEG.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ROUND_L_S(){
	sprintf(op, "[11]  ROUND.L.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void TRUNC_L_S(){
	sprintf(op, "[11] TRUNC.L.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CEIL_L_S(){
	sprintf(op, "[11] CEIL.L.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void FLOOR_L_S(){
	sprintf(op, "[11] FLOOR.L.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ROUND_W_S(){
	sprintf(op, "[11] ROUND.W.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void TRUNC_W_S(){
	sprintf(op, "[11] TRUNC.W.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CEIL_W_S(){
	sprintf(op, "[11] CEIL.W.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void FLOOR_W_S(){
	sprintf(op, "[11] FLOOR.W.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_D_S(){
	sprintf(op, "[11] CVT.D.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_W_S(){
	sprintf(op, "[11] CVT.W.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_L_S(){
	sprintf(op, "[11] CVT.L.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void C_F_S(){
	sprintf(op, "[11] C.F.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_UN_S(){
	sprintf(op, "[11] C.UN.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_EQ_S(){
	sprintf(op, "[11] C.EQ.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_UEQ_S(){
	sprintf(op, "[11] C.UEQ.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_OLT_S(){
	sprintf(op, "[11] C.OLT.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_ULT_S(){
	sprintf(op, "[11] C.ULT.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_OLE_S(){
	sprintf(op, "[11] C.OLE.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_ULE_S(){
	sprintf(op, "[11] C.ULE.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_SF_S(){
	sprintf(op, "[11] C.SF.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGLE_S(){
	sprintf(op, "[11] C.NGLE.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_SEQ_S(){
	sprintf(op, "[11] C.SEQ.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGL_S(){
	sprintf(op, "[11] C.NGL.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_LT_S(){
	sprintf(op, "[11] C.LT.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGE_S(){
	sprintf(op, "[11] C.NGE.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_LE_S(){
	sprintf(op, "[11] C.LE.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGT_S(){
	sprintf(op, "[11] C.NGT.S");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}


static void S()
{
//	sprintf(op, "[11] S");
	
	switch( mot & 0x3F)
	{
		case 0x00: ADD_S();	break;
		case 0x01: SUB_S();	break;
		case 0x02: MUL_S();	break;
		case 0x03: DIV_S();	break;
		case 0x04: SQRT_S();	break;
		case 0x05: ABS_S();	break;
		case 0x06: MOV_S();	break;
		case 0x07: NEG_S();	break;
		case 0x08: ROUND_L_S();	break;
		case 0x09: TRUNC_L_S();	break;
		case 0x0A: CEIL_L_S();	break;
		case 0x0B: FLOOR_L_S();	break;
		case 0x0C: ROUND_W_S();	break;
		case 0x0D: TRUNC_W_S();	break;
		case 0x0E: CEIL_W_S();	break;
		case 0x0F: FLOOR_W_S();	break;
		case 0x21: CVT_D_S();	break;
		case 0x24: CVT_W_S();	break;
		case 0x25: CVT_L_S();	break;
		case 0x30: C_F_S();	break;
		case 0x31: C_UN_S();	break;
		case 0x32: C_EQ_S();	break;
		case 0x33: C_UEQ_S();	break;
		case 0x34: C_OLT_S();	break;
		case 0x35: C_ULT_S();	break;
		case 0x36: C_OLE_S();	break;
		case 0x37: C_ULE_S();	break;
		case 0x38: C_SF_S();	break;
		case 0x39: C_NGLE_S();	break;
		case 0x3A: C_SEQ_S();	break;
		case 0x3B: C_NGL_S();	break;
		case 0x3C: C_LT_S();	break;
		case 0x3D: C_NGE_S();	break;
		case 0x3E: C_LE_S();	break;
		case 0x3F: C_NGT_S();	break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ D ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//


static void ADD_D(){
	sprintf(op, "[11] ADD.D");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void SUB_D(){
	sprintf(op, "[11] SUB.D");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void MUL_D(){
	sprintf(op, "[11] MUL.D");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void DIV_D(){
	sprintf(op, "[11] DIV.D");
	sprintf(args, "fgr%ld, fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void SQRT_D(){
	sprintf(op, "[11] ADD.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ABS_D(){
	sprintf(op, "[11] ADD.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void MOV_D(){
	sprintf(op, "[11] MOV.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void NEG_D(){
	sprintf(op, "[11] NEG.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ROUND_L_D(){
	sprintf(op, "[11]  ROUND.L.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void TRUNC_L_D(){
	sprintf(op, "[11] TRUNC.L.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CEIL_L_D(){
	sprintf(op, "[11] CEIL.L.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void FLOOR_L_D(){
	sprintf(op, "[11] FLOOR.L.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void ROUND_W_D(){
	sprintf(op, "[11] ROUND.W.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void TRUNC_W_D(){
	sprintf(op, "[11] TRUNC.W.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CEIL_W_D(){
	sprintf(op, "[11] CEIL.W.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void FLOOR_W_D(){
	sprintf(op, "[11] FLOOR.W.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_S_D(){
	sprintf(op, "[11] CVT.S.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_W_D(){
	sprintf(op, "[11] CVT.W.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_L_D(){
	sprintf(op, "[11] CVT.L.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void C_F_D(){
	sprintf(op, "[11] C.F.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_UN_D(){
	sprintf(op, "[11] C.UN.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_EQ_D(){
	sprintf(op, "[11] C.EQ.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_UEQ_D(){
	sprintf(op, "[11] C.UEQ.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_OLT_D(){
	sprintf(op, "[11] C.OLT.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_ULT_D(){
	sprintf(op, "[11] C.ULT.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_OLE_D(){
	sprintf(op, "[11] C.OLE.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_ULE_D(){
	sprintf(op, "[11] C.ULE.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_SF_D(){
	sprintf(op, "[11] C.SF.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGLE_D(){
	sprintf(op, "[11] C.NGLE.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_SEQ_D(){
	sprintf(op, "[11] C.SEQ.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGL_D(){
	sprintf(op, "[11] C.NGL.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_LT_D(){
	sprintf(op, "[11] C.LT.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGE_D(){
	sprintf(op, "[11] C.NGE.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_LE_D(){
	sprintf(op, "[11] C.LE.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}

static void C_NGT_D(){
	sprintf(op, "[11] C.NGT.D");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>11)&0x1F, (mot>>16)&0x1F);
}


static void D()
{
//	sprintf(op, "[11] D");

	switch( mot & 0x3F)
	{
		case 0x00: ADD_D();	break;
		case 0x01: SUB_D();	break;
		case 0x02: MUL_D();	break;
		case 0x03: DIV_D();	break;
		case 0x04: SQRT_D();	break;
		case 0x05: ABS_D();	break;
		case 0x06: MOV_D();	break;
		case 0x07: NEG_D();	break;
		case 0x08: ROUND_L_D();	break;
		case 0x09: TRUNC_L_D();	break;
		case 0x0A: CEIL_L_D();	break;
		case 0x0B: FLOOR_L_D();	break;
		case 0x0C: ROUND_W_D();	break;
		case 0x0D: TRUNC_W_D();	break;
		case 0x0E: CEIL_W_D();	break;
		case 0x0F: FLOOR_W_D();	break;
		case 0x20: CVT_S_D();	break;
		case 0x24: CVT_W_D();	break;
		case 0x25: CVT_L_D();	break;
		case 0x30: C_F_D();	break;
		case 0x31: C_UN_D();	break;
		case 0x32: C_EQ_D();	break;
		case 0x33: C_UEQ_D();	break;
		case 0x34: C_OLT_D();	break;
		case 0x35: C_ULT_D();	break;
		case 0x36: C_OLE_D();	break;
		case 0x37: C_ULE_D();	break;
		case 0x38: C_SF_D();	break;
		case 0x39: C_NGLE_D();	break;
		case 0x3A: C_SEQ_D();	break;
		case 0x3B: C_NGL_D();	break;
		case 0x3C: C_LT_D();	break;
		case 0x3D: C_NGE_D();	break;
		case 0x3E: C_LE_D();	break;
		case 0x3F: C_NGT_D();	break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ W ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void CVT_S_W(){
	sprintf(op, "[11] CVT.S.W");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_D_W(){
	sprintf(op, "[11] CVT.D.W");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}


static void W()
{
//	sprintf(op, "[11] W");
//	sprintf(args, " ");
	
	switch( mot & 0x3F)
	{
		case 0x20: CVT_S_W();	break;
		case 0x21: CVT_D_W();	break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ L ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void CVT_S_L(){
	sprintf(op, "[11] CVT.S.L");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}

static void CVT_D_L(){
	sprintf(op, "[11] CVT.D.L");
	sprintf(args, "fgr%ld, fgr%ld", (mot>>6)&0x1F, (mot>>11)&0x1F);
}


static void L(){
//	sprintf(op, "[11] L");
//	sprintf(args, " ");

	switch( mot & 0x3F)
	{
		case 0x20: CVT_S_L();	break;
		case 0x21: CVT_D_L();	break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ cop1 ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void cop1()
{
//	sprintf(op, "[11] COP1");
	
	switch( (mot>>21) & 0x1F)
	{
		case 0x00: MFC1();	break;
		case 0x01: DMFC1();	break;
		case 0x02: CFC1();	break;
		case 0x04: MTC1();	break;
		case 0x05: DMTC1();	break;
		case 0x06: CTC1();	break;
		case 0x08: BC();	break;
		case 0x10: S();		break;
		case 0x11: D(); 	break;
		case 0x14: W(); 	break;
		case 0x15: L(); 	break;
		default: RESERV();
	}
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ ... ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//

static void BEQL(){
	sprintf(op, "[14] BEQL");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>21)&0x1F, (mot>>16)&0x1F, mot&0xFFFF);
}

static void BNEL(){
	sprintf(op, "[15] BNEL");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>21)&0x1F, (mot>>16)&0x1F, mot&0xFFFF);
}

static void BLEZL(){
	sprintf(op, "[16] BLEZL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void BGTZL(){
	sprintf(op, "[17] BGTZL");
	sprintf(args, "reg%ld, 0x%.4lX", (mot>>21)&0x1F, mot&0xFFFF);
}

static void DADDI(){
	sprintf(op, "[18] DADDI");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void DADDIU(){
	sprintf(op, "[19] DADDIU");
	sprintf(args, "reg%ld, reg%ld, 0x%.4lX", (mot>>16)&0x1F, (mot>>21)&0x1F, mot&0xFFFF);
}

static void LDL(){
	sprintf(op, "[1A] LDL");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LDR(){
	sprintf(op, "[1B] LRD");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LB(){
	sprintf(op, "[20] LB");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LH(){
	sprintf(op, "[21] LH");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LWL(){
	sprintf(op, "[22] LWL");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LW(){
	sprintf(op, "[23] LW");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LBU(){
	sprintf(op, "[24] LBU");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LHU(){
	sprintf(op, "[25] LHU");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LWR(){
	sprintf(op, "[26] LWR");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LWU(){
	sprintf(op, "[27] LWU");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SB(){
	sprintf(op, "[28] SB");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SH(){
	sprintf(op, "[29] SH");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SWL(){
	sprintf(op, "[2A] SWL");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SW(){
	sprintf(op, "[2B] SW");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SDL(){
	sprintf(op, "[2C] SDL");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SDR(){
	sprintf(op, "[2D] SDR");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SWR(){
	sprintf(op, "[2E] SWR");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void CACHE(){
	sprintf(op, "[2F] CACHE");
	sprintf(args, "op%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
	//Creuser "op"
}

static void LL(){
	sprintf(op, "[30] LL");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LWC1(){
	sprintf(op, "[31] LWC1");
	sprintf(args, "fgr%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LLD(){
	sprintf(op, "[34] LLD");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LDC1(){
	sprintf(op, "[35] LDC1");
	sprintf(args, "fgr%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void LD(){
	sprintf(op, "[37] LD");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SC(){
	sprintf(op, "[38] SC");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SWC1(){
	sprintf(op, "[39] SWC1");
	sprintf(args, "fgr%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SCD(){
	sprintf(op, "[3C] SCD");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SDC1(){
	sprintf(op, "[3D] SDC1");
	sprintf(args, "fgr%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}

static void SD(){
	sprintf(op, "[3F] SD");
	sprintf(args, "reg%ld, 0x%.4lX(reg%ld)", (mot>>16)&0x1F, mot&0xFFFF, (mot>>21)&0x1F);
}



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[ DECODE_OP ]=-=-=-=-=-=-=-=-=-=-=-=-=-=-=[//


void decode_op( uint32 instr, char *opcode, char *arguments )
{
	mot = instr;
	op = opcode;
	args = arguments;
	
	switch((mot>>26)&0x3F)
	{
		case 0x00: special();	break;
		case 0x01: regimm();	break;
		case 0x02: J();		break;
		case 0x03: JAL();	break;
		case 0x04: BEQ();	break;
		case 0x05: BNE();	break;
		case 0x06: BLEZ();	break;
		case 0x07: BGTZ();	break;
		case 0x08: ADDI();	break;
		case 0x09: ADDIU();	break;
		case 0x0A: SLTI();	break;
		case 0x0B: SLTIU();	break;
		case 0x0C: ANDI();	break;
		case 0x0D: ORI();	break;
		case 0x0E: XORI();	break;
		case 0x0F: LUI();	break;
		case 0x10: cop0();	break;
		case 0x11: cop1();	break;
		case 0x14: BEQL();	break;
		case 0x15: BNEL();	break;
		case 0x16: BLEZL();	break;
		case 0x17: BGTZL();	break;
		case 0x18: DADDI();	break;
		case 0x19: DADDIU();	break;
		case 0x1A: LDL();	break;
		case 0x1B: LDR();	break;
		case 0x20: LB();	break;
		case 0x21: LH();	break;
		case 0x22: LWL();	break;
		case 0x23: LW();	break;
		case 0x24: LBU();	break;
		case 0x25: LHU();	break;
		case 0x26: LWR();	break;
		case 0x27: LWU();	break;
		case 0x28: SB();	break;
		case 0x29: SH();	break;
		case 0x2A: SWL();	break;
		case 0x2B: SW();	break;
		case 0x2C: SDL();	break;
		case 0x2D: SDR();	break;
		case 0x2E: SWR();	break;
		case 0x2F: CACHE();	break;
		case 0x30: LL();	break;
		case 0x31: LWC1();	break;
		case 0x34: LLD();	break;
		case 0x35: LDC1();	break;
		case 0x37: LD();	break;
		case 0x38: SC();	break;
		case 0x39: SWC1();	break;
		case 0x3C: SCD();	break;
		case 0x3D: SDC1();	break;
		case 0x3F: SD();	break;
		default: RESERV();
	}
}
