/*
 * debugger/regVI.c
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

#include "regVI.h"


static GtkWidget *clRegVI;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32	gui_fantom_reg_VI[15];

static char *mnemonicVI[]=
{
	"VI_STATUS_REG",	"VI_DRAM_ADDR_REG",
	"VI_WIDTH_REG",		"VI_INTR_REG",
	"VI_CURRENT_LINE_REG",	"VI_TIMING_REG",
	"VI_V_SYNC_REG",	"VI_H_SYNC_REG",	"VI_H_SYNC_LEAP_REG",
	"VI_H_START_REG",	"VI_V_START_REG",
	"VI_V_BURST_REG",
	"VI_X_SCALE_REG",	"VI_Y_SCALE_REG",
	"OsTvType",
};



//]=-=-=-=-=-=-=[ Initialisation of Video Interface Display ]=-=-=-=-=-=-=-=-=[

void init_regVI()
{
	GtkWidget *boxH1,
			*boxV1,
				*labRegVI[15];
	int i;
	char **txt;
	txt=malloc( sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );


	frRegVI = gtk_frame_new("Video Interface");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frRegVI), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "VI_*_REG" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labRegVI[0] = gtk_label_new( mnemonicVI[0] );
	gtk_box_pack_start( GTK_BOX(boxV1), labRegVI[0], FALSE, TRUE, 1);
	for( i=1; i<15; i++)
	{
		labRegVI[i] = gtk_label_new( mnemonicVI[i] );
		gtk_box_pack_start( GTK_BOX(boxV1), labRegVI[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ==========/
	clRegVI = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegVI, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegVI), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegVI), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<15; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegVI), txt);
	}

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<15; i++)
	{
		gui_fantom_reg_VI[i] = 0x12345678;
	}
}



//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Video Interface Display ]=-=-=-=-=-=-=-=--=[

void update_regVI()
{
	//TODO: To be rewritten.
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegVI) );

	if( gui_fantom_reg_VI[0] != vi_register.vi_status )
	{
		gui_fantom_reg_VI[0] = vi_register.vi_status;
		sprintf( txt, "%.16lX", vi_register.vi_status );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 0, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 0, &color_ident);
	}

	if( gui_fantom_reg_VI[1] != vi_register.vi_origin )
	{
		gui_fantom_reg_VI[1] = vi_register.vi_origin;
		sprintf( txt, "%.16lX", vi_register.vi_origin );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 1, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 1, &color_ident);
	}

	if( gui_fantom_reg_VI[2] != vi_register.vi_width )
	{
		gui_fantom_reg_VI[2] = vi_register.vi_width;
		sprintf( txt, "%.16lX", vi_register.vi_width );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 2, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 2, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 2, &color_ident);
	}

	if( gui_fantom_reg_VI[3] != vi_register.vi_v_intr )
	{
		gui_fantom_reg_VI[3] = vi_register.vi_v_intr;
		sprintf( txt, "%.16lX", vi_register.vi_v_intr );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 3, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 3, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 3, &color_ident);
	}

	if( gui_fantom_reg_VI[4] != (uint32) vi_register.vi_current )
	{
		gui_fantom_reg_VI[4] = vi_register.vi_current;
		sprintf( txt, "%.16lX", vi_register.vi_current );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 4, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 4, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 4, &color_ident);
	}

	if( gui_fantom_reg_VI[5] != (uint32) vi_register.vi_burst )
	{
		gui_fantom_reg_VI[5] = vi_register.vi_burst;
		sprintf( txt, "%.16lX", vi_register.vi_burst );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 5, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 5, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 5, &color_ident);
	}
	
	if( gui_fantom_reg_VI[6] != (uint32) vi_register.vi_v_sync )
	{
		gui_fantom_reg_VI[6] = vi_register.vi_v_sync;
		sprintf( txt, "%.16lX", vi_register.vi_v_sync );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 6, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 6, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 6, &color_ident);
	}

	if( gui_fantom_reg_VI[7] != (uint32) vi_register.vi_h_sync )
	{
		gui_fantom_reg_VI[7] = vi_register.vi_h_sync;
		sprintf( txt, "%.16lX", vi_register.vi_h_sync );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 7, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 7, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 7, &color_ident);
	}

	if( gui_fantom_reg_VI[8] != (uint32) vi_register.vi_leap )
	{
		gui_fantom_reg_VI[8] = vi_register.vi_leap;
		sprintf( txt, "%.16lX", vi_register.vi_leap );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 8, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 8, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 8, &color_ident);
	}

	if( gui_fantom_reg_VI[9] != (uint32) vi_register.vi_h_start )
	{
		gui_fantom_reg_VI[9] = vi_register.vi_h_start;
		sprintf( txt, "%.16lX", vi_register.vi_h_start );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 9, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 9, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 9, &color_ident);
	}

	if( gui_fantom_reg_VI[10] != (uint32) vi_register.vi_v_start )
	{
		gui_fantom_reg_VI[10] = vi_register.vi_v_start;
		sprintf( txt, "%.16lX", vi_register.vi_v_start );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 10, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 10, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 10, &color_ident);
	}

	if( gui_fantom_reg_VI[11] != (uint32) vi_register.vi_v_burst )
	{
		gui_fantom_reg_VI[11] = vi_register.vi_v_burst;
		sprintf( txt, "%.16lX", vi_register.vi_v_burst );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 11, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 11, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 11, &color_ident);
	}

	if( gui_fantom_reg_VI[12] != (uint32) vi_register.vi_x_scale )
	{
		gui_fantom_reg_VI[12] = vi_register.vi_x_scale;
		sprintf( txt, "%.16lX", vi_register.vi_x_scale);
		gtk_clist_set_text( GTK_CLIST(clRegVI), 12, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 12, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 12, &color_ident);
	}

	if( gui_fantom_reg_VI[13] != (uint32) vi_register.vi_y_scale )
	{
		gui_fantom_reg_VI[13] = vi_register.vi_y_scale;
		sprintf( txt, "%.16lX", vi_register.vi_y_scale);
		gtk_clist_set_text( GTK_CLIST(clRegVI), 13, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 13, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 13, &color_ident);
	}

	if( gui_fantom_reg_VI[14] != rdram[0x300/4] )
	{
		gui_fantom_reg_VI[14] = rdram[0x300/4];
		sprintf( txt, "%.16lX", rdram[0x300/4] );
		gtk_clist_set_text( GTK_CLIST(clRegVI), 14, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegVI), 14, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegVI), 14, &color_ident);
	}

	gtk_clist_thaw( GTK_CLIST(clRegVI) );
}
