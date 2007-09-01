/*
 * debugger/regCop0.c
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

#include "regCop0.h"


static GtkWidget *clRegCop0;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static sint64	gui_fantom_reg_cop0[32];

static char *mnemonicCop0[]=
{
	"Index",	"Random",	"EntryLo0",	"EntryLo1",
	"Context",	"PageMask",	"Wired",	"----",
	"BadVAddr",	"Count",	"EntryHi",	"Compare",
	"SR",		"Cause",	"EPC",		"PRid",
	"Config",	"LLAddr",	"WatchLo",	"WatchHi",
	"Xcontext",	"----",		"----",		"----",
	"----",		"----",		"PErr",		"CacheErr",
	"TagLo",	"TagHi",	"ErrorEPC",	"----",
};



//]=-=-=-=-=-=-=-=[ Initialisation of Cop0 Registre Display ]=-=-=-=-=-=-=-=-=[

void init_regCop0()
{

	GtkWidget *boxH1,
			*boxV1,
				*labRegCop0[32],
			*boxV2,
				*labMneCop0[32];
	
	int i;
	char **txt;
	txt=malloc( 2*sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );
	txt[1]=malloc( 64*sizeof(char) );


	regCop0_opened = 1;

	
	frCop0 = gtk_frame_new("cop0");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frCop0), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "regXX" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	sprintf( txt[0], "reg%d", 0);
	labRegCop0[0]=gtk_label_new( txt[0] );
	gtk_label_set_justify( GTK_LABEL(labRegCop0[0]), GTK_JUSTIFY_RIGHT);
	gtk_box_pack_start( GTK_BOX(boxV1), labRegCop0[0], FALSE, TRUE, 1);
	//
	for( i=1; i<32; i++)
	{
		sprintf( txt[0], "reg%d", i);
		labRegCop0[i]=gtk_label_new( txt[0] );
		gtk_label_set_justify( GTK_LABEL(labRegCop0[i]), GTK_JUSTIFY_RIGHT);
		gtk_box_pack_start( GTK_BOX(boxV1), labRegCop0[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ==========/
	clRegCop0 = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegCop0, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegCop0), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegCop0), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<32; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegCop0), txt);
	}

	//=== Creation of Mnemonics Column ================/
	boxV2 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV2, FALSE, FALSE, 0);

	labMneCop0[0]=gtk_label_new( mnemonicCop0[0] );
	gtk_box_pack_start( GTK_BOX(boxV2), labMneCop0[0], FALSE, TRUE, 1);

	for( i=1; i<32; i++)
	{
		labMneCop0[i]=gtk_label_new( mnemonicCop0[i] );
		gtk_box_pack_start( GTK_BOX(boxV2), labMneCop0[i], FALSE, TRUE, 0);
	}

	//=== Signal Connection ==========================/
//TODO: I plan to enable user to modify cop0 registers 'on-the-fly'.

//	gtk_signal_connect( GTK_OBJECT(clRegCop0), "button_press_event",
//				GTK_SIGNAL_FUNC(on_click), clRegCop0 );

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<32; i++)
	{
		gui_fantom_reg_cop0[i] = 0x1234567890LL;
		//Should be put to the least probable value.
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop0 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_regCop0()
{
	int i;
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegCop0) );

	for(i=0; i<32; i++)
	{
		if( gui_fantom_reg_cop0[i] != reg_cop0[i] )
		{
			gui_fantom_reg_cop0[i] = reg_cop0[i];
			sprintf( txt, "%.16llX", (long long)reg_cop0[i] );
			gtk_clist_set_text( GTK_CLIST(clRegCop0), i, 0, txt );
			gtk_clist_set_background( GTK_CLIST(clRegCop0), i, &color_modif);
		}
		else
		{
			gtk_clist_set_background( GTK_CLIST(clRegCop0), i, &color_ident);
		}
	}

	gtk_clist_thaw( GTK_CLIST(clRegCop0) );
}
