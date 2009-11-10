/**
 * Wii64 - main_gc-menu.c (aka MenuV1)
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * New main that uses menu's instead of prompts
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
 *                emukidid@gmail.com
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
**/


/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#ifdef DEBUGON
# include <debug.h>
#endif

#include "winlnxdefs.h"
#include "main.h"
#include "rom.h"
#include "plugin.h"
#include "savestates.h"
#include <gccore.h>
#include "../gui/gui_GX-menu.h"
#include "../gui/GUI.h"
#include "../gui/menu.h"
#include "../gui/DEBUG.h"
#include "timers.h"
#ifdef WII
unsigned int MALLOC_MEM2 = 0;
#include <ogc/conf.h>
#include <wiiuse/wpad.h>
#include <di/di.h>
#include "../gc_memory/MEM2.h"
#endif

#include "../r4300/r4300.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/ARAM.h"
#include "../gc_memory/TLB-Cache.h"
#include "../gc_memory/tlb.h"
#include "../gc_memory/pif.h"
#include "ROM-Cache.h"
#include "wii64config.h"

/* NECESSARY FUNCTIONS AND VARIABLES */

// -- Initialization functions --
static void Initialise(void);
static void gfx_info_init(void);
static void audio_info_init(void);
static void rsp_info_init(void);
static void control_info_init(void);
// -- End init functions --

// -- Plugin data --
#define DEFAULT_FIFO_SIZE    (256*1024)//(64*1024) minimum

CONTROL Controls[4];

static GFX_INFO     gfx_info;
       AUDIO_INFO   audio_info;
static CONTROL_INFO control_info;
static RSP_INFO     rsp_info;

extern char audioEnabled;
extern char printToScreen;
extern char showFPSonScreen;
extern char printToSD;
#ifdef GLN64_GX
extern char glN64_useFrameBufferTextures;
extern char glN64_use2xSaiTextures;
#endif //GLN64_GX
extern timers Timers;
       char saveEnabled;
       char creditsScrolling;
       char padNeedScan;
       char wpadNeedScan;
       char shutdown;
	   char saveStateDevice;
	   char pakMode[4];

extern void gfx_set_fb(unsigned int* fb1, unsigned int* fb2);
// -- End plugin data --

static u32* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
//static GXRModeObj *vmode;				/*** Graphics Mode Object ***/
GXRModeObj *vmode, *rmode;				/*** Graphics Mode Object ***/
GXRModeObj vmode_phys, rmode_phys;		/*** Graphics Mode Object ***/
char screenMode = 0;
int GX_xfb_offset = 0;

// Dummy functions
static void dummy_func(){ }
void (*fBRead)(DWORD addr) = NULL;
void (*fBWrite)(DWORD addr, DWORD size) = NULL;
void (*fBGetFrameBufferInfo)(void *p) = NULL;
//void new_frame(){ }
//void new_vi(){ }
// Read PAD format from Classic if available
u16 readWPAD(void);

int main(){
	/* INITIALIZE */
#ifdef HW_RVL
  DI_Close();
  DI_Init();    // first
#endif

	Initialise(); // Stock OGC initialization
#ifndef WII
	DVD_Init();
#endif
	menuInit();
#ifdef DEBUGON
	//DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT); //Default port is 2828
	DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	_break();
#endif

	// Default Settings
	audioEnabled     = 1; // Audio
	showFPSonScreen  = 1; // Show FPS on Screen
	printToScreen    = 0; // Show DEBUG text on screen
	printToSD        = 0; // Disable SD logging
	Timers.limitVIs  = 0; // Sync to Audio
	saveEnabled      = 0; // Don't save game
	creditsScrolling = 0; // Normal menu for now
	dynacore         = 1; // Dynarec
	screenMode		 = 0; // Stretch FB horizontally
	pakMode[0]		 = PAKMODE_MEMPAK; // memPak plugged into controller 1
	pakMode[1]		 = PAKMODE_MEMPAK;
	pakMode[2]		 = PAKMODE_MEMPAK;
	pakMode[3]		 = PAKMODE_MEMPAK;
#ifdef GLN64_GX
// glN64 specific  settings
 	glN64_useFrameBufferTextures = 0; // Disable FrameBuffer textures
	glN64_use2xSaiTextures = 0;	// Disable 2xSai textures
	renderCpuFramebuffer = 0; // Disable CPU Framebuffer Rendering
#endif //GLN64_GX
	// 'Page flip' buttons so we know when it released
	int which_pad = 0;
	u16 buttons[2];

	/* MAIN LOOP */
	while(TRUE){
		if(padNeedScan){ PAD_ScanPads(); padNeedScan = 0; }
		// First read the pads
		buttons[which_pad] = PAD_ButtonsHeld(0) | readWPAD();
#ifdef HW_RVL
    if(shutdown)
      SYS_ResetSystem(SYS_POWEROFF, 0, 0);
#endif
// Check whether button is pressed and make sure it wasn't registered last time
#define PAD_PRESSED(butt0n) ( buttons[which_pad] & butt0n && !(buttons[which_pad^1] & butt0n) )

		// Navigate the menu accordingly
		if( PAD_PRESSED( PAD_BUTTON_UP ) )
			menuNavigate(-1);
		else if( PAD_PRESSED( PAD_BUTTON_DOWN ) )
			menuNavigate(1);

		if( PAD_PRESSED( PAD_BUTTON_A ) )
			menuSelect();
		else if( PAD_PRESSED( PAD_BUTTON_B ) )
			menuBack();

		// TODO: Analog stick

		// 'Flip' buttons
		which_pad ^= 1;

		// Display everything
		menuDisplay();
		if(creditsScrolling) GUI_creditScreen();
		else GUI_draw();
	}

	return 0;
}

#if defined(WII)
u16 readWPAD(void){
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(0);

	u16 b = 0;
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_CLASSIC){
	   	u16 w = wpad->exp.classic.btns;
	   	b |= (w & CLASSIC_CTRL_BUTTON_UP)    ? PAD_BUTTON_UP    : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_DOWN)  ? PAD_BUTTON_DOWN  : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_LEFT)  ? PAD_BUTTON_LEFT  : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_RIGHT) ? PAD_BUTTON_RIGHT : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_A) ? PAD_BUTTON_A : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_B) ? PAD_BUTTON_B : 0;
	}

	return b;
}
#else
u16 readWPAD(void){ return 0; }
#endif

extern BOOL eepromWritten;
extern BOOL mempakWritten;
extern BOOL sramWritten;
extern BOOL flashramWritten;
BOOL hasLoadedROM = FALSE;
int loadROM(fileBrowser_file* rom){

  savestates_job = 0;
	// First, if there's already a loaded ROM
	if(hasLoadedROM){
		// Unload it, and deinit everything
		cpu_deinit();
		eepromWritten = FALSE;
		mempakWritten = FALSE;
		sramWritten = FALSE;
		flashramWritten = FALSE;
		romClosed_RSP();
		romClosed_input();
		romClosed_audio();
		romClosed_gfx();
		closeDLL_RSP();
		closeDLL_input();
		closeDLL_audio();
		closeDLL_gfx();

		ROMCache_deinit();
		free_memory();
#ifndef HW_RVL
		ARAM_manager_deinit();
#endif
	}
	format_mempacks();
	hasLoadedROM = TRUE;
#ifndef HW_RVL
	ARAM_manager_init();
#endif
#ifdef USE_TLB_CACHE
	TLBCache_init();
#else
	tlb_mem2_init();
#endif
	//romFile_init(rom);
	if(rom_read(rom)){	// Something failed while trying to read the ROM.
		hasLoadedROM = FALSE;
		return -1;
	}

	// Init everything for this ROM
	init_memory();

	gfx_info_init();
	audio_info_init();
	control_info_init();
	rsp_info_init();

	romOpen_gfx();
	gfx_set_fb(xfb[0], xfb[1]);
	romOpen_audio();
	romOpen_input();

	cpu_init();
	return 0;
}

static void gfx_info_init(void){
	gfx_info.MemoryBswaped = TRUE;
	gfx_info.HEADER = (BYTE*)ROM_HEADER;
	gfx_info.RDRAM = (BYTE*)rdram;
	gfx_info.DMEM = (BYTE*)SP_DMEM;
	gfx_info.IMEM = (BYTE*)SP_IMEM;
	gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
	gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
	gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
	gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
	gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
	gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
	gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
	gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
	gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
	gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
	gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
	gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
	gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
	gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
	gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
	gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
	gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
	gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
	gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
	gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
	gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
	gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
	gfx_info.CheckInterrupts = dummy_func;
	initiateGFX(gfx_info);
}

static void audio_info_init(void){
	audio_info.MemoryBswaped = TRUE;
	audio_info.HEADER = (BYTE*)ROM_HEADER;
	audio_info.RDRAM = (BYTE*)rdram;
	audio_info.DMEM = (BYTE*)SP_DMEM;
	audio_info.IMEM = (BYTE*)SP_IMEM;
	audio_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
	audio_info.AI_LEN_REG = &(ai_register.ai_len);
	audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
	audio_info.AI_STATUS_REG = &(ai_register.ai_status); // FIXME: This was set to dummy
	audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
	audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);
	audio_info.CheckInterrupts = dummy_func;
	initiateAudio(audio_info);
}

static void control_info_init(void){
	control_info.MemoryBswaped = TRUE;
	control_info.HEADER = (BYTE*)ROM_HEADER;
	control_info.Controls = Controls;
	int i;
	for (i=0; i<4; i++)
	  {
	     Controls[i].Present = FALSE;
	     Controls[i].RawData = FALSE;
	     Controls[i].Plugin = PLUGIN_NONE;
	  }
	initiateControllers(control_info);
}

static void rsp_info_init(void){
	static int cycle_count;
	rsp_info.MemoryBswaped = TRUE;
	rsp_info.RDRAM = (BYTE*)rdram;
	rsp_info.DMEM = (BYTE*)SP_DMEM;
	rsp_info.IMEM = (BYTE*)SP_IMEM;
	rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
	rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
	rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
	rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
	rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
	rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
	rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
	rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
	rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
	rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
	rsp_info.DPC_START_REG = &dpc_register.dpc_start;
	rsp_info.DPC_END_REG = &dpc_register.dpc_end;
	rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
	rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
	rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
	rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
	rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
	rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
	rsp_info.CheckInterrupts = dummy_func;
	rsp_info.ProcessDlistList = processDList;
	rsp_info.ProcessAlistList = processAList;
	rsp_info.ProcessRdpList = processRDPList;
	rsp_info.ShowCFB = showCFB;
	initiateRSP(rsp_info,(DWORD*)&cycle_count);
}

void ScanPADSandReset() {
	padNeedScan = wpadNeedScan = 1;
	if(!((*(u32*)0xCC003000)>>16))
		stop = 1;
}

#ifdef HW_RVL
void ShutdownWii() {
  shutdown = 1;
  stop = 1;
}
#endif

static void Initialise (void){

  VIDEO_Init();
  PAD_Init();
  PAD_Reset(0xf0000000);
#ifdef HW_RVL
  CONF_Init();
  WPAD_Init();
  //WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownWii);
  SYS_SetPowerCallback(ShutdownWii);
#endif

  vmode = VIDEO_GetPreferredMode(&vmode_phys);
  rmode = &rmode_phys;
  memcpy( rmode, vmode, sizeof(GXRModeObj));
#ifdef HW_RVL
  if(VIDEO_HaveComponentCable() && CONF_GetProgressiveScan())
  {
		memcpy( vmode, &TVNtsc480Prog, sizeof(GXRModeObj));
		memcpy( rmode, vmode, sizeof(GXRModeObj));
/*		if(CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		{
			screenMode = 1;
			vmode->fbWidth = VI_MAX_WIDTH_NTSC;
			vmode->viWidth = VI_MAX_WIDTH_NTSC;
//			vmode->viXOrigin = 80;
			GX_xfb_offset = 24;
		}*/
  }
#else
  if(VIDEO_HaveComponentCable())
  {
		memcpy( vmode, &TVNtsc480Prog, sizeof(GXRModeObj));
		memcpy( rmode, vmode, sizeof(GXRModeObj));
  }
#endif
  VIDEO_Configure (vmode);
#if 0 //def HW_RVL //Place xfb in MEM2.
  xfb[0] = (u32 *) XFB0_LO;
  xfb[1] = (u32 *) XFB1_LO;
#else
  xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
  xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
#endif
  console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight,
        vmode->fbWidth * 2);
  VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
  VIDEO_SetNextFramebuffer (xfb[0]);
  VIDEO_SetPostRetraceCallback (ScanPADSandReset);
  VIDEO_SetBlack (0);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();        /*** Wait for VBL ***/
  if (vmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync ();

  // setup the fifo and then init GX
  void *gp_fifo = NULL;
  gp_fifo = MEM_K0_TO_K1 (memalign (32, DEFAULT_FIFO_SIZE));
  memset (gp_fifo, 0, DEFAULT_FIFO_SIZE);

  GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);
	  
  // clears the bg to color and clears the z buffer
//  GX_SetCopyClear ((GXColor){64,64,64,255}, 0x00000000);
  GX_SetCopyClear ((GXColor){0,0,0,255}, 0x00000000);
  // init viewport
  GX_SetViewport (0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
  // Set the correct y scaling for efb->xfb copy operation
  GX_SetDispCopyYScale ((f32) rmode->xfbHeight / (f32) rmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
  GX_SetCullMode (GX_CULL_NONE); // default in rsp init
  GX_CopyDisp (xfb[0]+GX_xfb_offset, GX_TRUE); // This clears the efb
  GX_CopyDisp (xfb[0]+GX_xfb_offset, GX_TRUE); // This clears the xfb

#ifdef USE_GUI
  GUI_setFB(xfb[0], xfb[1]);
  GUI_init();
#endif

	// Init PS GQRs so I can load signed/unsigned chars/shorts as PS values
	__asm__ volatile(
		"lis	3, 4     \n"
		"addi	3, 3, 4  \n"
		"mtspr	913, 3   \n" // GQR1 = unsigned char
		"lis	3, 6     \n"
		"addi	3, 3, 6  \n"
		"mtspr	914, 3   \n" // GQR2 = signed char
		"lis	3, 5     \n"
		"addi	3, 3, 5  \n"
		"mtspr	915, 3   \n" // GQR3 = unsigned short
		"lis	3, 7     \n"
		"addi	3, 3, 7  \n"
		"mtspr	916, 3   \n" // GQR4 = signed short
		"lis	3, %0    \n"
		"addi	3, 3, %0 \n"
		"mtspr	917, 3   \n" // GQR5 = unsigned short / (2^16)
		:: "n" (16<<8 | 5) : "r3");
}

