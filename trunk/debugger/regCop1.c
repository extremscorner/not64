/*
 * debugger/regCop1.c
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

#include "regCop1.h"

static float	gui_fantom_simple[32];
static double	gui_fantom_double[32];

GtkWidget	*clFGR;
GtkWidget	*clFGR2;

//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void init_FGR()
{
	GtkWidget *boxH1,
			*boxV1,
				*labFGR[32];
	
	int i;
	char **txt;
	txt=malloc( 2*sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );
	txt[1]=malloc( 64*sizeof(char) );


	FGR_opened = 1;

	frFGR = gtk_frame_new("cop1");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frFGR), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5 );

	//=== Creation of Labels "regXX" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	sprintf( txt[0], "fgr%d", 0);
	labFGR[0] = gtk_label_new( txt[0] );
	gtk_label_set_justify( GTK_LABEL(labFGR[0]), GTK_JUSTIFY_RIGHT );
	gtk_box_pack_start( GTK_BOX(boxV1), labFGR[0], FALSE, FALSE, 1 );
	for( i=1; i<32; i++)
	{
		sprintf( txt[0], "fgr%d", i);
		labFGR[i]=gtk_label_new( txt[0] );
		gtk_label_set_justify( GTK_LABEL(labFGR[i]), GTK_JUSTIFY_RIGHT);
		gtk_box_pack_start( GTK_BOX(boxV1), labFGR[i], FALSE, FALSE, 0);
	}

	//==== Simple Precision Registers Display ============/
	clFGR = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clFGR, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clFGR), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clFGR), 0, 130);
	strcpy( txt[0], "Undefined" );
	for( i=0; i<32; i++)
	{
		gtk_clist_append( GTK_CLIST(clFGR), txt);
	}

	//==== Double Precision Registers Display ============/
	clFGR2 = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clFGR2, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clFGR2), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clFGR2), 0, 130);
	strcpy( txt[0], "Undefined" );
	for( i=0; i<32; i++)
	{
		gtk_clist_append( GTK_CLIST(clFGR2), txt);
	}

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<32; i++) {
		gui_fantom_simple[i] = 1,2345678; // Some improbable value
	}
	for( i=0; i<32; i++) {
		gui_fantom_double[i] = 9,8765432;
	}
}




//]=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Cop1 Registers Display ]=-=-=-=-=-=-=-=-=-=[

void update_FGR()
{
	int i;
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clFGR) );
	for(i=0; i<32; i++) {
		if( gui_fantom_simple[i]!= *reg_cop1_simple[i] )
		{
			gui_fantom_simple[i] = *reg_cop1_simple[i];
			sprintf(txt, "%f", *reg_cop1_simple[i] );
			gtk_clist_set_text( GTK_CLIST(clFGR), i, 0, txt );
			gtk_clist_set_background( GTK_CLIST(clFGR), i, &color_modif);
		} else {
			gtk_clist_set_background( GTK_CLIST(clFGR), i, &color_ident);
		}
	}
	gtk_clist_thaw( GTK_CLIST(clFGR) );

	gtk_clist_freeze( GTK_CLIST(clFGR2) );
	for(i=0; i<32; i++) {
		if( gui_fantom_double[i]!=  *reg_cop1_double[i] )
		{
			gui_fantom_double[i] = *reg_cop1_double[i];
			sprintf(txt, "%f", *reg_cop1_double[i] );
			gtk_clist_set_text( GTK_CLIST(clFGR2), i, 0, txt );
			gtk_clist_set_background( GTK_CLIST(clFGR2), i, &color_modif);
		} else {
			gtk_clist_set_background( GTK_CLIST(clFGR2), i, &color_ident);
		}
	}
	gtk_clist_thaw( GTK_CLIST(clFGR2) );
}
