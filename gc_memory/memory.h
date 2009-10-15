/**
 * Mupen64 - memory.h
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

#ifndef MEMORY_H
#define MEMORY_H

#include "tlb.h"

#ifdef __WIN32__
#define byte __byte_
#endif

int init_memory();
void free_memory();

enum { MEM_READ_WORD,  MEM_READ_BYTE,  MEM_READ_HALF,  MEM_READ_LONG,
       MEM_WRITE_WORD, MEM_WRITE_BYTE, MEM_WRITE_HALF, MEM_WRITE_LONG };

#define read_word_in_memory()   rwmem[address>>16][MEM_READ_WORD]()
#define read_byte_in_memory()   rwmem[address>>16][MEM_READ_BYTE]()
#define read_hword_in_memory()  rwmem[address>>16][MEM_READ_HALF]()
#define read_dword_in_memory()  rwmem[address>>16][MEM_READ_LONG]()
#define write_word_in_memory()  rwmem[address>>16][MEM_WRITE_WORD]()
#define write_byte_in_memory()  rwmem[address>>16][MEM_WRITE_BYTE]()
#define write_hword_in_memory() rwmem[address>>16][MEM_WRITE_HALF]()
#define write_dword_in_memory() rwmem[address>>16][MEM_WRITE_LONG]()

extern unsigned long SP_DMEM[0x1000/4*2];
extern unsigned char *SP_DMEMb;
extern unsigned long *SP_IMEM;
extern unsigned long PIF_RAM[0x40/4];
extern unsigned char *PIF_RAMb;
#ifdef USE_EXPANSION
	extern unsigned long rdram[0x800000/4];
#else
	extern unsigned long rdram[0x800000/4/2];
#endif
extern unsigned long address, word;
extern unsigned char byte;
extern unsigned short hword;
extern unsigned long long int dword, *rdword;

extern void (**rwmem[0x10000])();

typedef struct _RDRAM_register
{
   unsigned long rdram_config;
   unsigned long rdram_device_id;
   unsigned long rdram_delay;
   unsigned long rdram_mode;
   unsigned long rdram_ref_interval;
   unsigned long rdram_ref_row;
   unsigned long rdram_ras_interval;
   unsigned long rdram_min_interval;
   unsigned long rdram_addr_select;
   unsigned long rdram_device_manuf;
} RDRAM_register;

typedef struct _SP_register
{
   unsigned long sp_mem_addr_reg;
   unsigned long sp_dram_addr_reg;
   unsigned long sp_rd_len_reg;
   unsigned long sp_wr_len_reg;
   unsigned long w_sp_status_reg;
   unsigned long sp_status_reg;
   char halt;
   char broke;
   char dma_busy;
   char dma_full;
   char io_full;
   char single_step;
   char intr_break;
   char signal0;
   char signal1;
   char signal2;
   char signal3;
   char signal4;
   char signal5;
   char signal6;
   char signal7;
   unsigned long sp_dma_full_reg;
   unsigned long sp_dma_busy_reg;
   unsigned long sp_semaphore_reg;
} SP_register;

typedef struct _RSP_register
{
   unsigned long rsp_pc;
   unsigned long rsp_ibist;
} RSP_register;

typedef struct _DPC_register
{
   unsigned long dpc_start;
   unsigned long dpc_end;
   unsigned long dpc_current;
   unsigned long w_dpc_status;
   unsigned long dpc_status;
   char xbus_dmem_dma;
   char freeze;
   char flush;
   char start_glck;
   char tmem_busy;
   char pipe_busy;
   char cmd_busy;
   char cbuf_busy;
   char dma_busy;
   char end_valid;
   char start_valid;
   unsigned long dpc_clock;
   unsigned long dpc_bufbusy;
   unsigned long dpc_pipebusy;
   unsigned long dpc_tmem;
} DPC_register;

typedef struct _DPS_register
{
   unsigned long dps_tbist;
   unsigned long dps_test_mode;
   unsigned long dps_buftest_addr;
   unsigned long dps_buftest_data;
} DPS_register;

typedef struct _mips_register
{
   unsigned long w_mi_init_mode_reg;
   unsigned long mi_init_mode_reg;
   char init_length;
   char init_mode;
   char ebus_test_mode;
   char RDRAM_reg_mode;
   unsigned long mi_version_reg;
   unsigned long mi_intr_reg;
   unsigned long mi_intr_mask_reg;
   unsigned long w_mi_intr_mask_reg;
   char SP_intr_mask;
   char SI_intr_mask;
   char AI_intr_mask;
   char VI_intr_mask;
   char PI_intr_mask;
   char DP_intr_mask;
} mips_register;

typedef struct _VI_register
{
   unsigned long vi_status;
   unsigned long vi_origin;
   unsigned long vi_width;
   unsigned long vi_v_intr;
   unsigned long vi_current;
   unsigned long vi_burst;
   unsigned long vi_v_sync;
   unsigned long vi_h_sync;
   unsigned long vi_leap;
   unsigned long vi_h_start;
   unsigned long vi_v_start;
   unsigned long vi_v_burst;
   unsigned long vi_x_scale;
   unsigned long vi_y_scale;
   unsigned long vi_delay;
} VI_register;

typedef struct _AI_register
{
   unsigned long ai_dram_addr;
   unsigned long ai_len;
   unsigned long ai_control;
   unsigned long ai_status;
   unsigned long ai_dacrate;
   unsigned long ai_bitrate;
   unsigned long next_delay;
   unsigned long next_len;
   unsigned long current_delay;
   unsigned long current_len;
} AI_register;

typedef struct _PI_register
{
   unsigned long pi_dram_addr_reg;
   unsigned long pi_cart_addr_reg;
   unsigned long pi_rd_len_reg;
   unsigned long pi_wr_len_reg;
   unsigned long read_pi_status_reg;
   unsigned long pi_bsd_dom1_lat_reg;
   unsigned long pi_bsd_dom1_pwd_reg;
   unsigned long pi_bsd_dom1_pgs_reg;
   unsigned long pi_bsd_dom1_rls_reg;
   unsigned long pi_bsd_dom2_lat_reg;
   unsigned long pi_bsd_dom2_pwd_reg;
   unsigned long pi_bsd_dom2_pgs_reg;
   unsigned long pi_bsd_dom2_rls_reg;
} PI_register;

typedef struct _RI_register
{
   unsigned long ri_mode;
   unsigned long ri_config;
   unsigned long ri_current_load;
   unsigned long ri_select;
   unsigned long ri_refresh;
   unsigned long ri_latency;
   unsigned long ri_error;
   unsigned long ri_werror;
} RI_register;

typedef struct _SI_register
{
   unsigned long si_dram_addr;
   unsigned long si_pif_addr_rd64b;
   unsigned long si_pif_addr_wr64b;
   unsigned long si_status;
} SI_register;

extern RDRAM_register rdram_register;
extern PI_register pi_register;
extern mips_register MI_register;
extern SP_register sp_register;
extern SI_register si_register;
extern VI_register vi_register;
extern RSP_register rsp_register;
extern RI_register ri_register;
extern AI_register ai_register;
extern DPC_register dpc_register;
extern DPS_register dps_register;

#ifndef _BIG_ENDIAN
#define sl(mot) \
( \
((mot & 0x000000FF) << 24) | \
((mot & 0x0000FF00) <<  8) | \
((mot & 0x00FF0000) >>  8) | \
((mot & 0xFF000000) >> 24) \
)

#define S8 3
#define S16 2
#define Sh16 1

#else

#define sl(mot) mot
#define S8 0
#define S16 0
#define Sh16 0

#endif

void read_nothing();
void read_nothingh();
void read_nothingb();
void read_nothingd();
void read_nomem();
void read_nomemb();
void read_nomemh();
void read_nomemd();
void read_rdram();
void read_rdramb();
void read_rdramh();
void read_rdramd();
void read_rdramFB();
void read_rdramFBb();
void read_rdramFBh();
void read_rdramFBd();
void read_rdramreg();
void read_rdramregb();
void read_rdramregh();
void read_rdramregd();
void read_rsp_mem();
void read_rsp_memb();
void read_rsp_memh();
void read_rsp_memd();
void read_rsp_reg();
void read_rsp_regb();
void read_rsp_regh();
void read_rsp_regd();
void read_rsp();
void read_rspb();
void read_rsph();
void read_rspd();
void read_dp();
void read_dpb();
void read_dph();
void read_dpd();
void read_dps();
void read_dpsb();
void read_dpsh();
void read_dpsd();
void read_mi();
void read_mib();
void read_mih();
void read_mid();
void read_vi();
void read_vib();
void read_vih();
void read_vid();
void read_ai();
void read_aib();
void read_aih();
void read_aid();
void read_pi();
void read_pib();
void read_pih();
void read_pid();
void read_ri();
void read_rib();
void read_rih();
void read_rid();
void read_si();
void read_sib();
void read_sih();
void read_sid();
void read_flashram_status();
void read_flashram_statusb();
void read_flashram_statush();
void read_flashram_statusd();
void read_rom();
void read_romb();
void read_romh();
void read_romd();
void read_pif();
void read_pifb();
void read_pifh();
void read_pifd();

void write_nothing();
void write_nothingb();
void write_nothingh();
void write_nothingd();
void write_nomem();
void write_nomemb();
void write_nomemd();
void write_nomemh();
void write_rdram();
void write_rdramb();
void write_rdramh();
void write_rdramd();
void write_rdramFB();
void write_rdramFBb();
void write_rdramFBh();
void write_rdramFBd();
void write_rdramreg();
void write_rdramregb();
void write_rdramregh();
void write_rdramregd();
void write_rsp_mem();
void write_rsp_memb();
void write_rsp_memh();
void write_rsp_memd();
void write_rsp_reg();
void write_rsp_regb();
void write_rsp_regh();
void write_rsp_regd();
void write_rsp();
void write_rspb();
void write_rsph();
void write_rspd();
void write_dp();
void write_dpb();
void write_dph();
void write_dpd();
void write_dps();
void write_dpsb();
void write_dpsh();
void write_dpsd();
void write_mi();
void write_mib();
void write_mih();
void write_mid();
void write_vi();
void write_vib();
void write_vih();
void write_vid();
void write_ai();
void write_aib();
void write_aih();
void write_aid();
void write_pi();
void write_pib();
void write_pih();
void write_pid();
void write_ri();
void write_rib();
void write_rih();
void write_rid();
void write_si();
void write_sib();
void write_sih();
void write_sid();
void write_flashram_dummy();
void write_flashram_dummyb();
void write_flashram_dummyh();
void write_flashram_dummyd();
void write_flashram_command();
void write_flashram_commandb();
void write_flashram_commandh();
void write_flashram_commandd();
void write_rom();
void write_pif();
void write_pifb();
void write_pifh();
void write_pifd();

void update_SP();
void update_DPC();

#endif
