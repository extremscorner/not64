/* main_gc-menu.c - New main that uses menu's instead of prompts
   by Mike Slegeir for Mupen64-GC
   **************************************************************
   TODO: Keep the logo updating while something is loading if possible
 */

/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "winlnxdefs.h"
#include "main.h"
#include "rom.h"
#include "plugin.h"

#include "../r4300/r4300.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/ARAM.h"
#include "../gc_memory/TLB-Cache.h"
#include "ROM-Cache.h"

#include <gccore.h>
#include "../gui/gui_GX.h"
#include "../gui/GUI.h"

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
       char saveEnabled;
// -- End plugin data --

// Framebuffers
static u32* xfb[2] = { NULL, NULL };

int main(){
	/* INITIALIZE */
	Initialise(); // Stock OGC initialization
	SDCARD_Init();
	DVD_Init();
	
	// Default Settings
	audioEnabled = 0; // No audio
	saveEnabled  = 0; // Don't save game
	dynacore     = 2; // Pure Interpreter
	
	/* MAIN LOOP */
	PADStatus padStatus;
	while(TRUE){
		// First read the pads
		PAD_Read( &padStatus );
		
		// Navigate the menu accordingly
		if( padStatus.button & PAD_BUTTON_UP )
			menuNavigate(-1);
		else if( padStatus.button & PAD_BUTTON_DOWN )
			menuNavigate(1);
		
		if( padStatus.button & PAD_BUTTON_A )
			menuSelect();
		else if( padStatus.button & PAD_BUTTON_B )
			menuBack();
		
		// TODO: Analog stick
		
		// Display everything
		/* DISPLAY BACKGROUND */
		menuDisplay();
		GUI_draw();
//		GUI_update();
//		GUI_displayText();
//		GUI_drawLogo();
		/* COPY TO FRAME BUFFER */
		
//		VIDEO_WaitVSync();
	}
	
	/* DEINITIALIZE */
	
	return 0;
}

static BOOL hasLoadedROM = FALSE;
void loadROM(fileBrowser* rom){
	// First, if there's already a loaded ROM
	if(hasLoadedROM){
		// Unload it, and deinit everything
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
		ARAM_manager_deinit();
	}
	hasLoadedROM = TRUE;
	
	ARAM_manager_init();
	TLBCache_init();
	
	rom_read(rom);
	
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
	
	/* CPU Init */
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

static void Initialise (void){
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
  GX_SetCopyClear ((GXColor){64,64,64,255}, 0x00000000);
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

