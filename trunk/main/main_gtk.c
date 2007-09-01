/**
 * Mupen64 - main_gtk.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
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

/* The gtk interface but it is far from being complete...*/

#define VERSION "0.5"

#include <stdlib.h>
#include <unistd.h>

#include "guifuncs.h"
#include "rom.h"
#include "winlnxdefs.h"
#include "plugin.h"
#include "../r4300/r4300.h"
#include "../r4300/recomph.h"
#include "../memory/memory.h"
#include "savestates.h"

#include <pthread.h>
#include <SDL.h>
#include <gtk/gtk.h>

#ifdef DBG
#include <glib.h>
#include <stdio.h>
#include "../debugger/debugger.h"
#endif

#include "../logo.xpm"

int autoinc_slot = 0;
int *autoinc_save_slot = &autoinc_slot;

static char cwd[1024];

char *get_currentpath()
{
   return cwd;
}

char *get_savespath()
{
   static char path[1024];
   strcpy(path, get_currentpath());
   strcat(path, "save/");
   return path;
}

void display_loading_progress(int p)
{
   printf("loading rom : %d%%\r", p);
   fflush(stdout);
   if (p==100) printf("\n");
}

void display_MD5calculating_progress(int p)
{
}

void warn_savestate_from_another_rom()
{
}

void warn_savestate_not_exist()
{
}

/*static int question_answer;

static gint delete_question_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   return TRUE;
}

static void yes_func(GtkWidget *widget, gpointer data)
{
   question_answer = 1;
   gtk_widget_destroy(data);
}

static void no_func(GtkWidget *widget, gpointer data)
{
   question_answer = 0;
   gtk_widget_destroy(data);
}*/

int ask_bad()
{
   /*GtkWidget *dialog, *yes_button, *no_button, *label;
   
   dialog = gtk_dialog_new();
   yes_button = gtk_button_new_with_label("Yes");
   no_button = gtk_button_new_with_label("No");
   label = 
     gtk_label_new("The rom you are trying to load is probably a bad dump\n"
		   "Be warned that this will probably give unexpected results,\n"
		   "Do you still want to run it ?");
   
   gtk_signal_connect(GTK_OBJECT(yes_button), "clicked",
		      GTK_SIGNAL_FUNC(yes_func), GTK_OBJECT(dialog));
   gtk_signal_connect(GTK_OBJECT(no_button), "clicked",
		      GTK_SIGNAL_FUNC(no_func), GTK_OBJECT(dialog));
   gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
		      GTK_SIGNAL_FUNC(delete_question_event), 
		      GTK_OBJECT(dialog));
   
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), yes_button);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), no_button);
   
   gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
   question_answer = 2;
   gtk_widget_show_all(dialog);
   
   while(question_answer == 2);
   return question_answer;*/
   return 1;
}

int ask_hack()
{
   return 1;
}

void new_frame()
{
}

void new_vi()
{
}

static int file_selection_launched = 0;
pthread_t thread_n64;

static GtkWidget *window;
static GtkWidget *interp;
static GtkWidget *dynam;
static GtkWidget *pure_interp;
static GtkWidget *combo_gfx;
static GtkWidget *combo_audio;
static GtkWidget *combo_input;
static GtkWidget *combo_RSP;

#ifdef DBG
static GtkWidget *button_debug;
#endif

static int filter(const SDL_Event *event)
{
   switch (event->type)
     {
      case SDL_KEYDOWN:
	switch (event->key.keysym.sym)
	  {
	   case SDLK_F5:
	     savestates_job |= SAVESTATE;
	     break;
	   case SDLK_F7:
	     savestates_job |= LOADSTATE;
	     break;
	   case SDLK_ESCAPE:
	     stop_it();
	     break;
	   case SDLK_F1:
	     changeWindow();
	     break;
	   default:
	     keyDown(0, event->key.keysym.sym);
	  }
	return 0;
	break;
      case SDL_KEYUP:
	switch (event->key.keysym.sym)
	  {
	   case SDLK_ESCAPE:
	     break;
	   case SDLK_F1:
	     break;
	   default:
	     keyUp(0, event->key.keysym.sym);
	  }
	return 0;
	break;
      default:
	return 1;
     }
}

static void *main_thread(void *p)
{
   if (GTK_TOGGLE_BUTTON(dynam)->active == 1) dynacore = 1;
   else if (GTK_TOGGLE_BUTTON(pure_interp)->active == 1) dynacore = 2;
   else dynacore = 0;
   
   SDL_Init(SDL_INIT_VIDEO);
   SDL_SetVideoMode(10, 10, 16, 0);
   SDL_SetEventFilter(filter);
   SDL_ShowCursor(0);
   SDL_EnableKeyRepeat(0, 0);
   
   init_memory();
   plugin_load_plugins(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_gfx)->entry)),
		       gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_audio)->entry)),
		       gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_input)->entry)),
		       gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_RSP)->entry)));
   romOpen_gfx();
   romOpen_audio();
   romOpen_input();
   go();
   romClosed_RSP();
   romClosed_input();
   romClosed_audio();
   romClosed_gfx();
   closeDLL_RSP();
   closeDLL_input();
   closeDLL_audio();
   closeDLL_gfx();
   free(rom);
   rom = NULL;
   free(ROM_HEADER);
   ROM_HEADER = NULL;
   free_memory();
   file_selection_launched = 0;
   return 0;
}

static void launch_rom(GtkWidget *widget, gpointer data)
{
   char *name, *name_aux;
   
   name_aux = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
   name = malloc(strlen(name_aux)+1);
   strcpy(name, name_aux);
   
   gtk_widget_destroy(GTK_WIDGET(data));
   
   printf("Emulateur Nintendo64 (mupen64) version : %s\n", VERSION);
   if (rom_read(name))
     {
	if(rom) {
	   free(rom);
	   rom = NULL;
	}
	if(ROM_HEADER) {
	   free(ROM_HEADER);
	   ROM_HEADER = NULL;
	}
	free(name);
	file_selection_launched = 0;
	return;
     }
   
   // checking 'debugger' option
#ifdef DBG
   if (GTK_TOGGLE_BUTTON(button_debug)->active == 1)
     {
	init_debugger();
     }
#endif
   
   // creating emulation thread
   if (pthread_create(&thread_n64, NULL, main_thread, NULL))
     printf("error: problem with thread_n64 creation.\n");
   else
     printf("creation of the emulation thread... PID=%ld\n", thread_n64);
}

static void cancel_load(GtkWidget *widget, gpointer data)
{
   file_selection_launched = 0;
   gtk_widget_destroy(GTK_WIDGET(data));
}

static void destroy_load(GtkWidget *widget, gpointer data)
{
}

static void load(GtkWidget *widget, gpointer data)
{
   if (!file_selection_launched)
     {
	GtkWidget *load_dialog;
	load_dialog = gtk_file_selection_new("Choose a N64 rom file : ");
	
	gtk_signal_connect(GTK_OBJECT(load_dialog), "destroy",
			   GTK_SIGNAL_FUNC(destroy_load), NULL);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_dialog)->ok_button), 
			   "clicked", GTK_SIGNAL_FUNC(launch_rom), load_dialog);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(load_dialog)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(cancel_load), load_dialog);
   
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(load_dialog),
					".");
	gtk_widget_show(load_dialog);
	file_selection_launched = 1;
     }
}

static gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   return(FALSE);
}

static void destroy(GtkWidget *widget, gpointer data)
{
   pthread_kill(thread_n64, 9); // sigKill
   gtk_main_quit();
}

static void config_gfx(GtkWidget *widget, gpointer data)
{
   plugin_exec_config(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_gfx)->entry)));
}

static void test_gfx(GtkWidget *widget, gpointer data)
{
   plugin_exec_test(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_gfx)->entry)));
}

static void about_gfx(GtkWidget *widget, gpointer data)
{
   plugin_exec_about(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_gfx)->entry)));
}

static void config_audio(GtkWidget *widget, gpointer data)
{
   plugin_exec_config(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_audio)->entry)));
}

static void test_audio(GtkWidget *widget, gpointer data)
{
   plugin_exec_test(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_audio)->entry)));
}

static void about_audio(GtkWidget *widget, gpointer data)
{
   plugin_exec_about(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_audio)->entry)));
}

static void config_input(GtkWidget *widget, gpointer data)
{
   plugin_exec_config(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_input)->entry)));
}

static void test_input(GtkWidget *widget, gpointer data)
{
   plugin_exec_test(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_input)->entry)));
}

static void about_input(GtkWidget *widget, gpointer data)
{
   plugin_exec_about(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_input)->entry)));
}

static void config_RSP(GtkWidget *widget, gpointer data)
{
   plugin_exec_config(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_RSP)->entry)));
}

static void test_RSP(GtkWidget *widget, gpointer data)
{
   plugin_exec_test(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_RSP)->entry)));
}

static void about_RSP(GtkWidget *widget, gpointer data)
{
   plugin_exec_about(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo_RSP)->entry)));
}

int main (int argc, char *argv[])
{
   GtkWidget *load_button;
   GtkWidget *table;
   GtkWidget *pixmapwid;
   GtkStyle *style;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkWidget *label_core;
   
   GList *glist_gfx = NULL;
   GtkWidget *label_gfx;
   GtkWidget *config_gfx_button;
   GtkWidget *test_gfx_button;
   GtkWidget *about_gfx_button;
   
   GList *glist_audio = NULL;
   GtkWidget *label_audio;
   GtkWidget *config_audio_button;
   GtkWidget *test_audio_button;
   GtkWidget *about_audio_button;
   
   GList *glist_input = NULL;
   GtkWidget *label_input;
   GtkWidget *config_input_button;
   GtkWidget *test_input_button;
   GtkWidget *about_input_button;
   
   GList *glist_RSP = NULL;
   GtkWidget *label_RSP;
   GtkWidget *config_RSP_button;
   GtkWidget *test_RSP_button;
   GtkWidget *about_RSP_button;
   
#ifdef DBG
   if (!g_thread_supported())
     g_thread_init(NULL);
   else
     {
	fprintf(stderr, "mupen64 will certainly have problems with GTK threads.\n");
	fprintf(stderr, "Check your GLIB/GDK/GTK installation for thread support.\n");
     }
#endif
   
   gtk_init(&argc, &argv);
   
   if (argv[0][0] != '/')
     {
	getcwd(cwd, 1024);
	strcat(cwd, "/");
	strcat(cwd, argv[0]);
     }
   else
     strcpy(cwd, argv[0]);
   while(cwd[strlen(cwd)-1] != '/') cwd[strlen(cwd)-1] = '\0';
   
   // création de la fenêtre
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_container_set_border_width(GTK_CONTAINER(window), 10);
   gtk_widget_show(window);
   
   // création du bouton Load rom
   load_button = gtk_button_new_with_label("Load rom...");   
   gtk_widget_show(load_button);
   
   // création du logo
   style = gtk_widget_get_style(window);
   pixmap = gdk_pixmap_create_from_xpm_d(window->window, &mask,
					 &style->bg[GTK_STATE_NORMAL],
					 (gchar **)logo_xpm);
   pixmapwid = gtk_pixmap_new(pixmap, mask);
   gtk_widget_show(pixmapwid);
   
   // création de la selection du mode d'émulation
   label_core = gtk_frame_new("cpu core : ");
   gtk_widget_show(label_core);
   interp = gtk_radio_button_new_with_label(NULL, "interpreter");
   gtk_widget_show(interp);
   dynam = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(interp)),
					   "dynamic compiler");
   gtk_widget_show(dynam);
   pure_interp = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(interp)), 
						 "pure interpreter");
   gtk_widget_show(pure_interp);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dynam), TRUE);
   
   // création de l'option 'debugger'
#ifdef DBG
   button_debug = gtk_check_button_new_with_label("debugger mode");
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_debug), FALSE);
   gtk_widget_show(button_debug);
#endif
   
   // recherche des plugins
   plugin_scan_directory(cwd);
   while(plugin_type() != -1)
     switch (plugin_type())
       {
	case PLUGIN_TYPE_GFX:
	  glist_gfx = g_list_append(glist_gfx, plugin_next());
	  break;
	case PLUGIN_TYPE_AUDIO:
	  glist_audio = g_list_append(glist_audio, plugin_next());
	  break;
	case PLUGIN_TYPE_CONTROLLER:
	  glist_input = g_list_append(glist_input, plugin_next());
	  break;
	case PLUGIN_TYPE_RSP:
	  glist_RSP = g_list_append(glist_RSP, plugin_next());
	  break;
       }
   
   label_gfx = gtk_frame_new("gfx plugin : ");
   gtk_widget_show(label_gfx);
   combo_gfx = gtk_combo_new();
   gtk_combo_set_popdown_strings(GTK_COMBO(combo_gfx), glist_gfx);
   gtk_combo_set_value_in_list(GTK_COMBO(combo_gfx), TRUE, FALSE);
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo_gfx)->entry), FALSE);
   gtk_widget_show(combo_gfx);
   config_gfx_button = gtk_button_new_with_label("Config");
   gtk_widget_show(config_gfx_button);
   test_gfx_button = gtk_button_new_with_label("Test");
   gtk_widget_show(test_gfx_button);
   about_gfx_button = gtk_button_new_with_label("About");
   gtk_widget_show(about_gfx_button);
   
   label_audio = gtk_frame_new("audio plugin : ");
   gtk_widget_show(label_audio);
   combo_audio = gtk_combo_new();
   gtk_combo_set_popdown_strings(GTK_COMBO(combo_audio), glist_audio);
   gtk_combo_set_value_in_list(GTK_COMBO(combo_audio), TRUE, FALSE);
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo_audio)->entry), FALSE);
   gtk_widget_show(combo_audio);
   config_audio_button = gtk_button_new_with_label("Config");
   gtk_widget_show(config_audio_button);
   test_audio_button = gtk_button_new_with_label("Test");
   gtk_widget_show(test_audio_button);
   about_audio_button = gtk_button_new_with_label("About");
   gtk_widget_show(about_audio_button);
   
   label_input = gtk_frame_new("input plugin : ");
   gtk_widget_show(label_input);
   combo_input = gtk_combo_new();
   gtk_combo_set_popdown_strings(GTK_COMBO(combo_input), glist_input);
   gtk_combo_set_value_in_list(GTK_COMBO(combo_input), TRUE, FALSE);
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo_input)->entry), FALSE);
   gtk_widget_show(combo_input);
   config_input_button = gtk_button_new_with_label("Config");
   gtk_widget_show(config_input_button);
   test_input_button = gtk_button_new_with_label("Test");
   gtk_widget_show(test_input_button);
   about_input_button = gtk_button_new_with_label("About");
   gtk_widget_show(about_input_button);
   
   label_RSP = gtk_frame_new("RSP plugin : ");
   gtk_widget_show(label_RSP);
   combo_RSP = gtk_combo_new();
   gtk_combo_set_popdown_strings(GTK_COMBO(combo_RSP), glist_RSP);
   gtk_combo_set_value_in_list(GTK_COMBO(combo_RSP), TRUE, FALSE);
   gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo_RSP)->entry), FALSE);
   gtk_widget_show(combo_RSP);
   config_RSP_button = gtk_button_new_with_label("Config");
   gtk_widget_show(config_RSP_button);
   test_RSP_button = gtk_button_new_with_label("Test");
   gtk_widget_show(test_RSP_button);
   about_RSP_button = gtk_button_new_with_label("About");
   gtk_widget_show(about_RSP_button);
   
   // mise en place des évenements
   gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event), NULL);
   gtk_signal_connect(GTK_OBJECT(window), "destroy",
		      GTK_SIGNAL_FUNC(destroy), NULL);
   gtk_signal_connect(GTK_OBJECT(load_button), "clicked",
		      GTK_SIGNAL_FUNC(load), NULL);
   gtk_signal_connect(GTK_OBJECT(config_gfx_button), "clicked",
		      GTK_SIGNAL_FUNC(config_gfx), NULL);
   gtk_signal_connect(GTK_OBJECT(test_gfx_button), "clicked",
		      GTK_SIGNAL_FUNC(test_gfx), NULL);
   gtk_signal_connect(GTK_OBJECT(about_gfx_button), "clicked",
		      GTK_SIGNAL_FUNC(about_gfx), NULL);
   gtk_signal_connect(GTK_OBJECT(config_audio_button), "clicked",
		      GTK_SIGNAL_FUNC(config_audio), NULL);
   gtk_signal_connect(GTK_OBJECT(test_audio_button), "clicked",
		      GTK_SIGNAL_FUNC(test_audio), NULL);
   gtk_signal_connect(GTK_OBJECT(about_audio_button), "clicked",
		      GTK_SIGNAL_FUNC(about_audio), NULL);
   gtk_signal_connect(GTK_OBJECT(config_input_button), "clicked",
		      GTK_SIGNAL_FUNC(config_input), NULL);
   gtk_signal_connect(GTK_OBJECT(test_input_button), "clicked",
		      GTK_SIGNAL_FUNC(test_input), NULL);
   gtk_signal_connect(GTK_OBJECT(about_input_button), "clicked",
		      GTK_SIGNAL_FUNC(about_input), NULL);
   gtk_signal_connect(GTK_OBJECT(config_RSP_button), "clicked",
		      GTK_SIGNAL_FUNC(config_RSP), NULL);
   gtk_signal_connect(GTK_OBJECT(test_RSP_button), "clicked",
		      GTK_SIGNAL_FUNC(test_RSP), NULL);
   gtk_signal_connect(GTK_OBJECT(about_RSP_button), "clicked",
		      GTK_SIGNAL_FUNC(about_RSP), NULL);
   
   // mise en page
   table = gtk_table_new(55, 30, TRUE);
   gtk_widget_show(table);
   
   gtk_container_add(GTK_CONTAINER(window), table);
   gtk_table_attach_defaults(GTK_TABLE(table), load_button, 0, 10, 0, 3);
   gtk_table_attach_defaults(GTK_TABLE(table), label_core, 0, 10, 5, 15);
   gtk_table_attach_defaults(GTK_TABLE(table), pixmapwid, 10, 30, 0, 10);
   gtk_table_attach_defaults(GTK_TABLE(table), interp, 1, 10, 6, 9);
   gtk_table_attach_defaults(GTK_TABLE(table), dynam, 1, 10, 9, 12);
   gtk_table_attach_defaults(GTK_TABLE(table), pure_interp, 1, 10, 12, 15);
   
#ifdef DBG
   gtk_table_attach_defaults(GTK_TABLE(table), button_debug, 15, 25, 12, 15);
#endif
   
   gtk_table_attach_defaults(GTK_TABLE(table), label_gfx, 0, 30, 15, 25);
   gtk_table_attach_defaults(GTK_TABLE(table), combo_gfx, 1, 29, 17, 22);
   gtk_table_attach_defaults(GTK_TABLE(table), config_gfx_button, 1, 10, 22, 24);
   gtk_table_attach_defaults(GTK_TABLE(table), test_gfx_button, 10, 20, 22, 24);
   gtk_table_attach_defaults(GTK_TABLE(table), about_gfx_button, 20, 29, 22, 24);
   
   gtk_table_attach_defaults(GTK_TABLE(table), label_audio, 0, 30, 25, 35);
   gtk_table_attach_defaults(GTK_TABLE(table), combo_audio, 1, 29, 27, 32);
   gtk_table_attach_defaults(GTK_TABLE(table), config_audio_button, 1, 10, 32, 34);
   gtk_table_attach_defaults(GTK_TABLE(table), test_audio_button, 10, 20, 32, 34);
   gtk_table_attach_defaults(GTK_TABLE(table), about_audio_button, 20, 29, 32, 34);
   
   gtk_table_attach_defaults(GTK_TABLE(table), label_input, 0, 30, 35, 45);
   gtk_table_attach_defaults(GTK_TABLE(table), combo_input, 1, 29, 37, 42);
   gtk_table_attach_defaults(GTK_TABLE(table), config_input_button, 1, 10, 42, 44);
   gtk_table_attach_defaults(GTK_TABLE(table), test_input_button, 10, 20, 42, 44);
   gtk_table_attach_defaults(GTK_TABLE(table), about_input_button, 20, 29, 42, 44);
   
   gtk_table_attach_defaults(GTK_TABLE(table), label_RSP, 0, 30, 45, 55);
   gtk_table_attach_defaults(GTK_TABLE(table), combo_RSP, 1, 29, 47, 52);
   gtk_table_attach_defaults(GTK_TABLE(table), config_RSP_button, 1, 10, 52, 54);
   gtk_table_attach_defaults(GTK_TABLE(table), test_RSP_button, 10, 20, 52, 54);
   gtk_table_attach_defaults(GTK_TABLE(table), about_RSP_button, 20, 29, 52, 54);
   
   gtk_main();
   
   return 0;
}
