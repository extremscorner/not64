/*
 * debugger/regRI.c
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

#include "regRI.h"


static GtkWidget *clRegRI;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32	gui_fantom_reg_RI[5];

static char *mnemonicRI[]=
{
	"RI_MODE_REG",		"RI_CONFIG_REG",
	"RI_CURRENT_LOAD_REG",	"RI_SELECT_REG",
	"RI_REFRESH_REG",
};



//]=-=-=-=-=-=-=-=[ Initialisation of RDRAM Interface Display ]=-=-=-=-=-=-=-=[

void init_regRI()
{
	GtkWidget *boxH1,
			*boxV1,
				*labRegRI[5];
	int i;
	char **txt;
	txt=malloc( sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );


	frRegRI = gtk_frame_new("RDRAM Interface");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frRegRI), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "RI_*_REG" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labRegRI[0] = gtk_label_new( mnemonicRI[0] );
	gtk_box_pack_start( GTK_BOX(boxV1), labRegRI[0], FALSE, TRUE, 1);
	for( i=1; i<5; i++) {
		labRegRI[i] = gtk_label_new( mnemonicRI[i] );
		gtk_box_pack_start( GTK_BOX(boxV1), labRegRI[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ==========/
	clRegRI = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegRI, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegRI), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegRI), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<5; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegRI), txt);
	}

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<5; i++)
	{
		gui_fantom_reg_RI[i] = 0x12345678;
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour RDRAM Interface Display ]=-=-=-=-=-=-=-=-=-[

void update_regRI()
{
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegRI) );

	if( gui_fantom_reg_RI[0] != ri_register.ri_mode )
	{
		gui_fantom_reg_RI[0] = ri_register.ri_mode;
		sprintf( txt, "%.16lX", ri_register.ri_mode );
		gtk_clist_set_text( GTK_CLIST(clRegRI), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegRI), 0, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegRI), 0, &color_ident);
	}

	if( gui_fantom_reg_RI[1] != ri_register.ri_config )
	{
		gui_fantom_reg_RI[1] = ri_register.ri_config;
		sprintf( txt, "%.16lX", ri_register.ri_config );
		gtk_clist_set_text( GTK_CLIST(clRegRI), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegRI), 1, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegRI), 1, &color_ident);
	}

	if( gui_fantom_reg_RI[2] != ri_register.ri_current_load )
	{
		gui_fantom_reg_RI[2] = ri_register.ri_current_load;
		sprintf( txt, "%.16lX", ri_register.ri_current_load );
		gtk_clist_set_text( GTK_CLIST(clRegRI), 2, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegRI), 2, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegRI), 2, &color_ident);
	}

	if( gui_fantom_reg_RI[3] != ri_register.ri_select )
	{
		gui_fantom_reg_RI[3] = ri_register.ri_select;
		sprintf( txt, "%.16lX", ri_register.ri_select );
		gtk_clist_set_text( GTK_CLIST(clRegRI), 3, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegRI), 3, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegRI), 3, &color_ident);
	}

	if( gui_fantom_reg_RI[4] != (uint32) ri_register.ri_refresh )
	{
		gui_fantom_reg_RI[4] = ri_register.ri_refresh;
		sprintf( txt, "%.16lX", ri_register.ri_refresh );
		gtk_clist_set_text( GTK_CLIST(clRegRI), 4, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegRI), 4, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegRI), 4, &color_ident);
	}

	gtk_clist_thaw( GTK_CLIST(clRegRI) );
}
