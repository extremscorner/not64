/**
 * Mupen64 - savestates.c
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

#include <zlib.h>
#include <stdlib.h>
#include <string.h>

#include "savestates.h"
#include "guifuncs.h"
#include "rom.h"
#include "../memory/memory.h"
#include "../memory/flashram.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"

extern unsigned long interp_addr;
extern int *autoinc_save_slot;

int savestates_job = 0;

static unsigned int slot = 0;
static char fname[1024];

void savestates_select_slot(unsigned int s)
{
   if (s > 9) return;
   slot = s;
}

void savestates_select_filename(unsigned char *fn)
{
   slot += 10;
   if (strlen(fn) >= 1024) return;
   strcpy(fname, fn);
}

void savestates_save()
{
   char *filename, buf[1024];
   gzFile f;
   int len, i;
   
   if (*autoinc_save_slot)
     {
	if (++slot == 10)
	  {
	     slot = 0;
	  }
     }
   
   if (slot <= 9)
     {
	filename = malloc(strlen(get_savespath())+
			  strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, get_savespath());
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".st");
	sprintf(buf, "%d", slot);
	strcat(filename, buf);
     }
   else
     {
	filename = malloc(strlen(fname)+1);
	strcpy(filename, fname);
	slot -= 10;
     }
   	
   f = gzopen(filename, "wb");
   free(filename);
   
   gzwrite(f, ROM_SETTINGS.MD5, 32);
   
   gzwrite(f, &rdram_register, sizeof(RDRAM_register));
   gzwrite(f, &MI_register, sizeof(mips_register));
   gzwrite(f, &pi_register, sizeof(PI_register));
   gzwrite(f, &sp_register, sizeof(SP_register));
   gzwrite(f, &rsp_register, sizeof(RSP_register));
   gzwrite(f, &si_register, sizeof(SI_register));
   gzwrite(f, &vi_register, sizeof(VI_register));
   gzwrite(f, &ri_register, sizeof(RI_register));
   gzwrite(f, &ai_register, sizeof(AI_register));
   gzwrite(f, &dpc_register, sizeof(DPC_register));
   gzwrite(f, &dps_register, sizeof(DPS_register));
   gzwrite(f, rdram, 0x800000);
   gzwrite(f, SP_DMEM, 0x1000);
   gzwrite(f, SP_IMEM, 0x1000);
   gzwrite(f, PIF_RAM, 0x40);

   save_flashram_infos(buf);
   gzwrite(f, buf, 24);
   
   gzwrite(f, tlb_LUT_r, 0x100000);
   gzwrite(f, tlb_LUT_w, 0x100000);
   
   gzwrite(f, &llbit, 4);
   gzwrite(f, reg, 32*8);
   for (i=0; i<32; i++) gzwrite(f, reg_cop0+i, 8); // *8 for compatibility with old versions purpose
   gzwrite(f, &lo, 8);
   gzwrite(f, &hi, 8);
   gzwrite(f, reg_cop1_fgr_64, 32*8);
   gzwrite(f, &FCR0, 4);
   gzwrite(f, &FCR31, 4);
   gzwrite(f, tlb_e, 32*sizeof(tlb));
   if (!dynacore && interpcore) gzwrite(f, &interp_addr, 4);
   else gzwrite(f, &PC->addr, 4);
   
   gzwrite(f, &next_interupt, 4);
   gzwrite(f, &next_vi, 4);
   gzwrite(f, &vi_field, 4);
   
   len = save_eventqueue_infos(buf);
   gzwrite(f, buf, len);
   
   gzclose(f);
}

void savestates_load()
{
   char *filename, buf[1024];
   gzFile f;
   int len, i;
   
   if (slot <= 9)
     {
	filename = malloc(strlen(get_savespath())+
			  strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, get_savespath());
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".st");
	sprintf(buf, "%d", slot);
	strcat(filename, buf);
     }
   else
     {
	filename = malloc(strlen(fname)+1);
	strcpy(filename, fname);
	slot -= 10;
     }
   f = gzopen(filename, "rb");
   free(filename);
   
   if (f == NULL)
     {
	warn_savestate_not_exist();
	return;
     }
   
   gzread(f, buf, 32);
   if (memcmp(buf, ROM_SETTINGS.MD5, 32))
     {
	warn_savestate_from_another_rom();
	gzclose(f);
	return;
     }
   
   gzread(f, &rdram_register, sizeof(RDRAM_register));
   gzread(f, &MI_register, sizeof(mips_register));
   gzread(f, &pi_register, sizeof(PI_register));
   gzread(f, &sp_register, sizeof(SP_register));
   gzread(f, &rsp_register, sizeof(RSP_register));
   gzread(f, &si_register, sizeof(SI_register));
   gzread(f, &vi_register, sizeof(VI_register));
   gzread(f, &ri_register, sizeof(RI_register));
   gzread(f, &ai_register, sizeof(AI_register));
   gzread(f, &dpc_register, sizeof(DPC_register));
   gzread(f, &dps_register, sizeof(DPS_register));
   gzread(f, rdram, 0x800000);
   gzread(f, SP_DMEM, 0x1000);
   gzread(f, SP_IMEM, 0x1000);
   gzread(f, PIF_RAM, 0x40);

   gzread(f, buf, 24);
   load_flashram_infos(buf);
   
   gzread(f, tlb_LUT_r, 0x100000);
   gzread(f, tlb_LUT_w, 0x100000);
   
   gzread(f, &llbit, 4);
   gzread(f, reg, 32*8);
   for (i=0; i<32; i++) 
     {
	gzread(f, reg_cop0+i, 4);
	gzread(f, buf, 4); // for compatibility with old versions purpose
     }
   gzread(f, &lo, 8);
   gzread(f, &hi, 8);
   gzread(f, reg_cop1_fgr_64, 32*8);
   gzread(f, &FCR0, 4);
   gzread(f, &FCR31, 4);
   gzread(f, tlb_e, 32*sizeof(tlb));
   if (!dynacore && interpcore) gzread(f, &interp_addr, 4);
   else
     {
	int i;
	gzread(f, &len, 4);
	for (i=0; i<0x100000; i++) invalid_code[i] = 1;
	jump_to(len);
     }
   
   gzread(f, &next_interupt, 4);
   gzread(f, &next_vi, 4);
   gzread(f, &vi_field, 4);
   
   len = 0;
   while(1)
     {
	gzread(f, buf+len, 4);
	if (*((unsigned long*)&buf[len]) == 0xFFFFFFFF) break;
	gzread(f, buf+len+4, 4);
	len += 8;
     }
   load_eventqueue_infos(buf);
   
   gzclose(f);
   if (!dynacore && interpcore)
     last_addr = interp_addr;
   else
     last_addr = PC->addr;
}
