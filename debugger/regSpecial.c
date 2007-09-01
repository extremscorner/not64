/*
 * debugger/regSpecial.c
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

#include "regSpecial.h"

static sint64	gui_fantom_hi,
		gui_fantom_lo,
		gui_fantom_nextint;


static GtkWidget	*frNextInt,
				*enNextInt,
			*frRegPC,
				*enRegPC,
			*frRegPreviousPC,
				*enRegPreviousPC,
			*frRegHiLo,
				*clRegHiLo;


//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Registers Display ]=-=-=-=-=-=-=-=-=-=-==[

void init_regHiLo()
{
	int i;
	char **txt=malloc(sizeof(char*));
	txt[0]=malloc(16*sizeof(char));

	frRegHiLo = gtk_frame_new("Hi/Lo");
	
	clRegHiLo = gtk_clist_new(1);
	gtk_container_add( GTK_CONTAINER(frRegHiLo), clRegHiLo );
	strcpy( txt[0], "Undefined" );
	for(i=0;i<2;i++)
	{
		gtk_clist_append( GTK_CLIST(clRegHiLo), txt );
	}
	gtk_clist_set_column_width( GTK_CLIST(clRegHiLo), i, 130);

	gui_fantom_hi=0x1234567890LL;
	gui_fantom_lo=0x1234567890LL;
}

void init_regPC()
{
	frRegPC = gtk_frame_new("PC");

	enRegPC = gtk_entry_new();
	gtk_container_add( GTK_CONTAINER(frRegPC), enRegPC );
	gtk_entry_set_editable( GTK_ENTRY (enRegPC), FALSE);
	gtk_entry_set_text(GTK_ENTRY (enRegPC), "Undefined");
}

void init_regPreviousPC()
{
	frRegPreviousPC = gtk_frame_new("Previous PC");

	enRegPreviousPC = gtk_entry_new();
	gtk_container_add( GTK_CONTAINER(frRegPreviousPC), enRegPreviousPC );
	gtk_entry_set_editable( GTK_ENTRY (enRegPreviousPC), FALSE);
	gtk_entry_set_text(GTK_ENTRY (enRegPreviousPC), "Undefined");
}

void init_nextint()
{
	frNextInt = gtk_frame_new("Next Interrupt");

	enNextInt = gtk_entry_new();
	gtk_entry_set_editable( GTK_ENTRY (enNextInt), FALSE);
	gtk_entry_set_text( GTK_ENTRY (enNextInt), "Undefined");
	gtk_container_add( GTK_CONTAINER(frNextInt), enNextInt );

	gui_fantom_nextint = 0x12345678;
}

void init_regSpecial()
{
	GtkWidget	*boxV1;
	
	regSpecial_opened = 1;

	frRegSpecial = gtk_frame_new("Special Registers");

	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_container_add( GTK_CONTAINER(frRegSpecial), boxV1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxV1), 2 );

	init_regPC();
	gtk_box_pack_start( GTK_BOX (boxV1), frRegPC, FALSE, FALSE, 10);

	init_regPreviousPC();
	gtk_box_pack_start( GTK_BOX (boxV1), frRegPreviousPC, FALSE, FALSE, 10);
	
	init_regHiLo();
	gtk_box_pack_start( GTK_BOX (boxV1), frRegHiLo, FALSE, FALSE, 10);

	init_nextint();
	gtk_box_pack_start( GTK_BOX (boxV1), frNextInt, FALSE, FALSE, 10);
}




//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void update_regHiLo()
{
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegHiLo) );
	
	if( gui_fantom_hi != hi ) {
		gui_fantom_hi = hi;
		sprintf( txt, "%.16llX", gui_fantom_hi );
		gtk_clist_set_text( GTK_CLIST(clRegHiLo), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegHiLo), 0, &color_modif);
	}
	else
		gtk_clist_set_background( GTK_CLIST(clRegHiLo), 0, &color_ident);

	if(gui_fantom_lo!=lo) {
		gui_fantom_lo=lo;
		sprintf(txt, "%.16llX", gui_fantom_lo );
		gtk_clist_set_text( GTK_CLIST(clRegHiLo), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegHiLo), 0, &color_modif);
	}
	else
		gtk_clist_set_background( GTK_CLIST(clRegHiLo), 1, &color_ident);

	gtk_clist_thaw( GTK_CLIST(clRegHiLo) );
}

void update_regPC()
{
	uint32	instr;
	char txt[128];

	get_instruction( PC->addr, &instr);
	sprintf(txt, "%.16lX: 0x%.8lX", PC->addr, instr );
	gtk_entry_set_text(GTK_ENTRY (enRegPC), txt );
}

void update_regPreviousPC()
{
	char txt[128];

	//TODO: print binary of the instruction.
	//sprintf(txt, "%.16lX: 0x%.8lX", PC->addr, PC->instr );
	sprintf(txt, "%.16lX", previousPC );
	gtk_entry_set_text(GTK_ENTRY (enRegPreviousPC), txt );
}

void update_nextint()
{
	char txt[24];

	if( gui_fantom_nextint != next_interupt )
	{
		gui_fantom_nextint = next_interupt;
		sprintf( txt, "0%.16X", next_interupt );
		gtk_entry_set_text( GTK_ENTRY(enNextInt), txt );
	}
}

void update_regSpecial()
{
	update_regPC();
	update_regPreviousPC();
	update_regHiLo();
	update_nextint();
}
