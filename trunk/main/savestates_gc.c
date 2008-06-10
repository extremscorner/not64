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
#include <unistd.h>
#include <gccore.h>
#include "savestates.h"
#include "guifuncs.h"
#include "rom.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/flashram.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"

char* statespath = "/N64SAVES/";

extern unsigned long interp_addr;
extern int *autoinc_save_slot;

int savestates_job = 0;
extern BOOL hasLoadedROM;
static unsigned int slot = 0;

void savestates_select_slot(unsigned int s)
{
   if (s > 9) return;
   slot = s;
}

char* savestates_save()
{
	if(!hasLoadedROM)
		return "A ROM must be loaded first";
   	char *filename, buf[1024];
   	gzFile f;
   	int len, i;
    int curPos;
   
	filename = malloc(strlen(statespath)+strlen(ROM_SETTINGS.goodname)+4+1);
    strcpy(filename, statespath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".st");
	sprintf(buf, "%d", slot);
	strcat(filename, buf);


	f = gzopen(filename, "wb");
   	free(filename);
   	if(!f)
   		return "Error Saving State";
   
   
   	gzwrite(f, ROM_SETTINGS.MD5, 32);		//Needs to actually get done
   
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
#ifdef USE_EXPANSION	
	gzwrite(f, rdram, 0x800000);
#else
	gzwrite(f, rdram, 0x400000);
	curPos = gztell(f);
	gzseek(f,curPos+0x400000,SEEK_SET);	//lets just write 0x400000 zeroes
#endif
	gzwrite(f, SP_DMEM, 0x1000);
	gzwrite(f, SP_IMEM, 0x1000);
	gzwrite(f, PIF_RAM, 0x40);
	
	save_flashram_infos(buf);
	gzwrite(f, buf, 24);
#ifndef USE_TLB_CACHE
	gzwrite(f, tlb_LUT_r, 0x100000);		
	gzwrite(f, tlb_LUT_w, 0x100000);
#else
	curPos = gztell(f);
	gzseek(f,curPos+0x200000,SEEK_SET);	//until we can save tlb cache, lets just write 0x200000 zeroes
#endif

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
	return "Save Successful";
}

char* savestates_load()
{
	if(!hasLoadedROM)
		return "A ROM must be loaded first";
	char *filename, buf[1024];
	gzFile f = NULL;
	int len, i;
	int curPos;
	
	filename = malloc(strlen(statespath)+strlen(ROM_SETTINGS.goodname)+4+1);
	strcpy(filename, statespath);
	strcat(filename, ROM_SETTINGS.goodname);
	strcat(filename, ".st");
	sprintf(buf, "%d", slot);
	strcat(filename, buf);
	
	f = gzopen(filename, "rb");
	free(filename);
	
	if (f == NULL)
	  {
		return "Save doesn't exist";
	  }
	
	gzread(f, buf, 32);
	//don't care for now
/*	if (memcmp(buf, ROM_SETTINGS.MD5, 32))
	{
			warn_savestate_from_another_rom();
			gzclose(f);
			return;
	}
*/	
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
	//only read what we can handle
#ifdef USE_EXANSION	
	gzread(f, rdram, 0x800000);
#else
	gzread(f, rdram, 0x400000);
	curPos = gztell(f);
	gzseek(f,curPos+0x400000,SEEK_SET);
#endif
	gzread(f, SP_DMEM, 0x1000);
	gzread(f, SP_IMEM, 0x1000);
	gzread(f, PIF_RAM, 0x40);
	
	gzread(f, buf, 24);
	load_flashram_infos(buf);
#ifndef USE_TLB_CACHE
	gzread(f, tlb_LUT_r, 0x100000);
	gzread(f, tlb_LUT_w, 0x100000);
#else
	curPos = gztell(f);
	gzseek(f,curPos+0x200000,SEEK_SET);		//for now, skip tlb, later, fix me to load the cache again
#endif
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
	else	//finish me when dyarec is done
	  {
	//	int i;
		gzread(f, &len, 4);
	//	for (i=0; i<0x100000; i++) invalid_code[i] = 1;
	//	jump_to(len);
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
	return "Load Successful";
}
