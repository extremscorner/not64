#ifdef __BIG_ENDIAN__

#ifdef __WIN32__
#include <windows.h>
#include "./winproject/resource.h"
#include "./win/win.h"
#else
#ifdef USE_GTK
#include <gtk/gtk.h>
#endif
#include "wintypes.h"
#endif
#include <stdio.h>

#ifdef __PPC__
#include "RSPPlugin.h"
#endif

#include "Rsp_#1.1.h"
#include "hle.h"

#include "Audio_#1.1.h"
#include <malloc.h>
#include <string.h>
RSP_INFO rsp;

unsigned long inst1, inst2;

BOOL AudioHle = FALSE, GraphicsHle = TRUE, SpecificHle = FALSE;

#ifdef __WIN32__
extern void (*processAList)();
static BOOL firstTime = TRUE;
void loadPlugin();
#endif

//void disasm(FILE *f, unsigned long t[0x1000/4]);

void CloseDLL (void)
{
}

void DllAbout ( HWND hParent )
{
#ifdef __WIN32__
   MessageBox(NULL, "Mupen64 HLE RSP-PPC plugin v0.2 by Hacktarux", "RSP HLE", MB_OK);
#else
#ifdef USE_GTK
   char tMsg[256];
   GtkWidget *dialog, *label, *okay_button;
   
   dialog = gtk_dialog_new();
   sprintf(tMsg,"Mupen64 HLE RSP-PPC plugin v0.2 by Hacktarux");
   label = gtk_label_new(tMsg);
   okay_button = gtk_button_new_with_label("OK");
   
   gtk_signal_connect_object(GTK_OBJECT(okay_button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT(dialog));
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
		     okay_button);
   
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
		     label);
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   gtk_widget_show_all(dialog);
#else
   char tMsg[256];
   sprintf(tMsg,"Mupen64 HLE RSP-PPC plugin v0.2 by Hacktarux");
   //fprintf(stderr, "About\n%s\n", tMsg);
#endif
#endif
}

void DllConfig ( HWND hParent )
{
#ifdef __WIN32__
    if (firstTime)
    DialogBox(dll_hInstance, 
                     MAKEINTRESOURCE(IDD_RSPCONFIG), hParent, ConfigDlgProc);
   //MessageBox(NULL, "no config", "noconfig", MB_OK);
#endif
}

 void DllTest ( HWND hParent )
{
#ifdef __WIN32__
   MessageBox(NULL, "no test", "no test", MB_OK);
#endif
}

int audio_ucode_detect(OSTask_t *task)
{
   if (*(unsigned long*)(rsp.RDRAM + task->ucode_data + 0) != 0x1)
     {
	if (*(rsp.RDRAM + task->ucode_data + (0 ^ (3-S8))) == 0xF)
	  return 4;
	else
	  return 2;
     }
   else 
     {
	if (*(unsigned long*)(rsp.RDRAM + task->ucode_data + 0x30) == 0xF0000F00)
	  return 1;
	else
	  return 3;
     }
}

__declspec(dllexport) DWORD DoRspCycles ( DWORD Cycles )
{
   OSTask_t *task = (OSTask_t*)(rsp.DMEM + 0xFC0);
   unsigned int i, sum=0;
   
   if( task->type == 1 && task->data_ptr != 0 && GraphicsHle) {
      if (rsp.ProcessDlistList != NULL) {
	 rsp.ProcessDlistList();
      }
      *rsp.SP_STATUS_REG |= 0x0203;
      if ((*rsp.SP_STATUS_REG & 0x40) != 0 ) {
	 *rsp.MI_INTR_REG |= 0x1;
	 rsp.CheckInterrupts();
      }
      
      *rsp.DPC_STATUS_REG &= ~0x0002;
      return Cycles;
   } else if (task->type == 2 && AudioHle) {
      if (rsp.ProcessAlistList != NULL) {
	 rsp.ProcessAlistList();
      }
      *rsp.SP_STATUS_REG |= 0x0203;
      if ((*rsp.SP_STATUS_REG & 0x40) != 0 ) {
	 *rsp.MI_INTR_REG |= 0x1;
	 rsp.CheckInterrupts();
      }
      return Cycles;
   } else if (task->type == 7) {
      rsp.ShowCFB();
   }
   
   *rsp.SP_STATUS_REG |= 0x203;
   if ((*rsp.SP_STATUS_REG & 0x40) != 0 )
     {
	*rsp.MI_INTR_REG |= 0x1;
	rsp.CheckInterrupts();
     }
   
   if (task->ucode_size <= 0x1000)
     for (i=0; i<(task->ucode_size/2); i++)
       sum += *(rsp.RDRAM + task->ucode + i);
   else
     for (i=0; i<(0x1000/2); i++)
       sum += *(rsp.IMEM + i);
   
   
   if (task->ucode_size > 0x1000)
     {
	switch(sum)
	  {
	   case 0x9E2: // banjo tooie (U) boot code
	       {
		  int i,j;
		  memcpy(rsp.IMEM + 0x120, rsp.RDRAM + 0x1e8, 0x1e8);
		  for (j=0; j<0xfc; j++)
		    for (i=0; i<8; i++)
		      *(rsp.RDRAM+((0x2fb1f0+j*0xff0+i)^S8))=*(rsp.IMEM+((0x120+j*8+i)^S8));
	       }
	     return Cycles;
	     break;
	   case 0x9F2: // banjo tooie (E) + zelda oot (E) boot code
	       {
		  int i,j;
		  memcpy(rsp.IMEM + 0x120, rsp.RDRAM + 0x1e8, 0x1e8);
		  for (j=0; j<0xfc; j++)
		    for (i=0; i<8; i++)
		      *(rsp.RDRAM+((0x2fb1f0+j*0xff0+i)^S8))=*(rsp.IMEM+((0x120+j*8+i)^S8));
	       }
	     return Cycles;
	     break;
	  }
     }
   else
     {
	switch(task->type)
	  {
	   case 2: // audio
	     switch(audio_ucode_detect(task))
	       {
		case 1: // mario ucode
		  ucode1(task);
		  return Cycles;
		  break;
		case 2: // banjo kazooie ucode
		  ucode2(task);
		  return Cycles;
		  break;
		case 3: // zelda ucode
		  ucode3(task);
		  return Cycles;
		  break;
		default:
			break;
	       }
	     break;
	   case 4: // jpeg
	     switch(sum)
	       {
		case 0x278: // used by zelda during boot
		  *rsp.SP_STATUS_REG |= 0x200;
		  return Cycles;
		  break;
		case 0x2e4fc: // uncompress
		  jpg_uncompress(task);
		  return Cycles;
		  break;
		default:
			break;
	       }
	     break;
	  }
     }

	if (task->ucode_size <= 0x1000)
	  {
	     memcpy(rsp.DMEM, rsp.RDRAM+task->ucode_data, task->ucode_data_size);
	     memcpy(rsp.IMEM+0x80, rsp.RDRAM+task->ucode, 0xF7F);
	  }

   
   return Cycles;
}

 void GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
   PluginInfo->Version = 0x0101;
   PluginInfo->Type = PLUGIN_TYPE_RSP;
   strcpy(PluginInfo->Name, "Hacktarux hle rsp-PPC plugin");
   PluginInfo->NormalMemory = TRUE;
   PluginInfo->MemoryBswaped = TRUE;
}

__declspec(dllexport) void InitiateRSP ( RSP_INFO Rsp_Info, DWORD * CycleCount)
{
   rsp = Rsp_Info;
}

 void RomClosed (void)
{
   int i;
   for (i=0; i<0x1000; i++)
     {
	rsp.DMEM[i] = rsp.IMEM[i] = 0;
     }
   init_ucode1();
   init_ucode2();
#ifdef __WIN32__
   firstTime = TRUE;
#endif
}

#endif
