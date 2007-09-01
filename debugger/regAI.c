/*
 * debugger/regAI.c
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

#include "regAI.h"


static GtkWidget *clRegAI;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32	gui_fantom_reg_AI[6];

static char *mnemonicAI[]=
{
	"AI_DRAM_ADDR_REG",	"AI_LEN_REG",
	"AI_CONTROL_REG",	"AI_STATUS_REG",
	"AI_DACRATE_REG",	"AI_BITRATE_REG",
};



//]=-=-=-=-=-=-=-=[ Initialisation of Cop0 Registre Display ]=-=-=-=-=-=-=-=-=[
//]=-=-=-=-=-=-=-=[ Initialisation of Audio Interface Display ]=-=-=-=-=-=-=-=[

void init_regAI()
{

	GtkWidget *boxH1,
			*boxV1,
				*labRegAI[6];
	
	int i;
	char **txt;
	txt=malloc( sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );


	frRegAI = gtk_frame_new("Audio Interface");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frRegAI), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "AI_*_REG" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labRegAI[0] = gtk_label_new( mnemonicAI[0] );
	gtk_box_pack_start( GTK_BOX(boxV1), labRegAI[0], FALSE, TRUE, 1);

	for( i=1; i<6; i++)
	{
		labRegAI[i] = gtk_label_new( mnemonicAI[i] );
		gtk_box_pack_start( GTK_BOX(boxV1), labRegAI[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ==========/
	clRegAI = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegAI, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegAI), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegAI), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<6; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegAI), txt);
	}


	//=== Signal Connection ==========================/
//TODO: I plan to enable user to modify cop0 registers 'on-the-fly'.
//	gtk_signal_connect( GTK_OBJECT(clRegCop0), "button_press_event",
//				GTK_SIGNAL_FUNC(on_click), clRegCop0 );

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<6; i++)
	{
		gui_fantom_reg_AI[i] = 0x12345678;
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop0 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_regAI()
{
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegAI) );

	if( gui_fantom_reg_AI[0] != ai_register.ai_dram_addr )
	{
		gui_fantom_reg_AI[0] = ai_register.ai_dram_addr;
		sprintf( txt, "%.16lX", ai_register.ai_dram_addr );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 0, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 0, &color_ident);
	}

	if( gui_fantom_reg_AI[1] != ai_register.ai_len )
	{
		gui_fantom_reg_AI[1] = ai_register.ai_len;
		sprintf( txt, "%.16lX", ai_register.ai_len );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 1, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 1, &color_ident);
	}

	if( gui_fantom_reg_AI[2] != ai_register.ai_control )
	{
		gui_fantom_reg_AI[2] = ai_register.ai_control;
		sprintf( txt, "%.16lX", ai_register.ai_control );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 2, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 2, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 2, &color_ident);
	}

	if( gui_fantom_reg_AI[3] != ai_register.ai_status )
	{
		gui_fantom_reg_AI[3] = ai_register.ai_status;
		sprintf( txt, "%.16lX", ai_register.ai_status );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 3, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 3, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 3, &color_ident);
	}

	if( gui_fantom_reg_AI[4] != (uint32) ai_register.ai_dacrate )
	{
		gui_fantom_reg_AI[4] = ai_register.ai_dacrate;
		sprintf( txt, "%.16lX", ai_register.ai_dacrate );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 4, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 4, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 4, &color_ident);
	}

	if( gui_fantom_reg_AI[5] != (uint32) ai_register.ai_bitrate )
	{
		gui_fantom_reg_AI[5] = ai_register.ai_bitrate;
		sprintf( txt, "%.16lX", ai_register.ai_bitrate );
		gtk_clist_set_text( GTK_CLIST(clRegAI), 5, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegAI), 5, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegAI), 5, &color_ident);
	}

	
	gtk_clist_thaw( GTK_CLIST(clRegAI) );
}
