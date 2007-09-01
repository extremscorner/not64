/**
 * Mupen64 - breakpoints.c
 * Copyright (C) 2002 DavFr - robind@esiee.fr
 *
 * If you want to contribute to this part of the project please
 * contact me (or Hacktarux) first.
 * 
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
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


// TODO: check_breakpoints() being a critic bottleneck when run=1,
// it should be optimised to maximum.
// First solution: to sort bp_addresses[] to shorten while loop.
// Second solution: to use a hash function, but efficiency have
// to be verified (eg in "2 breakpoints defined" case).

#include "breakpoints.h"

#define BREAKPOINTS_MAX_NUMBER	128

static uint32 bp_addresses[BREAKPOINTS_MAX_NUMBER];
static int selected[BREAKPOINTS_MAX_NUMBER];
static int nb_breakpoints;

static void on_row_selection(GtkCList *clist, gint row);
static void on_row_unselection(GtkCList *clist, gint row);
static void on_add();
static void on_remove();
static void on_close();

static GtkWidget *clBreakpoints;


//]=-=-=-=-=-=-=-=-=-=-=-=[ Breakpoints Initialisation ]=-=-=-=-=-=-=-=-=-=-=-=[

void init_breakpoints()
{
	int i;
	GtkWidget	*boxH1,
			*scrolledwindow1,
			*boxV1,
			*buAdd, *buRemove;

	breakpoints_opened = 1;
	for(i=0; i<BREAKPOINTS_MAX_NUMBER; i++)	{
		bp_addresses[i]=0;
	}
	for(i=0; i<BREAKPOINTS_MAX_NUMBER; i++)	{
		selected[i]=0;
	}

	//=== Creation of Breakpoints Management ===========/
	winBreakpoints = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW(winBreakpoints), "Breakpoints");
	gtk_window_set_default_size( GTK_WINDOW(winBreakpoints), 100, 150);
	gtk_container_set_border_width( GTK_CONTAINER(winBreakpoints), 2);

	boxH1 = gtk_hbox_new( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER(winBreakpoints), boxH1 );

 	//=== Creation of Breakpoints Display ==============/
	scrolledwindow1 = gtk_scrolled_window_new( NULL, NULL );
	gtk_box_pack_start( GTK_BOX(boxH1), scrolledwindow1, FALSE, FALSE, 0);
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolledwindow1),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC );
	gtk_range_set_update_policy( GTK_RANGE (GTK_SCROLLED_WINDOW(scrolledwindow1)->hscrollbar),
					GTK_POLICY_AUTOMATIC );

	clBreakpoints = gtk_clist_new( 1 );
	gtk_container_add( GTK_CONTAINER(scrolledwindow1), clBreakpoints );
	gtk_clist_set_selection_mode( GTK_CLIST(clBreakpoints), GTK_SELECTION_EXTENDED );
	gtk_clist_set_column_width( GTK_CLIST(clBreakpoints), 0, 80 );
	gtk_clist_set_auto_sort( GTK_CLIST(clBreakpoints), TRUE );
	
 	//=== Creation of the Buttons ======================/
	boxV1 = gtk_vbox_new( FALSE, 2 );
	gtk_box_pack_end( GTK_BOX(boxH1), boxV1, FALSE, FALSE, 0 );
	
	buAdd = gtk_button_new_with_label("Add");
	gtk_box_pack_start( GTK_BOX(boxV1), buAdd, FALSE, FALSE, 0 );
	buRemove = gtk_button_new_with_label( "Remove" );
	gtk_box_pack_start( GTK_BOX(boxV1), buRemove, FALSE, FALSE, 0 );

	gtk_widget_show_all(winBreakpoints);

	//=== Signal Connections ===========================/
	gtk_signal_connect( GTK_OBJECT(clBreakpoints), "select-row", on_row_selection, NULL );
	gtk_signal_connect( GTK_OBJECT(clBreakpoints), "unselect-row", on_row_unselection, NULL );
	gtk_signal_connect( GTK_OBJECT(buAdd), "clicked", on_add, NULL );
	gtk_signal_connect( GTK_OBJECT(buRemove), "clicked", on_remove, NULL );
	gtk_signal_connect( GTK_OBJECT(winBreakpoints), "destroy", on_close, NULL );
}




//]=-=-=-=-=-=-=-=-=-=-=-=[ Update Breakpoints Display ]=-=-=-=-=-=-=-=-=-=-=-=[

int add_breakpoint( uint32 address )
{
	if( nb_breakpoints == BREAKPOINTS_MAX_NUMBER ) {
		printf("BREAKPOINTS_MAX_NUMBER have been reached.\n");
		return -1;
	}
	else {
		int new_row;	//index of the appended row.
		char **line;
		line = malloc(1*sizeof(char*));    // new breakpoint:
		line[0] = malloc(16*sizeof(char)); // - address
// TODO:	line[1] = malloc(16*sizeof(char)); // - enabled/disabled
	
		sprintf( line[0], "0x%lX", address);
		new_row = gtk_clist_append( GTK_CLIST(clBreakpoints), line );
		gtk_clist_set_row_data( GTK_CLIST(clBreakpoints), new_row, (gpointer) address );

		bp_addresses[nb_breakpoints]=address;
		nb_breakpoints++;

		return new_row;
	}
}


void remove_breakpoint_by_row( int row )
{
	int i=0;
	uint32 address;
	
	address = (uint32) gtk_clist_get_row_data( GTK_CLIST(clBreakpoints), row);
	while(bp_addresses[i]!=address)
	{
		i++;
	}
	nb_breakpoints--;
	bp_addresses[i]=bp_addresses[nb_breakpoints];

	gtk_clist_remove( GTK_CLIST(clBreakpoints), row);
}


int remove_breakpoint_by_address( uint32 address )
{
	int row;

	row = gtk_clist_find_row_from_data( GTK_CLIST(clBreakpoints),(gpointer) address);
	if( row != -1 ) {
		remove_breakpoint_by_row( row );
	}
	return row;
}


int check_breakpoints( uint32 address )
{
	int i=0;
	
	while( i != nb_breakpoints )
	{
		if( bp_addresses[i]!=address )
			i++;
		else
			return gtk_clist_find_row_from_data( GTK_CLIST(clBreakpoints),(gpointer) address);
	}
	return -1;
}



//]=-=-=-=-=-=-=[ Les Fonctions de Retour des Signaux (CallBack) ]=-=-=-=-=-=-=[

static void on_row_selection(GtkCList *clist, gint row)
{
	selected[row]=1;
}


static void on_row_unselection(GtkCList *clist, gint row)
{
	selected[row]=0;
}


static gint modify_address(ClistEditData *ced, const gchar *old, const gchar *new, gpointer data)
{
	uint32 address;

	if( sscanf(new, "%lX", &address) != 1)
	{
		return FALSE;
	}
	printf( "%lX\n", address);
	gtk_clist_set_row_data( GTK_CLIST(ced->clist), ced->row, (gpointer) address );
	bp_addresses[nb_breakpoints-1]=address;
	return TRUE;
}

static void on_add()
{
	int new_row;
	
	new_row = add_breakpoint( 0x0 );
	clist_edit_by_row(GTK_CLIST(clBreakpoints), new_row, 0, modify_address, NULL);
	//FIXME:color are not updated +everything
}


static void on_remove()
{
	int i;
	uint32 address;

	gtk_clist_freeze( GTK_CLIST(clBreakpoints) );
	for( i=BREAKPOINTS_MAX_NUMBER-1; i>=0; i-- )
	{
		if( selected[i] == 1 ) {
			address = (uint32) gtk_clist_get_row_data( GTK_CLIST(clBreakpoints), i);
			remove_breakpoint_by_row( i );
			update_desasm_color( address );
			selected[i] = 0;
		}
	}
	gtk_clist_unselect_all( GTK_CLIST(clBreakpoints) );
	gtk_clist_thaw( GTK_CLIST(clBreakpoints) );
}


static void on_close()
{
	breakpoints_opened = 0;
}
