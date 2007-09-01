/*
 * debugger/regSI.c
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

#include "regSI.h"


static GtkWidget *clRegSI;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32	gui_fantom_reg_SI[4];

static char *mnemonicSI[]=
{
	"SI_DRAM_ADDR_REG",		"SI_PIF_ADDR_RD64B_REG",
	"SI_PIF_ADDR_WR64B_REG",	"SI_STATUS_REG",
};



//]=-=-=-=-=-=-=-=[ Initialisation of Serial Interface Display ]=-=-=-=-=-=-=-[

void init_regSI()
{
	GtkWidget *boxH1,
			*boxV1,
				*labRegSI[32];
	int i;
	char **txt;
	txt=malloc( sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );


	frRegSI = gtk_frame_new("Serial Interface");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frRegSI), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "SD_*_REG" Column =======/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labRegSI[0] = gtk_label_new( mnemonicSI[0] );
	gtk_box_pack_start( GTK_BOX(boxV1), labRegSI[0], FALSE, TRUE, 1);
	for( i=1; i<4; i++)
	{
		labRegSI[i] = gtk_label_new( mnemonicSI[i] );
		gtk_box_pack_start( GTK_BOX(boxV1), labRegSI[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ========/
	clRegSI = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegSI, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegSI), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegSI), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<4; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegSI), txt);
	}

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<4; i++)
	{
		gui_fantom_reg_SI[i] = 0x12345678;
		//Should be put to the least probable value.
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Serial Interface Display ]=-=-=-=-=-=-=-=-=[

void update_regSI()
{
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegSI) );

	if( gui_fantom_reg_SI[0] != si_register.si_dram_addr ) {
		gui_fantom_reg_SI[0] = si_register.si_dram_addr;
		sprintf( txt, "%.16lX", si_register.si_dram_addr);
		gtk_clist_set_text( GTK_CLIST(clRegSI), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegSI), 0, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegSI), 0, &color_ident);
	}

	if( gui_fantom_reg_SI[1] != si_register.si_pif_addr_rd64b )
	{
		gui_fantom_reg_SI[1] = si_register.si_pif_addr_rd64b;
		sprintf( txt, "%.16lX", si_register.si_pif_addr_rd64b);
		gtk_clist_set_text( GTK_CLIST(clRegSI), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegSI), 1, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegSI), 1, &color_ident);
	}

	if( gui_fantom_reg_SI[2] != si_register.si_pif_addr_wr64b )
	{
		gui_fantom_reg_SI[2] = si_register.si_pif_addr_wr64b;
		sprintf( txt, "%.16lX", si_register.si_pif_addr_wr64b);
		gtk_clist_set_text( GTK_CLIST(clRegSI), 2, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegSI), 2, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegSI), 2, &color_ident);
	}


	if( gui_fantom_reg_SI[3] != si_register.si_status )
	{
		gui_fantom_reg_SI[3] = si_register.si_status;
		sprintf( txt, "%.16lX", si_register.si_status );
		gtk_clist_set_text( GTK_CLIST(clRegSI), 3, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegSI), 3, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegSI), 3, &color_ident);
	}

	gtk_clist_thaw( GTK_CLIST(clRegSI) );
}
