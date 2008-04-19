/* input.c - Input plugin for gamecube
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include <stdio.h>

#include "../main/winlnxdefs.h"
#include "InputPlugin.h"
#include "Controller_#1.1.h"
#include "PakIO.h"
#include "controller.h"

#ifdef USE_GUI

#endif
 
static CONTROL_INFO control_info;
static BOOL lastData[4];

static struct {
	BOOL          inUse;
	controller_t* control;
	int           number;
} controllers[4];

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	controllers[Control].control->func(controllers[Control].number, ## args)

unsigned char mempack_crc(unsigned char *data);

static BYTE writePak(int Control, BYTE* Command){
	// From N-Rage Plugin by Norbert Wladyka
	BYTE* data = &Command[2];
	unsigned int dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);
	
	if( dwAddress == PAK_IO_RUMBLE ){
		DO_CONTROL(Control, rumble, *data);
	} else if( dwAddress >= 0x8000 && dwAddress < 0x9000 ){
		lastData[Control] = (*data) ? TRUE : FALSE;
	}
	
	data[32] = mempack_crc(data);
	return RD_OK;
}

static BYTE readPak(int Control, BYTE* Command){
	// From N-Rage Plugin by Norbert Wladyka
	BYTE* data = &Command[2];
	unsigned int dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);
	
	int i;
	if( ((dwAddress >= 0x8000) && (dwAddress < 0x9000)) && lastData[Control] )
		for(i=0; i<32; ++i) data[i] = 0x80;
	else
		for(i=0; i<32; ++i) data[i] = 0;
	
	data[32] = mempack_crc(data);
	return RD_OK;
}

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
	// We don't need to handle this because apparently
	//   a call to ReadController immediately follows
	return;
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
#ifdef USE_GUI
   
#else
	char s[] = "Input plugin for Mupen64 emulator for GC\n\tby Mike Slegeir\n";
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
	if( DO_CONTROL(Control, GetKeys, Keys) ) stop = 1;
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
	
	// FIXME: This needs to work for all controller_t's
	extern controller_t controller_GC;
	
	controller_GC.init();
	int i;
	for(i=0; i<4; ++i){
		// TODO: Use controller type priorities
		controllers[i].control = &controller_GC;
		controllers[i].inUse   = controller_GC.available[i];
		controllers[i].number  = i;
		control_info.Controls[i].Present = controllers[i].inUse;
		control_info.Controls[i].Plugin  = PLUGIN_MEMPAK;
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
	if(Control < 0 || !Command) return;
	
	// From N-Rage Plugin by Norbert Wladyka
	switch(Command[2]){
	case RD_RESETCONTROLLER:
	case RD_GETSTATUS:
		// expected: controller gets 1 byte (command), controller sends back 3 bytes
		// should be:	Command[0] == 0x01
		//				Command[1] == 0x03
		Command[3] = RD_GAMEPAD | RD_ABSOLUTE;
		Command[4] = RD_NOEEPROM;
		if(control_info.Controls[Control].Present)
			Command[5] = ( control_info.Controls[Control].Plugin != PLUGIN_NONE )
			              ? RD_PLUGIN : RD_NOPLUGIN;
		else
			Command[5] = RD_NOPLUGIN | RD_NOTINITIALIZED;
		break;
	case RD_READKEYS:
		// I don't think I should be getting this command
		//   but just in case
		GetKeys(Control, (BUTTONS*)&Command[3]);
		break;
	case RD_READPAK:
		readPak(Control, &Command[3]);
		break;
	case RD_WRITEPAK:
		writePak(Control, &Command[3]);
		break;
	case RD_READEEPROM:
		// Should be handled by the Emulator
		break;
	case RD_WRITEEPROM:
		// Should be handled by the Emulator
		break;
	default:
		// only accessible if the Emulator has bugs.. or maybe the Rom is flawed
		Command[1] = Command[1] | RD_ERROR;
	}
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
void pauseInput(void){
	int i;
	for(i=0; i<4; ++i)
		if(controllers[i].inUse) DO_CONTROL(i, pause);
}

void resumeInput(void){
	int i;
	for(i=0; i<4; ++i)
		if(controllers[i].inUse) DO_CONTROL(i, resume);
}

