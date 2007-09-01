/**
 * Mupen64 - cop1_d.c
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

#include <math.h>

#include "r4300.h"
#include "ops.h"
#include "macros.h"

void ADD_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = *reg_cop1_double[cffs] +
     *reg_cop1_double[cfft];
   PC++;
}

void SUB_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = *reg_cop1_double[cffs] -
     *reg_cop1_double[cfft];
   PC++;
}

void MUL_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = *reg_cop1_double[cffs] *
     *reg_cop1_double[cfft];
   PC++;
}

void DIV_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = *reg_cop1_double[cffs] /
     *reg_cop1_double[cfft];
   PC++;
}

void SQRT_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = sqrt(*reg_cop1_double[cffs]);
   PC++;
}

void ABS_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = fabs(*reg_cop1_double[cffs]);
   PC++;
}

void MOV_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = *reg_cop1_double[cffs];
   PC++;
}

void NEG_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_double[cffd] = -(*reg_cop1_double[cffs]);
   PC++;
}

void ROUND_L_D()
{
   if (check_cop1_unusable()) return;
   set_round();
   *((long long*)(reg_cop1_double[cffd])) = *reg_cop1_double[cffs];
   PC++;
}

void TRUNC_L_D()
{
   if (check_cop1_unusable()) return;
   set_trunc();
   *((long long*)(reg_cop1_double[cffd])) = *reg_cop1_double[cffs];
   PC++;
}

void CEIL_L_D()
{
   if (check_cop1_unusable()) return;
   set_ceil();
   *((long long*)(reg_cop1_double[cffd])) = *reg_cop1_double[cffs];
   PC++;
}

void FLOOR_L_D()
{
   if (check_cop1_unusable()) return;
   set_floor();
   *((long long*)(reg_cop1_double[cffd])) = *reg_cop1_double[cffs];
   PC++;
}

void ROUND_W_D()
{
   if (check_cop1_unusable()) return;
   set_round();
   *((long*)reg_cop1_simple[cffd]) = *reg_cop1_double[cffs];
   PC++;
}

void TRUNC_W_D()
{
   if (check_cop1_unusable()) return;
   set_trunc();
   *((long*)reg_cop1_simple[cffd]) = *reg_cop1_double[cffs];
   PC++;
}

void CEIL_W_D()
{
   if (check_cop1_unusable()) return;
   set_ceil();
   *((long*)reg_cop1_simple[cffd]) = *reg_cop1_double[cffs];
   PC++;
}

void FLOOR_W_D()
{
   if (check_cop1_unusable()) return;
   set_floor();
   *((long*)reg_cop1_simple[cffd]) = *reg_cop1_double[cffs];
   PC++;
}

void CVT_S_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *reg_cop1_simple[cffd] = *reg_cop1_double[cffs];
   PC++;
}

void CVT_W_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *((long*)reg_cop1_simple[cffd]) = *reg_cop1_double[cffs];
   PC++;
}

void CVT_L_D()
{
   if (check_cop1_unusable()) return;
   set_rounding();
   *((long long*)(reg_cop1_double[cffd])) = *reg_cop1_double[cffs];
   PC++;
}

void C_F_D()
{
   if (check_cop1_unusable()) return;
   FCR31 &= ~0x800000;
   PC++;
}

void C_UN_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_EQ_D()
{
   if (check_cop1_unusable()) return;
   if (!isnan(*reg_cop1_double[cffs]) && !isnan(*reg_cop1_double[cfft]) &&
       *reg_cop1_double[cffs] == *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_UEQ_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]) ||
       *reg_cop1_double[cffs] == *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_OLT_D()
{
   if (check_cop1_unusable()) return;
   if (!isnan(*reg_cop1_double[cffs]) && !isnan(*reg_cop1_double[cfft]) &&
       *reg_cop1_double[cffs] < *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_ULT_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]) ||
       *reg_cop1_double[cffs] < *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_OLE_D()
{
   if (check_cop1_unusable()) return;
   if (!isnan(*reg_cop1_double[cffs]) && !isnan(*reg_cop1_double[cfft]) &&
       *reg_cop1_double[cffs] <= *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_ULE_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]) ||
       *reg_cop1_double[cffs] <= *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_SF_D()
{
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   FCR31 &= ~0x800000;
   PC++;
}

void C_NGLE_D()
{
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   FCR31 &= ~0x800000;
   PC++;
}

void C_SEQ_D()
{
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] == *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_NGL_D()
{
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] == *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_LT_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] < *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_NGE_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] < *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_LE_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] <= *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}

void C_NGT_D()
{
   if (check_cop1_unusable()) return;
   if (isnan(*reg_cop1_double[cffs]) || isnan(*reg_cop1_double[cfft]))
     {
	printf("Invalid operation exception in C opcode\n");
	stop=1;
     }
   if (*reg_cop1_double[cffs] <= *reg_cop1_double[cfft])
     FCR31 |= 0x800000;
   else FCR31 &= ~0x800000;
   PC++;
}
