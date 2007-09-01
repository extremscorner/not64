/*
 * debugger/registers.c
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

#include "registers.h"
#include <pthread.h>


// State of the Emulation Thread:
//    0 -> pause, 2 -> run.
extern int run;

static void on_close();



//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void init_registers()
{
	GtkWidget *boxH1, *notebook;
	GtkWidget *label1, *label2, *label3, *lab_RI1, *lab_RI2,
			*lab_AI1, *lab_AI2, *lab_PI1, *lab_PI2,  
			*lab_SI1, *lab_SI2, *lab_VI1, *lab_VI2;
	
	char **txt;
	txt=malloc( 2*sizeof(char*) );
	txt[0]=malloc( 64*sizeof(char) );
	txt[1]=malloc( 64*sizeof(char) );


	registers_opened = 1;

	//=== Creation of the Registers Window =============/
	winRegisters = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	sprintf( txt[0], "%s - %s", "N64 Registers", DEBUGGER_VERSION );
	gtk_window_set_title( GTK_WINDOW(winRegisters), txt[0] );
	gtk_container_set_border_width( GTK_CONTAINER(winRegisters), 10);

	boxH1 = gtk_hbox_new( FALSE, 0);
	gtk_container_add( GTK_CONTAINER(winRegisters), boxH1 );

	//=== Initialisation of GPR display ================/
	init_GPR();
	gtk_box_pack_start( GTK_BOX(boxH1), frGPR, FALSE, FALSE, 0);

	//=== Initialisation of Cop0 display ===============/
	init_regCop0();
	gtk_box_pack_start( GTK_BOX(boxH1), frCop0, FALSE, FALSE, 10);


	//=== Initialisation of the Notebook ================/
	notebook = gtk_notebook_new();
	gtk_box_pack_end( GTK_BOX(boxH1), notebook, FALSE, FALSE, 0 );
	gtk_notebook_set_tab_pos( GTK_NOTEBOOK(notebook), GTK_POS_RIGHT );
	gtk_notebook_set_homogeneous_tabs( GTK_NOTEBOOK(notebook), TRUE );
	gtk_notebook_set_scrollable( GTK_NOTEBOOK(notebook), TRUE );
	gtk_notebook_popup_enable( GTK_NOTEBOOK(notebook) );


	//=== Initialisation of R4300 Special reg. display ==/
	init_regSpecial();
	label1=gtk_label_new("Special");
	label2=gtk_label_new("Various Registers");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegSpecial, label1, label2);
	
	//=== Initialisation of FGR (Cop1) display ==========/
	init_FGR();
	label3=gtk_label_new("Cop1");
	gtk_notebook_append_page( GTK_NOTEBOOK(notebook), frFGR, label3);

	//=== Initialisation of AI registers ================/
	init_regAI();
	lab_AI1 = gtk_label_new("AI");
	lab_AI2 = gtk_label_new("Audio Interface");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegAI, lab_AI1, lab_AI2);

	//=== Initialisation of PI registers ================/
	init_regPI();
	lab_PI1 = gtk_label_new("PI");
	lab_PI2 = gtk_label_new("Peripheral Interface");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegPI, lab_PI1, lab_PI2);

	//=== Initialisation of RI registers ================/
	init_regRI();
	lab_RI1 = gtk_label_new("RI");
	lab_RI2 = gtk_label_new("RDRAM Interface");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegRI, lab_RI1, lab_RI2);
		
	//=== Initialisation of SI registers ================/
	init_regSI();
	lab_SI1 = gtk_label_new("SI");
	lab_SI2 = gtk_label_new("Serial Interface");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegSI, lab_SI1, lab_SI2);
	
	//=== Initialisation of VI registers ================/
	init_regVI();
	lab_VI1 = gtk_label_new("VI");
	lab_VI2 = gtk_label_new("Video Interface");
	gtk_notebook_append_page_menu( GTK_NOTEBOOK(notebook), frRegVI, lab_VI1, lab_VI2);
	
	//=== Signals Connection ============================/
	gtk_signal_connect( GTK_OBJECT(winRegisters), "destroy", on_close, NULL);
	
	gtk_widget_show_all(winRegisters);
}




//]=-=-=-=-=-=-=-=-=-=-=[ Mise-a-jour Affichage Registre ]=-=-=-=-=-=-=-=-=-=-=[

void update_registers()
{
	update_GPR();		// GPR (R4300 core)
	update_regCop0();	// cop0 (status registers)
	update_FGR();		// cop1 (floating points registers)
	update_regSpecial();	// R4300 core special registers.
	update_regAI();		// Audio interface.
	update_regPI();		// Peripheral interface.
	update_regRI();		// RDRAM interface.
	update_regSI();		// Serial interface.
	update_regVI();		// Serial interface.
}


//]=-=-=-=-=-=-=[ Les Fonctions de Retour des Signaux (CallBack) ]=-=-=-=-=-=-=[

static void on_close()
{
	registers_opened = 0;
}
