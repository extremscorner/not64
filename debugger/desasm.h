/*
 * debugger/desasm.h
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


#ifndef DESASM_H
#define DESASM_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>

#include <glib.h>

#include "types.h"
#include "../r4300/r4300.h"
#include "../memory/memory.h"

#include "debugger.h"


int desasm_opened;

extern pthread_t thread_n64;

GtkWidget *winDesasm;

void init_desasm();
int get_instruction( uint32 address, uint32 *ptr_instruction );
void update_desasm( uint32 focused_address );
void update_desasm_color( uint32 address );

void switch_button_to_run();
#endif	// DESASM_H
