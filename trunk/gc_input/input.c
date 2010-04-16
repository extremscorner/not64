/**
 * Wii64 - input.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Input plugin for GC/Wii
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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

#include <string.h>
#include <stdio.h>

#include "../main/winlnxdefs.h"
#include "InputPlugin.h"
#include "Controller_#1.1.h"
#include "PakIO.h"
#include "controller.h"
#include "../main/wii64config.h"

#ifdef USE_GUI

#endif

static CONTROL_INFO control_info;
static BOOL lastData[4];

virtualControllers_t virtualControllers[4];

controller_t* controller_ts[num_controller_t] =
#if defined(WII) && !defined(NO_BT)
	{ &controller_GC, &controller_Classic,
	  &controller_WiimoteNunchuk,
	 };
#else
	{ &controller_GC,
	 };
#endif

// Use to invoke func on the mapped controller with args
#define DO_CONTROL(Control,func,args...) \
	virtualControllers[Control].control->func( \
		virtualControllers[Control].number, ## args)

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
#if defined(WII) && !defined(NO_BT)
	//Need to switch between Classic and WiimoteNunchuck if user swapped extensions
	if (padType[virtualControllers[Control].number] == PADTYPE_WII)
	{
		if (virtualControllers[Control].control == &controller_Classic &&
			!controller_Classic.available[virtualControllers[Control].number] &&
			controller_WiimoteNunchuk.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_WiimoteNunchuk, virtualControllers[Control].number);
		else if (virtualControllers[Control].control == &controller_WiimoteNunchuk &&
			!controller_WiimoteNunchuk.available[virtualControllers[Control].number] &&
			controller_Classic.available[virtualControllers[Control].number])
			assign_controller(Control, &controller_Classic, virtualControllers[Control].number);
	}
#endif
	if(DO_CONTROL(Control, GetKeys, Keys, virtualControllers[Control].config))
		stop = 1;
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
	int i,t,w;
	control_info = ControlInfo;

	init_controller_ts();

	// 'Initialize' the unmapped virtual controllers
//	for(i=0; i<4; ++i){
//		unassign_controller(i);
//	}

	int num_assigned[num_controller_t];
	memset(num_assigned, 0, sizeof(num_assigned));

	// Map controllers in the priority given
	// Outer loop: virtual controllers
	for(i=0; i<4; ++i){
		// Middle loop: controller type
		for(t=0; t<num_controller_t; ++t){
			controller_t* type = controller_ts[t];
			// Inner loop: which controller
			for(w=num_assigned[t]; w<4 && !type->available[w]; ++w, ++num_assigned[t]);
			// If we've exhausted this type, move on
			if(w == 4) continue;

			assign_controller(i, type, w);
			padType[i] = type == &controller_GC ? PADTYPE_GAMECUBE : PADTYPE_WII;
			padAssign[i] = w;

			// Don't assign the next type over this one or the same controller
			++num_assigned[t];
			break;
		}
		if(t == num_controller_t)
			break;
	}

	// 'Initialize' the unmapped virtual controllers
	for(; i<4; ++i){
		unassign_controller(i);
		padType[i] = PADTYPE_NONE;
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
		if(virtualControllers[i].inUse) DO_CONTROL(i, pause);
}

void resumeInput(void){
	int i;
	for(i=0; i<4; ++i)
		if(virtualControllers[i].inUse) DO_CONTROL(i, resume);
}

void init_controller_ts(void){
	int i;
	for(i=0; i<num_controller_t; ++i)
		controller_ts[i]->init();
}

void assign_controller(int wv, controller_t* type, int wp){
	virtualControllers[wv].control = type;
	virtualControllers[wv].inUse   = 1;
	virtualControllers[wv].number  = wp;
	virtualControllers[wv].config  = type->configs;

	type->assign(wp,wv);

	control_info.Controls[wv].Present = 1;
	if (pakMode[wv] == PAKMODE_MEMPAK)	control_info.Controls[wv].Plugin  = PLUGIN_MEMPAK;
	else								control_info.Controls[wv].Plugin  = PLUGIN_RAW;
}

void unassign_controller(int wv){
	virtualControllers[wv].control = NULL;
	virtualControllers[wv].inUse   = 0;
	virtualControllers[wv].number  = -1;

	control_info.Controls[wv].Present = 0;
	control_info.Controls[wv].Plugin  = PLUGIN_NONE;
}

