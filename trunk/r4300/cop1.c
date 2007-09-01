/**
 * Mupen64 - cop1.c
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

#include "ops.h"
#include "r4300.h"
#include "macros.h"
#include "exception.h"
#include "../gc_memory/memory.h"

void MFC1()
{
   if (check_cop1_unusable()) return;
   rrt32 = *((long*)reg_cop1_simple[rfs]);
   sign_extended(rrt);
   PC++;
}

void DMFC1()
{
   if (check_cop1_unusable()) return;
   rrt = *((long long*)reg_cop1_double[rfs]);
   PC++;
}

void CFC1()
{  
   if (check_cop1_unusable()) return;
   if (rfs==31)
     {
	rrt32 = FCR31;
	sign_extended(rrt);
     }
   if (rfs==0)
     {
	rrt32 = FCR0;
	sign_extended(rrt);
     }
   PC++;
}

void MTC1()
{
   if (check_cop1_unusable()) return;
   *((long*)reg_cop1_simple[rfs]) = rrt32;
   PC++;
}

void DMTC1()
{
   if (check_cop1_unusable()) return;
   *((long long*)reg_cop1_double[rfs]) = rrt;
   PC++;
}

void CTC1()
{
   if (check_cop1_unusable()) return;
   if (rfs==31)
     FCR31 = rrt32;
   switch((FCR31 & 3))
     {
      case 0:
	rounding_mode = 0x33F;
	break;
      case 1:
	rounding_mode = 0xF3F;
	break;
      case 2:
	rounding_mode = 0xB3F;
	break;
      case 3:
	rounding_mode = 0x73F;
	break;
     }
   //if ((FCR31 >> 7) & 0x1F) printf("FPU Exception enabled : %x\n", 
//				   (int)((FCR31 >> 7) & 0x1F));
   PC++;
}
