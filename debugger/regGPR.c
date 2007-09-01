/*
 * debugger/regGPR.c
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

#include "regGPR.h"

// On garde une copie des valeurs affichees a l'ecran, pour accelerer la maj de
// l'affichage.
// Les "registres fantomes" evitent de raffraichir l'affichage de chaque
// registre. Seules les modifications sont affichees a l'ecran.
static sint64	gui_fantom_gpr[32];

/*static char *mnemonicGPR[]=
{
	"R0",	"AT",	"V0",	"V1",
	"A0",	"A1",	"A2",	"A3",
	"T0",	"T1",	"T2",	"T3",
	"T4",	"T5",	"T6",	"T7",
	"S0",	"S1",	"S2",	"S3",
	"S4",	"S5",	"S6",	"S7",
	"T8",	"T9",	"K0",	"K1",
	"GP",	"SP",	"S8",	"RA",
};*/


static GtkWidget *clGPR;


//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void init_GPR()
{
	
	GtkWidget *boxH1,
				*boxV1,
					**labGPR;
				
	int i;
	char **txt;
	txt=malloc( 2*sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );
	txt[1]=malloc( 64*sizeof(char) );


	GPR_opened = 1;

	//CREATION TABLEAU REGISTRES R4300
	frGPR = gtk_frame_new("GPR");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5 );
	gtk_container_add( GTK_CONTAINER(frGPR), boxH1 );

	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	labGPR = malloc( 32*sizeof(GtkWidget*) );

	//Petit pb d'alignement entr l'affichage des "regs" et les valeurs hexa.
	sprintf( txt[0], "reg%d", 0);
	labGPR[0] = gtk_label_new( txt[0] );
	gtk_label_set_justify( GTK_LABEL(labGPR[0]), GTK_JUSTIFY_RIGHT );
	gtk_box_pack_start( GTK_BOX(boxV1), labGPR[0], FALSE, FALSE, 1 );
	// c'est ca la difference avec le reste de la boucle --------^
	for( i=1; i<32; i++)
	{
		sprintf( txt[0], "reg%d", i);
		labGPR[i]=gtk_label_new( txt[0] );
		gtk_label_set_justify( GTK_LABEL(labGPR[i]), GTK_JUSTIFY_RIGHT);
		gtk_box_pack_start( GTK_BOX(boxV1), labGPR[i], FALSE, FALSE, 0);
	}

	// nb: gtk1.2.10 seems to have a bug with gtk_clist_new()
	// => "gtk_clist_set_column_width" is required.
	clGPR = gtk_clist_new(1);
	gtk_box_pack_start( GTK_BOX(boxH1), clGPR, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clGPR), GTK_SELECTION_SINGLE);
	gtk_clist_set_column_width( GTK_CLIST(clGPR), 0, 130);
	strcpy( txt[0], "Undefined" );
	for( i=0; i<32; i++)
	{
		gtk_clist_append( GTK_CLIST(clGPR), txt);
	}
	gtk_clist_set_column_width( GTK_CLIST(clGPR), 0, 130);

//	gtk_signal_connect( GTK_OBJECT(clGPR), "button_press_event",
//				GTK_SIGNAL_FUNC(on_click), clGPR);
	//Initialisation des registres fantomes.
	for( i=0; i<32; i++)
	{
		gui_fantom_gpr[i] = 0x1234567890LL; //la valeur la moins probable possible
	}
}




//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void update_GPR()
{
	int i;
	char txt[24];

	gtk_clist_freeze( GTK_CLIST(clGPR) );
	
	for(i=0; i<32; i++)
	{
		// Les "registres fantomes" evitent de raffraichir l'affichage de chaque
		// registre. Seules les modifications sont affichees a l'ecran.
		if(gui_fantom_gpr[i]!=reg[i])
		{
			gui_fantom_gpr[i] = reg[i];
			sprintf(txt, "%.16llX", reg[i]);
			gtk_clist_set_text( GTK_CLIST(clGPR), i, 0, txt );
			gtk_clist_set_background( GTK_CLIST(clGPR), i, &color_modif);
		}
		else
		{
			gtk_clist_set_background( GTK_CLIST(clGPR), i, &color_ident);
		}
	}
	gtk_clist_thaw( GTK_CLIST(clGPR) );
}
