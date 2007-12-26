/* main_gc.c - Main entry point for emulator
	by Mike Slegeir for Mupen64-GC
 */

#define VERSION "0.5\0"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "rom.h"

#include "plugin.h"

#include "../gui/gui_GX.h"
#include "../r4300/r4300.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/ARAM.h"
#include "../gc_memory/TLB-Cache.h"
#include "ROM-Cache.h" 
#include "winlnxdefs.h"
#include "savestates.h"

#include "../config.h"

#include <gccore.h>
#include <sdcard.h>

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif
 
#define DEFAULT_FIFO_SIZE    (256*1024)//(64*1024) minimum


int p_noask;

extern char audioEnabled;
       char saveEnabled;

CONTROL Controls[4];

static GFX_INFO gfx_info;
AUDIO_INFO audio_info;
static CONTROL_INFO control_info;
static RSP_INFO rsp_info;

static void dummy_func(){ }
void (*fBRead)(DWORD addr) = NULL;
void (*fBWrite)(DWORD addr, DWORD size) = NULL;
void (*fBGetFrameBufferInfo)(void *p) = NULL;
static u32* xfb[2] = { NULL, NULL };    /*** Framebuffers ***/

void display_loading_progress(int p){
	printf("loading rom : %d%%\r", p);
	fflush(stdout);
	if (p==100) printf("\n");
}

void display_MD5calculating_progress(int p){ }

int ask_bad(){
	char c;
	printf("The rom you are trying to load is probably a bad dump\n");
	printf("Be warned that this will probably give unexpected results.\n");
	printf("Do you still want to run it (y/n) ?");
	
	if(p_noask) return 1;
	/*
	else {
		c = getchar();
		getchar();
		if (c=='y' || c=='Y') return 1;
		else return 0;
	}
	*/
}

int ask_hack(){
	char c;
	printf("The rom you are trying to load is not a good verified dump\n");
	printf("It can be a hacked dump, trained dump or anything else that \n");
	printf("may work but be warned that it can give unexpected results.\n");
	printf("Do you still want to run it (y/n) ?");
	
	if(p_noask) return 1;
	/*
	else {
		c = getchar();
		getchar();
		if (c=='y' || c=='Y') return 1;
		else return 0;
	}
	*/
}

void warn_savestate_from_another_rom(){
	printf("Error: You're trying to load a save state from either another rom\n");
	printf("       or another dump.\n");
}

void warn_savestate_not_exist(){
	printf("Error: The save state you're trying to load doesn't exist\n");
}

void new_frame(){ }

void new_vi(){ }
static void Initialise(void);
static void gfx_info_init(void);
static void audio_info_init(void);
static void rsp_info_init(void);
static void control_info_init(void);
//DVD
int isFromDVD = 0;
extern int rom_sizeDVD;
extern unsigned int rom_offsetDVD;

static void check_heap_space(void){
	int space = 6 * 1024 * 1024, *ptr=NULL;
	while(space > 0)
		if(ptr = malloc(space)) break;
		else space -= 4096;
	if(ptr) free(ptr);
	printf("At least %dB or %dKB space available\n", space, space/1024);
}

unsigned int isWii = 0; //this will come in handly later (used for DVD now)
#define mfpvr()   ({unsigned int rval; \
      asm volatile("mfpvr %0" : "=r" (rval)); rval;})

// Initialize all the values that are set by the user
static void setDefaults(void){
	audioEnabled = 0; // No audio
	dynacore = 2; // Pure Interpreter
	saveEnabled = 0; // Don't save game
}
      
int main(){
	char* romfile = NULL;		//SD
	int i;
	
	rom = NULL;
	ROM_HEADER = NULL;

	Initialise();
	
	isWii = mfpvr();
	if(isWii == 0x87102)
	{
		isWii = 1;
		PRINT("Running on a Wii :)\n");
	}
	else {
		PRINT("Running on a GC :)\n");
		isWii = 0;
	}


	while(TRUE){
	
		ARAM_manager_init();
		TLBCache_init();
		SDCARD_Init();
		DVD_Init();
		
		PRINT("Press A to choose ROM from SDCard\n");
		PRINT("Press Z to choose ROM from DVD\n");
		while(1){
			if((PAD_ButtonsHeld(0) & PAD_BUTTON_A)) {
				romfile = textFileBrowser("dev0:\\N64ROMS");
				rom_read(romfile);
				break;
			}
			if((PAD_ButtonsHeld(0) & PAD_TRIGGER_Z)) {
				isFromDVD = 1;
				romfile = malloc(1024);
				strcpy(romfile,textFileBrowserDVD());
				rom_read(romfile); 
				break;
			}
	  	VIDEO_WaitVSync ();        /*** Wait for VBL ***/
	  	VIDEO_WaitVSync ();        /*** Wait for VBL ***/
	  	VIDEO_WaitVSync ();        /*** Wait for VBL ***/
	  	VIDEO_WaitVSync ();        /*** Wait for VBL ***/
	  	VIDEO_WaitVSync ();        /*** Wait for VBL ***/
		}
		free(romfile); 

		select_location(); // for game saves
		
		init_memory();
			
		char buffer[64];
		sprintf(buffer, "Goodname:%s\n", ROM_SETTINGS.goodname);
		PRINT(buffer);
		sprintf(buffer, "16kb eeprom=%d\n", ROM_SETTINGS.eeprom_16kb);
		PRINT(buffer);
	
		PRINT("Enable Audio?\n"
		      "  A. Yes\n"
		      "  B. No\n");
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_A ||
		         PAD_ButtonsHeld(0) & PAD_BUTTON_B ));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_A) audioEnabled = 1;
		else audioEnabled = 0;
		while (PAD_ButtonsHeld(0) & PAD_BUTTON_A ||
		       PAD_ButtonsHeld(0) & PAD_BUTTON_B );
	
		PRINT( "emulation mode:\n"
		       "	  A. interpreter (PROBABLY BROKEN)\n"
		       "	  B. dynamic recompiler (NOT SUPPORTED YET!)\n"
		       "	  X. pure interpreter\n");
		
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_A ||
		         PAD_ButtonsHeld(0) & PAD_BUTTON_B ||
		         PAD_ButtonsHeld(0) & PAD_BUTTON_X ));
		
		if (PAD_ButtonsHeld(0) & PAD_BUTTON_A) dynacore=0;
		else if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) dynacore=2;
		else dynacore=1;
		
		gfx_info_init();
		audio_info_init();
		control_info_init();
		rsp_info_init();
		
		romOpen_gfx();
		gfx_set_fb(xfb[0], xfb[1]);
		romOpen_audio();
		romOpen_input();
		//check_heap_space();
		PRINT("Press START to begin execution\n");
		while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_START));
	#ifdef USE_GUI
		GUI_toggle();
	#endif
		go();
	#ifdef USE_GUI
		GUI_toggle();
	#endif
		romClosed_RSP();
		romClosed_input();
		romClosed_audio();
		romClosed_gfx();
		closeDLL_RSP();
		closeDLL_input();
		closeDLL_audio();
		closeDLL_gfx();
		
		ROMCache_deinit();
		if(isFromDVD) {
			isFromDVD = 0;
			rom_sizeDVD = 0;
			rom_offsetDVD = 0;
		}
		free(romfile);
		romfile = NULL;
	
		free(ROM_HEADER);
		ROM_HEADER = NULL;
		free_memory();
		ARAM_manager_deinit();
		
		VIDEO_SetNextFramebuffer (xfb[0]); //switch xfb to show console
		VIDEO_Flush ();
	
		// Wait until X & Y are released before continuing
	   	while(((PAD_ButtonsHeld(0) & PAD_BUTTON_A) && (PAD_ButtonsHeld(0) & PAD_BUTTON_Y)));
		PRINT("Press X to return to SDLOAD\n  or START to load new ROM\n");
	   	while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_START) &&
	   	      !(PAD_ButtonsHeld(0) & PAD_BUTTON_X));
	   	if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) break;
   	
   	}

	void (*reload)() = (void (*)()) 0x80001800;
	reload();
   	
	return 0;
}

static void gfx_info_init(void){
	gfx_info.MemoryBswaped = TRUE;
	gfx_info.HEADER = ROM_HEADER;
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
	audio_info.HEADER = ROM_HEADER;
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
	control_info.HEADER = ROM_HEADER;
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

static void
Initialise (void)
{
  static GXRModeObj *vmode;        /*** Graphics Mode Object ***/
  static int whichfb = 0;        /*** Frame buffer toggle ***/
  VIDEO_Init ();
  PAD_Init ();
  switch (VIDEO_GetCurrentTvMode ())
    {
    case VI_NTSC:
      vmode = &TVNtsc480IntDf;
      break;
 
    case VI_PAL:
      vmode = &TVPal528IntDf;
      break;
 
    case VI_MPAL:
      vmode = &TVMpal480IntDf;
      break;
 
    default:
      vmode = &TVNtsc480IntDf;
      break;
    }
  VIDEO_Configure (vmode);
  xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
  xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
  console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight,
        vmode->fbWidth * 2);
  VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
  VIDEO_SetNextFramebuffer (xfb[0]);
  VIDEO_SetPostRetraceCallback (PAD_ScanPads);
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
  GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
  // Set the correct y scaling for efb->xfb copy operation
  GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight); 
  GX_SetCullMode (GX_CULL_NONE); // default in rsp init
  GX_CopyDisp (xfb[0], GX_TRUE); // This clears the efb
  GX_CopyDisp (xfb[0], GX_TRUE); // This clears the xfb

#ifdef USE_GUI
  GUI_setFB(xfb[0], xfb[1]);
  GUI_init();
#endif
}

