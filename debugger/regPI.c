/*
 * debugger/regPI.c
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

#include "regPI.h"


static GtkWidget *clRegPI;


// We keep a copy of values displayed to screen.
// Screen is only updated when values REALLY changed, and display need an
// update. It makes things go really faster :)
static uint32	gui_fantom_reg_PI[13];

static char *mnemonicPI[]=
{
	"PI_DRAM_ADDR_REG",	"PI_CART_ADDR_REG",
	"PI_RD_LEN_REG",	"PI_WR_LEN_REG",
	"PI_STATUS_REG",
	"PI_BSD_DOM1_LAT_REG",	"PI_BSD_DOM1_PWD_REG",
	"PI_BSD_DOM1_PGS_REG",	"PI_BSD_DOM1_RLS_REG",
	"PI_BSD_DOM2_LAT_REG",	"PI_BSD_DOM2_PWD_REG",
	"PI_BSD_DOM2_PGS_REG",	"PI_BSD_DOM2_RLS_REG",
};



//]=-=-=-=-=-=-=-=[ Initialisation of Peripheral Interface Display ]=-=-=-=-=-[

void init_regPI()
{
	GtkWidget *boxH1,
			*boxV1,
				*labRegPI[13];
	int i;
	char **txt;
	txt=malloc( sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );


	frRegPI = gtk_frame_new("Video Interface");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frRegPI), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5);

	//=== Creation of Labels "PI_*_REG" Column ========/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labRegPI[0] = gtk_label_new( mnemonicPI[0] );
	gtk_box_pack_start( GTK_BOX(boxV1), labRegPI[0], FALSE, TRUE, 1);
	for( i=1; i<13; i++)
	{
		labRegPI[i] = gtk_label_new( mnemonicPI[i] );
		gtk_box_pack_start( GTK_BOX(boxV1), labRegPI[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display =========/
	clRegPI = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clRegPI, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clRegPI), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clRegPI), 0, 130);
	strcpy( txt[0], "Undefined");
	for( i=0; i<13; i++)
	{
		gtk_clist_append( GTK_CLIST(clRegPI), txt);
	}

	//=== Fantom Registers Initialisation =============/
	for( i=0; i<13; i++)
	{
		gui_fantom_reg_PI[i] = 0x12345678;
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Peripheral Interface Display ]=-=-=-=-=-=-=[

void update_regPI()
{
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clRegPI) );

	if( gui_fantom_reg_PI[0] != pi_register.pi_dram_addr_reg )
	{
		gui_fantom_reg_PI[0] = pi_register.pi_dram_addr_reg;
		sprintf( txt, "%.16lX", pi_register.pi_dram_addr_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 0, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 0, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 0, &color_ident);
	}

	if( gui_fantom_reg_PI[1] != pi_register.pi_cart_addr_reg )
	{
		gui_fantom_reg_PI[1] = pi_register.pi_cart_addr_reg;
		sprintf( txt, "%.16lX", pi_register.pi_cart_addr_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 1, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 1, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 1, &color_ident);
	}

	if( gui_fantom_reg_PI[2] != pi_register.pi_rd_len_reg )
	{
		gui_fantom_reg_PI[2] = pi_register.pi_rd_len_reg;
		sprintf( txt, "%.16lX", pi_register.pi_rd_len_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 2, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 2, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 2, &color_ident);
	}

	if( gui_fantom_reg_PI[3] != pi_register. pi_wr_len_reg )
	{
		gui_fantom_reg_PI[3] = pi_register. pi_wr_len_reg;
		sprintf( txt, "%.16lX", pi_register. pi_wr_len_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 3, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 3, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 3, &color_ident);
	}

	if( gui_fantom_reg_PI[4] != (uint32) pi_register.read_pi_status_reg )
	{
		gui_fantom_reg_PI[4] = pi_register.read_pi_status_reg;
		sprintf( txt, "%.16lX", pi_register.read_pi_status_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 4, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 4, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 4, &color_ident);
	}

	if( gui_fantom_reg_PI[5] != (uint32) pi_register.pi_bsd_dom1_lat_reg )
	{
		gui_fantom_reg_PI[5] = pi_register.pi_bsd_dom1_lat_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom1_lat_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 5, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 5, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 5, &color_ident);
	}
	
	if( gui_fantom_reg_PI[6] != (uint32) pi_register.pi_bsd_dom1_pwd_reg )
	{
		gui_fantom_reg_PI[6] = pi_register.pi_bsd_dom1_pwd_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom1_pwd_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 6, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 6, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 6, &color_ident);
	}

	if( gui_fantom_reg_PI[7] != (uint32) pi_register.pi_bsd_dom1_pgs_reg )
	{
		gui_fantom_reg_PI[7] = pi_register.pi_bsd_dom1_pgs_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom1_pgs_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 7, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 7, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 7, &color_ident);
	}

	if( gui_fantom_reg_PI[8] != (uint32) pi_register.pi_bsd_dom1_rls_reg )
	{
		gui_fantom_reg_PI[8] = pi_register.pi_bsd_dom1_rls_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom1_rls_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 8, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 8, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 8, &color_ident);
	}

	if( gui_fantom_reg_PI[9] != (uint32) pi_register.pi_bsd_dom2_lat_reg )
	{
		gui_fantom_reg_PI[9] = pi_register.pi_bsd_dom2_lat_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom2_lat_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 9, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 9, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 9, &color_ident);
	}

	if( gui_fantom_reg_PI[10] != (uint32) pi_register.pi_bsd_dom2_pwd_reg )
	{
		gui_fantom_reg_PI[10] = pi_register.pi_bsd_dom2_pwd_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom2_pwd_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 10, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 10, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 10, &color_ident);
	}

	if( gui_fantom_reg_PI[11] != (uint32) pi_register.pi_bsd_dom2_pgs_reg )
	{
		gui_fantom_reg_PI[11] = pi_register.pi_bsd_dom2_pgs_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom2_pgs_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 11, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 11, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 11, &color_ident);
	}

	if( gui_fantom_reg_PI[12] != (uint32) pi_register.pi_bsd_dom2_rls_reg )
	{
		gui_fantom_reg_PI[12] = pi_register.pi_bsd_dom2_rls_reg;
		sprintf( txt, "%.16lX", pi_register.pi_bsd_dom2_rls_reg );
		gtk_clist_set_text( GTK_CLIST(clRegPI), 12, 0, txt );
		gtk_clist_set_background( GTK_CLIST(clRegPI), 12, &color_modif);
	} else {
		gtk_clist_set_background( GTK_CLIST(clRegPI), 12, &color_ident);
	}

	gtk_clist_thaw( GTK_CLIST(clRegPI) );
}
