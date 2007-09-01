/* input.c - Input plugin for gamecube
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include <ogc/pad.h>

#include "../main/winlnxdefs.h"
#include "InputPlugin.h"
#include "Controller_#1.1.h"

#ifdef USE_GUI

#endif

static CONTROL_INFO control_info;

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL CloseDLL (void)
{
}

/******************************************************************
  Function: ControllerCommand
  Purpose:  To process the raw data that has just been sent to a 
            specific controller.
  input:    - Controller Number (0 to 3) and -1 signalling end of 
              processing the pif ram.
			- Pointer of data to be processed.
  output:   none
  
  note:     This function is only needed if the DLL is allowing raw
            data, or the plugin is set to raw

            the data that is being processed looks like this:
            initilize controller: 01 03 00 FF FF FF 
            read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand ( int Control, BYTE * Command)
{
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllAbout ( HWND hParent )
{
   char s[] = "Input plugin for Mupen64 emulator for GC\n\tby Mike Slegeir\n";
#ifdef USE_GUI
   
#else
   printf(s);
#endif
}

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllConfig ( HWND hParent )
{
}

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllTest ( HWND hParent )
{
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
   PluginInfo->Version = 0x0101;
   PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
   strcpy(PluginInfo->Name, "Gamecube input plugin");
}

/******************************************************************
  Function: GetKeys
  Purpose:  To get the current state of the controllers buttons.
  input:    - Controller Number (0 to 3)
            - A pointer to a BUTTONS structure to be filled with 
			the controller state.
  output:   none
*******************************************************************/  
extern int stop;	
EXPORT void CALL GetKeys(int Control, BUTTONS * Keys )
{
	BUTTONS* c = Keys;
		
	int b = PAD_ButtonsHeld(Control);
	c->R_DPAD       = (b & PAD_BUTTON_RIGHT) ? 1 : 0;
	c->L_DPAD       = (b & PAD_BUTTON_LEFT)  ? 1 : 0;
	c->D_DPAD       = (b & PAD_BUTTON_DOWN)  ? 1 : 0;
	c->U_DPAD       = (b & PAD_BUTTON_UP)    ? 1 : 0;
	c->START_BUTTON = (b & PAD_BUTTON_START) ? 1 : 0;
	c->B_BUTTON     = (b & PAD_BUTTON_B)     ? 1 : 0;
	c->A_BUTTON     = (b & PAD_BUTTON_A)     ? 1 : 0;

	c->Z_TRIG       = (b & PAD_TRIGGER_Z)    ? 1 : 0;
	c->R_TRIG       = (b & PAD_TRIGGER_R)    ? 1 : 0;
	c->L_TRIG       = (b & PAD_TRIGGER_L)    ? 1 : 0;

	// FIXME: Proper values for analog and C-Stick
	s8 substickX = PAD_SubStickX(Control);
	c->R_CBUTTON    = (substickX >  5)       ? 1 : 0;
	c->L_CBUTTON    = (substickX < -5)       ? 1 : 0;
	s8 substickY = PAD_SubStickY(Control);
	c->D_CBUTTON    = (substickY >  5)       ? 1 : 0;
	c->U_CBUTTON    = (substickY < -5)       ? 1 : 0;
	
	c->X_AXIS       = PAD_StickX(Control);
	c->Y_AXIS       = PAD_StickY(Control);
	
	// In pure interpreter mode X+Y quits
	if((b & PAD_BUTTON_X) && (b & PAD_BUTTON_Y)) stop = 1;
	
}

/******************************************************************
  Function: InitiateControllers
  Purpose:  This function initialises how each of the controllers 
            should be handled.
  input:    - The handle to the main window.
            - A controller structure that needs to be filled for 
			  the emulator to know how to handle each controller.
  output:   none
*******************************************************************/  
EXPORT void CALL InitiateControllers (CONTROL_INFO ControlInfo)
{
	control_info = ControlInfo;
	
	PADStatus status[4];
	PAD_Read(status);
	int i;
	for(i=0; i<4; ++i){
		// Check if controller is plugged
		control_info.Controls[i].Present = 
			(status[i].err == PAD_ERR_NO_CONTROLLER) ?
					FALSE : TRUE;
		printf("Controller %d is %s\n", i,
		       control_info.Controls[i].Present ? "plugged in" : "unplugged");
		// TODO: Support rumble
		control_info.Controls[i].Plugin = PLUGIN_MEMPAK;
	}
}

/******************************************************************
  Function: ReadController
  Purpose:  To process the raw data in the pif ram that is about to
            be read.
  input:    - Controller Number (0 to 3) and -1 signalling end of 
              processing the pif ram.
			- Pointer of data to be processed.
  output:   none  
  note:     This function is only needed if the DLL is allowing raw
            data.
*******************************************************************/
EXPORT void CALL ReadController ( int Control, BYTE * Command )
{
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomClosed (void)
{
	
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the 
            emulation thread)
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomOpen (void)
{
	PAD_Init(); // FIXME: This is probably done in main
}

/******************************************************************
  Function: WM_KeyDown
  Purpose:  To pass the WM_KeyDown message from the emulator to the 
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/ 
EXPORT void CALL WM_KeyDown( WPARAM wParam, LPARAM lParam )
{
}
/******************************************************************
  Function: WM_KeyUp
  Purpose:  To pass the WM_KEYUP message from the emulator to the 
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/ 
EXPORT void CALL WM_KeyUp( WPARAM wParam, LPARAM lParam )
{
}

