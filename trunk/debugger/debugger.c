/*
 * debugger/debugger.c
 * 
 * 
 * Debugger for Mupen64 - davFr
 * Copyright (C) 2002 davFr - robind@esiee.fr
 *
 * Mupen64 is copyrighted (C) 2002 Hacktarux
 * Mupen64 homepage: http://mupen64.emulation64.com
 *         email address: hacktarux@yahoo.fr
 * 
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence.
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

#include <glib.h>
#include "debugger.h"

// State of the Emulation Thread:
// 0 -> pause, 2 -> run.
extern int run;


//]=-=-=-=-=-=-=-=-=-=-=[ Initialisation du Debugger ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_debugger()
{
	debugger_mode = 1;
	run = 0;

//]=-=-=[ Initialisation des Couleurs ]=-=-=[
	color_modif.red = 0x0000;
	color_modif.green = 0xA000;
	color_modif.blue = 0xFFFF;

	color_ident.red = 0xFFFF;
	color_ident.green = 0xFFFF;
	color_ident.blue = 0xFFFF;

	init_registers();
	init_desasm();
	init_breakpoints();
	//init_TLBwindow();

	pthread_mutex_init( &mutex, NULL);
	pthread_cond_init( &debugger_done_cond, NULL);
}


//]=-=-=-=-=-=-=-=-=-=-=-=-=[ Mise-a-Jour Debugger ]=-=-=-=-=-=-=-=-=-=-=-=-=[

void update_debugger()
// Update debugger state and display.
// Should be called after each R4300 instruction.
{
	if(run==2) {
		if( check_breakpoints(PC->addr)==-1 ) {
			previousPC = PC->addr;
			return;
		}
		else {
			run = 0;
			switch_button_to_run();
			gdk_beep();
		}
	}

	if(registers_opened) {
		gdk_threads_enter();
		update_registers();
		gdk_threads_leave();
	}	
	if(desasm_opened) {
		gdk_threads_enter();
		update_desasm( PC->addr );
		gdk_threads_leave();
	}
	/*if(regTLB_opened) {
		gdk_threads_enter();
		update_TLBwindow();
		gdk_threads_leave();
	}*/
	previousPC = PC->addr;

	// Emulation thread is blocked until a button is clicked.
	pthread_cond_wait(&debugger_done_cond, &mutex);
}
