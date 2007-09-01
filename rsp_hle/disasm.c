/**
 * Mupen64 - file_loader.c
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

#include <stdio.h>

void disasm(FILE *f, unsigned int t[0x1000/4])
{
   int i;
   
   for (i=0; i<(0x1000/4); i++)
     {
	fprintf(f,"%4x: ", i*4);
	switch((t[i] >> 26) & 0x3F)
	  {
	   case 0:
	     switch(t[i] & 0x3F)
	       {
		case 0:
		  if (t[i]) { 
		     fprintf(f,"SLL r%d, r%d, %d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 6) & 0x1F);
		  }
		  else fprintf(f,"NOP\n");
		  break;
		case 1:
		  fprintf(f,"invalid opcode\n");
		  break;
		case 2:
		  fprintf(f,"SRL r%d, r%d, %d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 6) & 0x1F);
		  break;
		case 3:
		  fprintf(f,"SRA r%d, r%d, %d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 6) & 0x1F);
		  break;
		case 4:
		  fprintf(f,"SLLV r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F);
		  break;
		case 6:
		  fprintf(f,"SRLV r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F);
		  break;
		case 7:
		  fprintf(f,"SRAV r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F);
		  break;
		case 8:
		  fprintf(f,"JR r%d\n", (t[i] >> 21) & 0x1F);
		  break;
		case 9:
		  fprintf(f,"JALR r%d\n", (t[i] >> 21) & 0x1F);
		  break;
		case 10:
		case 11:
		case 12:
		  fprintf(f, "invalid opcode\n");
		  break;
		case 13:
		  fprintf(f,"BREAK\n");
		  break;
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  fprintf(f, "invalid opcode\n");
		  break;
		case 32:
		  fprintf(f,"ADD r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 33:
		  fprintf(f,"ADDU r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 34:
		  fprintf(f,"SUB r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 35:
		  fprintf(f,"SUBU r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 36:
		  fprintf(f,"AND r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 37:
		  fprintf(f,"OR r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 38:
		  fprintf(f,"XOR r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 39:
		  fprintf(f,"NOR r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 40:
		case 41:
		  fprintf(f,"invalid opcode\n");
		  break;
		case 42:
		  fprintf(f,"SLT r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 43:
		  fprintf(f,"SLTU r%d, r%d, r%d\n", (t[i] >> 11) & 0x1F, (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F);
		  break;
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		case 60:
		case 61:
		case 62:
		case 63:
		  fprintf(f,"invalid opcode\n");
		  break;
		default:
		  fprintf(f,"opcode special inconnu: %d\n", t[i] & 0x3F);
		  return;
	       }
	     break;
	   case 1:
	     switch((t[i] >> 16) & 0x1F)
	       {
		case 0:
		  fprintf(f,"BLTZ r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
		  break;
		case 1:
		  fprintf(f,"BGEZ r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
		  break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		  fprintf(f,"invalid opcode\n");
		  break;
		case 16:
		  fprintf(f,"BLTZAL r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
		  break;
		case 17:
		  fprintf(f,"BGEZAL r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
		  break;
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  fprintf(f,"invalid opcode\n");
		  break;
		default:
		  fprintf(f,"opcode regimm inconnu: %d\n", (t[i] >> 16) & 0x1F);
		  return;
	       }
	     break;
	   case 2:
	     fprintf(f,"J %x\n", (t[i] & 0x3FF) * 4);
	     break;
	   case 3:
	     fprintf(f,"JAL %x\n", (t[i] & 0x3FF) * 4);
	     break;
	   case 4:
	     fprintf(f,"BEQ r%d, r%d, %x\n", (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
	     break;
	   case 5:
	     fprintf(f,"BNE r%d, r%d, %x\n", (t[i] >> 21) & 0x1F, (t[i] >> 16) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
	     break;
	   case 6:
	     fprintf(f,"BLEZ r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
	     break;
	   case 7:
	     fprintf(f,"BGTZ r%d, %x\n", (t[i] >> 21) & 0x1F, (i+1)*4 + (short)(t[i] & 0xFFFF)*4);
	     break;
	   case 8:
	     fprintf(f,"ADDI r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 9:
	     fprintf(f,"ADDIU r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 10:
	     fprintf(f,"SLTI r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 11:
	     fprintf(f,"SLTIU r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 12:
	     fprintf(f,"ANDI r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 13:
	     fprintf(f,"ORI r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 14:
	     fprintf(f,"XORI r%d, r%d, %d\n", (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 15:
	     fprintf(f,"LUI r%d, %d\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)));
	     break;
	   case 16:
	     switch((t[i] >> 21) & 0x1F)
	       {
		case 0:
		  fprintf(f,"MFC0 r%d, r%d\n", (t[i] >> 16) & 0x1F, (t[i] >> 11) & 0x1F);
		  break;
		case 1:
		case 2:
		case 3:
		  fprintf(f,"unknown opcode\n");
		  break;
		case 4:
		  fprintf(f,"MTC0 r%d, r%d\n", (t[i] >> 16) & 0x1F, (t[i] >> 11) & 0x1F);
		  break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  fprintf(f,"unknown opcode\n");
		  break;
		default:
		  fprintf(f,"opcode cop0 inconnu: %d\n", (t[i] >> 21) & 0x1F);
		  return;
	       }
	     break;
	   case 17:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 18:
	     switch((t[i] >> 21) & 0x1F)
	       {
		case 0:
		  fprintf(f,"MFC2\n");
		  break;
		case 1:
		  fprintf(f,"unknown opcode\n");
		  break;
		case 2:
		  fprintf(f,"CFC2\n");
		  break;
		case 3:
		  fprintf(f,"unknown opcode\n");
		  break;
		case 4:
		  fprintf(f,"MTC2 r%d, v[%d][%d]\n", (t[i] >> 16) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 6) & 0x1F);
		  break;
		case 5:
		  fprintf(f,"unknown opcode\n");
		  break;
		case 6:
		  fprintf(f, "CTC2\n");
		  break;
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		  fprintf(f,"unknown opcode\n");
		  break;
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  switch(t[i] & 0x3F)
		    {
		     case 0:
		       fprintf(f,"VMULF v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 1:
		       fprintf(f,"VMULU v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 2:
		       fprintf(f,"VRNDP v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 3:
		       fprintf(f,"VMULQ v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 4:
		       fprintf(f,"VMUDL v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 5:
		       fprintf(f,"VMUDM v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 6:
		       fprintf(f,"VMUDN v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 7:
		       fprintf(f,"VMUDH v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 8:
		       fprintf(f,"VMACF v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 9:
		       fprintf(f,"VMACU v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 10:
		       fprintf(f,"VRNDN v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 11:
		       fprintf(f,"VMACQ v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 12:
		       fprintf(f,"VMADL v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 13:
		       fprintf(f,"VMADM v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 14:
		       fprintf(f,"VMADN v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 15:
		       fprintf(f,"VMADH v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 16:
		       fprintf(f,"VADD v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 17:
		       fprintf(f,"VSUB v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 18:
		       fprintf(f,"VSUT? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 19:
		       fprintf(f,"VABS v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 20:
		       fprintf(f,"VADDC v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 21:
		       fprintf(f,"VSUBC v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 22:
		       fprintf(f,"VADDB? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 23:
		       fprintf(f,"VSUBB? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 24:
		       fprintf(f,"VACCB? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 25:
		       fprintf(f,"VSUCB? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 26:
		       fprintf(f,"VSAD? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 27:
		       fprintf(f,"VSAC? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 28:
		       fprintf(f,"VSUM? v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 29:
		       fprintf(f,"VSAW v[%d]\n", (t[i] >> 6) & 0x1F);
		       break;
		     case 30:
		     case 31:
		       fprintf(f,"unknown opcode\n");
		       break;
		     case 32:
		       fprintf(f,"VLT v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 33:
		       fprintf(f,"VEQ v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 34:
		       fprintf(f,"VNE v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 35:
		       fprintf(f,"VGE v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 36:
		       fprintf(f,"VCL v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 37:
		       fprintf(f,"VCH v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 38:
		       fprintf(f,"VCR v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 39:
		       fprintf(f,"VMRG v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 40:
		       fprintf(f,"VAND v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
			 case 41:
		       fprintf(f,"VNAND v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 42:
		       fprintf(f,"VOR v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 43:
		       fprintf(f,"VNOR v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 44:
		       fprintf(f,"VXOR v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 45:
		       fprintf(f,"VNXOR v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 46:
		     case 47:
		       fprintf(f,"unknown opcode\n");
		       break;
		     case 48:
		       fprintf(f,"VRCP\n");
		       break;
		     case 49:
		       fprintf(f,"VRCPL\n");
		       break;
		     case 50:
		       fprintf(f,"VRCPH\n");
		       break;
		     case 51:
		       fprintf(f,"VMOV v[%d], v[%d], v[%d][%d]\n", (t[i] >> 6) & 0x1F, (t[i] >> 11) & 0x1F, (t[i] >> 16) & 0x1F, (t[i] >> 21) & 0x0F);
		       break;
		     case 52:
		       fprintf(f,"VRSQ\n");
		       break;
		     case 53:
		       fprintf(f,"VRSQL\n");
		       break;
		     case 54:
		       fprintf(f,"VRSQH\n");
		       break;
		     case 55:
		     case 56:
		     case 57:
		     case 58:
		     case 59:
		     case 60:
		     case 61:
		     case 62:
		     case 63:
		       fprintf(f,"unknown opcode\n");
		       break;
		     default:
		       fprintf(f,"opcode vect inconnu: %d\n", t[i] & 0x3F);
		       return;
		    }
		  break;
		default:
		  fprintf(f,"opcode cop2 inconnu: %d\n", (t[i] >> 21) & 0x1F);
		  return;
	       }
	     break;
	   case 19:
	   case 20:
	   case 21:
	   case 22:
	   case 23:
	   case 24:
	   case 25:
	   case 26:
	   case 27:
	   case 28:
	   case 29:
	   case 30:
	   case 31:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 32:
	     fprintf(f,"LB r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 33:
	     fprintf(f,"LH r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 34:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 35:
	     fprintf(f,"LW r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 36:
	     fprintf(f,"LBU r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 37:
	     fprintf(f,"LHU r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 38:
	   case 39:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 40:
	     fprintf(f,"SB r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 41:
	     fprintf(f,"SH r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 42:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 43:
	     fprintf(f,"SW r%d, %d(r%d)\n", (t[i] >> 16) & 0x1F, (int)((short)(t[i] & 0xFFFF)), (t[i] >> 21) & 0x1F);
	     break;
	   case 44:
	   case 45:
	   case 46:
	   case 47:
	   case 48:
	   case 49:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 50:
	     switch((t[i] >> 11) & 0x1F)
	       {
		case 0:
		  fprintf(f,"LBV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 1:
		  fprintf(f,"LSV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 2:
		  fprintf(f,"LLV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 3:
		  fprintf(f,"LDV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 4:
		  fprintf(f,"LQV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 5:
		  fprintf(f,"LRV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 6:
		  fprintf(f,"LPV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 7:
		  fprintf(f,"LUV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 8:
		  fprintf(f,"LHV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 9:
		  fprintf(f,"LFV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 10:
		  fprintf(f,"LWV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 11:
		  fprintf(f,"LTV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  fprintf(f,"unknown opcode\n");
		  break;
		default:
		  fprintf(f,"opcode LWC2 inconnu: %d\n", (t[i] >> 11) & 0x1F);
		  return;
	       }
	     break;
	   case 51:
	   case 52:
	   case 53:
	   case 54:
	   case 55:
	   case 56:
	   case 57:
	     fprintf(f,"unknown opcode\n");
	     break;
	   case 58:
	     switch((t[i] >> 11) & 0x1F)
	       {
		case 0:
		  fprintf(f,"SBV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 1:
		  fprintf(f,"SSV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 2:
		  fprintf(f,"SLV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 3:
		  fprintf(f,"SDV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 4:
		  fprintf(f,"SQV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 5:
		  fprintf(f,"SRV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 6:
		  fprintf(f,"SPV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 7:
		  fprintf(f,"SUV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 8:
		  fprintf(f,"SHV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 9:
		  fprintf(f,"SFV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 10:
		  fprintf(f,"SWV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 11:
		  fprintf(f,"STV v[%d][%d], %d(r%d)\n", (t[i] >> 16) & 0x1F, (t[i] >> 7) & 0x0F, (int)(((char)((t[i] & 0x3F) << 2)) >> 2), (t[i] >> 21) & 0x1F);
		  break;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		  fprintf(f,"unknown opcode\n");
		  break;
		default:
		  fprintf(f,"opcode SWC2 inconnu: %d\n", (t[i] >> 11) & 0x1F);
		  return;
	       }
	     break;
	   case 59:
	   case 60:
	   case 61:
	   case 62:
	   case 63:
	     fprintf(f,"unknown opcode\n");
	     break;
	   default:
	     fprintf(f,"opcode inconnu: %d\n", (t[i] >> 26) & 0x3F);
	     return;
	  }
     }
}
