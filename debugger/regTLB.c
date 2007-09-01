/*
 * debugger/regTLB.c
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

#include "regTLB.h"
#include <pthread.h>

static GtkWidget *clTLBentries;

static tlb	gui_fantom_tlb_entry[32];

static void on_close();

static char *mnemonicTLB[]=
{
	"Mx",		"VPN2",		"G",		"asid",
	"PFNeven",	"Ceven",	"Deven",	"Veven",
	"PFNodd",	"Codd",		"Dodd",		"Vodd",
	"R",
	"START_even",	"END_even",	"PHYS_even",
	"START_odd",	"END_odd",	"PHYS_odd",
};


//]=-=-=-=-=-=-=-=-=-=-=[ Initialisation of TLB Display ]=-=-=-=-=-=-=-=-=-=-=[

void init_TLB()
{
	GtkWidget *boxH1,
			*boxV1,
				*labTLB[32];
	
	int i;
	char **txt;
	txt=malloc( 19*sizeof(char*) );
	for(i=0; i<19; i++)
		txt[i]=malloc( 64*sizeof(char) );

	regTLB_opened = 1;

	frTLB = gtk_frame_new("Translation Lookaside Buffer");

	boxH1 = gtk_hbox_new( FALSE, 2);
	gtk_container_add( GTK_CONTAINER(frTLB), boxH1 );
	gtk_container_set_border_width( GTK_CONTAINER(boxH1), 5 );
	
	//=== Creation of Labels "entryXX" Column ============/
	boxV1 = gtk_vbox_new( FALSE, 0);
	gtk_box_pack_start( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0);

	for( i=31; i>=0; i--)
	{
		sprintf( txt[0], "%d", i);
		labTLB[i]=gtk_label_new( txt[0] );
		gtk_label_set_justify( GTK_LABEL(labTLB[i]), GTK_JUSTIFY_RIGHT);
		gtk_box_pack_end( GTK_BOX(boxV1), labTLB[i], FALSE, TRUE, 0);
	}

	//=== Creation of Registers Value Display ==========/
	clTLBentries = gtk_clist_new_with_titles( 19, mnemonicTLB );
	gtk_box_pack_start( GTK_BOX(boxH1), clTLBentries, TRUE, TRUE, 0);
	gtk_clist_set_selection_mode( GTK_CLIST(clTLBentries), GTK_SELECTION_SINGLE);
	for(i=0; i<19; i++)
		strcpy( txt[i], "X");
	for( i=0; i<32; i++)
		gtk_clist_append( GTK_CLIST(clTLBentries), txt);

	//=== Fantom Registers Initialisation ============/
	for( i=0; i<32; i++)
	{
		gui_fantom_tlb_entry[i].mask=0xABCD;		//short (12bits)
		gui_fantom_tlb_entry[i].vpn2=0xABCDEF01;	//long (19bits)
		gui_fantom_tlb_entry[i].g=0xAB;			//char (1bit)
		gui_fantom_tlb_entry[i].asid=0xAB;		//unsigned char (8bits)
		gui_fantom_tlb_entry[i].pfn_even=0xABCDEF01;	//long (24bits)
		gui_fantom_tlb_entry[i].c_even=0xAB;		//char (3bits)
		gui_fantom_tlb_entry[i].d_even=0xAB;		//char (1bit)
		gui_fantom_tlb_entry[i].v_even=0xAB;		//char (1bit)
		gui_fantom_tlb_entry[i].pfn_odd=0xABCDEF01;	//long (24bits)
		gui_fantom_tlb_entry[i].c_odd=0xAB;		//char (3bits)
		gui_fantom_tlb_entry[i].d_odd=0xAB;		//char (1bit)
		gui_fantom_tlb_entry[i].v_odd=0xAB;		//char (1bit)
		gui_fantom_tlb_entry[i].r=0xAB;			//char (2bits)
		//gui_fantom_tlb_entry[i].check_parity_mask=0xABCD; NOT USED?
		
		gui_fantom_tlb_entry[i].start_even = 0x12345678;//ulong
		gui_fantom_tlb_entry[i].end_even = 0x12345678;	//ulong
		gui_fantom_tlb_entry[i].phys_even = 0x12345678;	//ulong
		gui_fantom_tlb_entry[i].start_odd = 0x12345678;	//ulong
		gui_fantom_tlb_entry[i].end_odd = 0x12345678;	//ulong
		gui_fantom_tlb_entry[i].phys_odd = 0x12345678;	//ulong
	}
}


void init_TLBwindow()
{
	GtkWidget *boxH;
	
	char **txt;
	txt=malloc( 2*sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );
	txt[1]=malloc( 64*sizeof(char) );

	
	winTLB = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title( GTK_WINDOW(winTLB), "Translation Lookaside Buffer (TLB)" );
	gtk_container_set_border_width( GTK_CONTAINER(winTLB), 10);

	boxH = gtk_hbox_new( FALSE, 0);
	gtk_container_add( GTK_CONTAINER(winTLB), boxH );

	init_TLB();
	gtk_box_pack_start( GTK_BOX(boxH), frTLB, FALSE, FALSE, 0);

	gtk_widget_show_all(winTLB);

	gtk_signal_connect( GTK_OBJECT(winTLB), "destroy", on_close, NULL);
}




//]=-=-=-=-=-=-=-=-=-=-[ Update function for TLB Display ]-=-=-=-=-=-=-=-=-=-=[

void update_TLB()
{
	int i;
	char **txt;
	txt=malloc( 19*sizeof(char*) );
	for(i=0; i<19; i++)
		txt[i]=malloc( 64*sizeof(char) );


	gtk_clist_freeze( GTK_CLIST(clTLBentries) );

	for( i=0; i<32; i++)
	{
		if( (gui_fantom_tlb_entry[i].mask != tlb_e[i].mask)	||
		 (gui_fantom_tlb_entry[i].vpn2 != tlb_e[i].vpn2)	||
		 (gui_fantom_tlb_entry[i].g != tlb_e[i].g)		||
		 (gui_fantom_tlb_entry[i].asid != tlb_e[i].asid)	||
		 (gui_fantom_tlb_entry[i].pfn_even != tlb_e[i].pfn_even) ||
		 (gui_fantom_tlb_entry[i].c_even != tlb_e[i].c_even) ||
		 (gui_fantom_tlb_entry[i].d_even != tlb_e[i].d_even) ||
		 (gui_fantom_tlb_entry[i].v_even != tlb_e[i].v_even) ||
		 (gui_fantom_tlb_entry[i].pfn_odd != tlb_e[i].pfn_odd)	||
		 (gui_fantom_tlb_entry[i].c_odd != tlb_e[i].c_odd)	||
		 (gui_fantom_tlb_entry[i].d_odd != tlb_e[i].d_odd)	||
		 (gui_fantom_tlb_entry[i].v_odd != tlb_e[i].v_odd)	||
		 (gui_fantom_tlb_entry[i].r != tlb_e[i].r)		||
		 (gui_fantom_tlb_entry[i].start_even != tlb_e[i].start_even) ||
		 (gui_fantom_tlb_entry[i].end_even != tlb_e[i].end_even)     ||
		 (gui_fantom_tlb_entry[i].phys_even != tlb_e[i].phys_even)   ||
		 (gui_fantom_tlb_entry[i].start_odd != tlb_e[i].start_odd)   ||
		 (gui_fantom_tlb_entry[i].end_odd != tlb_e[i].end_odd)	     ||
		 (gui_fantom_tlb_entry[i].phys_odd != tlb_e[i].phys_odd) )
		{
			gtk_clist_remove( GTK_CLIST(clTLBentries), i);
			
			gui_fantom_tlb_entry[i].mask	= tlb_e[i].mask;
			sprintf( txt[0], "%hX", tlb_e[i].mask);
			gui_fantom_tlb_entry[i].vpn2	= tlb_e[i].vpn2;
			sprintf( txt[1], "%lX", tlb_e[i].vpn2);
			gui_fantom_tlb_entry[i].g	= tlb_e[i].g;
			sprintf( txt[2], "%hhX", tlb_e[i].g);
			gui_fantom_tlb_entry[i].asid	= tlb_e[i].asid;
			sprintf( txt[3], "%hhX", tlb_e[i].asid);
			gui_fantom_tlb_entry[i].pfn_even= tlb_e[i].pfn_even;
			sprintf( txt[4], "%lX", tlb_e[i].pfn_even);
			gui_fantom_tlb_entry[i].c_even	= tlb_e[i].c_even;
			sprintf( txt[5], "%hhX", tlb_e[i].c_even);
			gui_fantom_tlb_entry[i].d_even	= tlb_e[i].d_even;
			sprintf( txt[6], "%hhX", tlb_e[i].d_even);
			gui_fantom_tlb_entry[i].v_even	= tlb_e[i].v_even;
			sprintf( txt[7], "%hhX", tlb_e[i].v_even);
			gui_fantom_tlb_entry[i].pfn_odd	= tlb_e[i].pfn_odd;
			sprintf( txt[8], "%lX", tlb_e[i].pfn_odd);
			gui_fantom_tlb_entry[i].c_odd	= tlb_e[i].c_odd;
			sprintf( txt[9], "%hhX", tlb_e[i].c_odd);
			gui_fantom_tlb_entry[i].d_odd	= tlb_e[i].d_odd;
			sprintf( txt[10], "%hhX", tlb_e[i].d_odd);
			gui_fantom_tlb_entry[i].v_odd	= tlb_e[i].v_odd;
			sprintf( txt[11], "%hhX", tlb_e[i].v_odd);
			gui_fantom_tlb_entry[i].r	= tlb_e[i].r;
			sprintf( txt[12], "%hhX", tlb_e[i].r);
	//gui_fantom_tlb_entry[i].check_parity_mask = tlb_e[i].check_parity_mask; NOT USED?
		
			gui_fantom_tlb_entry[i].start_even = tlb_e[i].start_even;
			sprintf( txt[13], "%lX", tlb_e[i].start_even);
			gui_fantom_tlb_entry[i].end_even   = tlb_e[i].end_even;
			sprintf( txt[14], "%lX", tlb_e[i].end_even);
			gui_fantom_tlb_entry[i].phys_even  = tlb_e[i].phys_even;
			sprintf( txt[15], "%lX", tlb_e[i].phys_even);
			gui_fantom_tlb_entry[i].start_odd  = tlb_e[i].start_odd;
			sprintf( txt[16], "%lX", tlb_e[i].start_odd);
			gui_fantom_tlb_entry[i].end_odd    = tlb_e[i].end_odd;
			sprintf( txt[17], "%lX", tlb_e[i].end_odd);
			gui_fantom_tlb_entry[i].phys_odd   = tlb_e[i].phys_odd;
			sprintf( txt[18], "%lX", tlb_e[i].phys_odd);
	
			gtk_clist_insert( GTK_CLIST(clTLBentries), i, txt);
			gtk_clist_set_background( GTK_CLIST(clTLBentries), i, &color_modif);
		}
		else
			gtk_clist_set_background( GTK_CLIST(clTLBentries), i, &color_ident);
	}

	gtk_clist_thaw( GTK_CLIST(clTLBentries) );
}

void update_TLBwindow()
{
	update_TLB();
}



//]=-=-=-=-=-=-=[ Les Fonctions de Retour des Signaux (CallBack) ]=-=-=-=-=-=-=[

static void on_close()
{
	regTLB_opened = 0;
}
