/**
 * Mupen64 - main.c
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

/* This is the command line version of the MUPEN64's entry point,
 * if you want to implement an interface, you should look here
 */

// Emulateur Nintendo 64, MUPEN64, Fichier Principal 
// main.c
#define VERSION "0.5\0"

#include <stdlib.h>
#include <unistd.h>

#include "main.h"
#include "guifuncs.h"
#include "rom.h"
#include "../r4300/r4300.h"
#include "../r4300/recomph.h"
#include "../memory/memory.h"
#include "winlnxdefs.h"
#include "plugin.h"
#include "savestates.h"

#define NOGUI_VERSION
#include "gui_gtk/config.h"
#include "../config.h"

#include <SDL.h>

#if defined (__linux__)
#include <signal.h>
#endif

int autoinc_slot = 0;
int *autoinc_save_slot = &autoinc_slot;

static char cwd[1024];
int p_noask;

char g_WorkingDir[PATH_MAX];

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

int ask_bad()
{
   char c;
   printf("The rom you are trying to load is probably a bad dump\n");
   printf("Be warned that this will probably give unexpected results.\n");
   printf("Do you still want to run it (y/n) ?");
   
   if(p_noask) return 1;
   else
     {
	c = getchar();
	getchar();
	if (c=='y' || c=='Y') return 1;
	else return 0;
     }
}

int ask_hack()
{
   char c;
   printf("The rom you are trying to load is not a good verified dump\n");
   printf("It can be a hacked dump, trained dump or anything else that \n");
   printf("may work but be warned that it can give unexpected results.\n");
   printf("Do you still want to run it (y/n) ?");
   
   if(p_noask) return 1;
   else
     {
	c = getchar();
	getchar();
	if (c=='y' || c=='Y') return 1;
	else return 0;
     }
}

void warn_savestate_from_another_rom()
{
   printf("Error: You're trying to load a save state from either another rom\n");
   printf("       or another dump.\n");
}

void warn_savestate_not_exist()
{
   printf("Error: The save state you're trying to load doesn't exist\n");
}

void new_frame()
{
}

void new_vi()
{
}

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
	     switch (event->key.keysym.unicode)
	       {
		case '0':
		  savestates_select_slot(0);
		  break;
		case '1':
		  savestates_select_slot(1);
		  break;
		case '2':
		  savestates_select_slot(2);
		  break;
		case '3':
		  savestates_select_slot(3);
		  break;
		case '4':
		  savestates_select_slot(4);
		  break;
		case '5':
		  savestates_select_slot(5);
		  break;
		case '6':
		  savestates_select_slot(6);
		  break;
		case '7':
		  savestates_select_slot(7);
		  break;
		case '8':
		  savestates_select_slot(8);
		  break;
		case '9':
		  savestates_select_slot(9);
		  break;
		default:
		  keyDown(0, event->key.keysym.sym);
	       }
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

#if defined (__linux__)
static void sigterm_handler()
{
   stop_it();
}
#endif

int main (int argc, char *argv[])
{
   char c;
   char plugins[100][100], s[20];
   char romfile[PATH_MAX];
   int old_i, i, i1, i2, i3, i4;
   int p, p_fullscreen = 0, p_emumode = 0, p_gfx = 0, p_audio = 0, p_input = 0, p_rsp = 0, p_help = 0, p_error = 0;
   int p_emumode_value=1, fileloaded = 0, p_interactive = 0;
   int true = 1;
   char *buffer, *buffer2;
   
#if defined (__linux__)
   if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
     printf("Warning: Couldn't register SIGTERM signal handler!\n");
#endif
   
   //Set working dir
#ifdef WITH_HOME
     {
	char temp[PATH_MAX], orig[PATH_MAX];
	FILE *src, *dest;
	struct dirent *entry;
	DIR *dir;
	
	strcpy(g_WorkingDir, getenv("HOME"));
	strcat(g_WorkingDir, "/.mupen64/");
	strcpy(cwd, g_WorkingDir);
	mkdir(g_WorkingDir, 0700);
	
	strcpy(temp, g_WorkingDir);
	strcat(temp, "mupen64.ini");
	dest = fopen(temp, "rb");
	if (dest == NULL)
	  {
	     unsigned char byte;
	     dest = fopen(temp, "wb");
	     strcpy(orig, WITH_HOME);
	     strcat(orig, "share/mupen64/mupen64.ini");
	     src = fopen(orig, "rb");
	     while(fread(&byte, 1, 1, src))
	       fwrite(&byte, 1, 1, dest);
	     fclose(src);
	     fclose(dest);
	  }
	else fclose(dest);
	
	strcpy(temp, g_WorkingDir);
	strcat(temp, "lang");
	strcpy(orig, WITH_HOME);
	strcat(orig, "share/mupen64/lang");
	symlink(orig, temp);
	
	/*strcpy(temp, g_WorkingDir);
	strcat(temp, "plugins");
	strcpy(orig, WITH_HOME);
	strcat(orig, "share/mupen64/plugins");
	symlink(orig, temp);*/
	
	strcpy(temp, g_WorkingDir);
	strcat(temp, "plugins");
	mkdir(temp, 0700);
	strcpy(orig, WITH_HOME);
	strcat(orig, "share/mupen64/plugins");
	dir = opendir(orig);
	while((entry = readdir(dir)) != NULL)
	  {
	     if(strcmp(entry->d_name + strlen(entry->d_name) - 3, ".so"))
	       {
		  strcpy(orig, WITH_HOME);
		  strcat(orig, "share/mupen64/plugins/");
		  strcat(orig, entry->d_name);
		  src = fopen(orig, "rb");
		  if(src == NULL) continue;
		  
		  strcpy(temp, g_WorkingDir);
		  strcat(temp, "plugins/");
		  strcat(temp, entry->d_name);
		  dest = fopen(temp, "rb");
		  if(dest == NULL)
		    {
		       unsigned char byte;
		       dest = fopen(temp, "wb");
		       while(fread(&byte, 1, 1, src))
			 fwrite(&byte, 1, 1, dest);
		       fclose(src);
		       fclose(dest);
		    }
		  else fclose(dest);
	       }
	     else
	       {
		  strcpy(temp, g_WorkingDir);
		  strcat(temp, "plugins/");
		  strcat(temp, entry->d_name);
		  strcpy(orig, WITH_HOME);
		  strcat(orig, "share/mupen64/plugins/");
		  strcat(orig, entry->d_name);
		  symlink(orig, temp);
	       }
	  }
	
	strcpy(temp, g_WorkingDir);
	strcat(temp, "save/");
	mkdir(temp, 0700);
	
	chdir(g_WorkingDir);
     }
#else
   if (argv[0][0] != '/')
     {
	getcwd(cwd, 1024);
	strcat(cwd, "/");
	strcat(cwd, argv[0]);
     }
   else
     strcpy(cwd, argv[0]);
   while(cwd[strlen(cwd)-1] != '/') cwd[strlen(cwd)-1] = '\0';
   strcpy(g_WorkingDir, cwd);
#endif
   
   //read config file, read plugins
   config_read();
   plugin_scan_directory(cwd);
   
   //get config file settings
   
   buffer = (char*)config_get_string("Gfx Plugin", "");
   buffer2= plugin_name_by_filename(buffer);
   if(buffer2)
     {
	strcpy(plugins[100], buffer2);
	p_gfx = true;
     }
   else if(buffer) printf("GFX Plugin from ini-file could not be loaded\n");
   
   buffer = (char*)config_get_string("Audio Plugin", "");
   buffer2= plugin_name_by_filename(buffer);
   if(buffer2)
     {
	strcpy(plugins[99], buffer2);
	p_audio = true;
     }
   else if(buffer) printf("Audio Plugin from ini-file could not be loaded\n");
   
   buffer = (char*)config_get_string("Input Plugin", "");
   buffer2= plugin_name_by_filename(buffer);
   if(buffer2)
     {
	strcpy(plugins[98], buffer2);
	p_input = true;
     }
   else if(buffer) printf("Input Plugin from ini-file could not be loaded\n");
   
   buffer = (char*)config_get_string("RSP Plugin", "");
   buffer2= plugin_name_by_filename(buffer);
   if(buffer2)
     {
	strcpy(plugins[97], buffer2);
	p_rsp = true;
     }
   else if(buffer) printf("RSP Plugin from ini-file could not be loaded\n");
   
   buffer = (char*)config_get_string("Core", "");
   if(strcmp(buffer,""))
     {
	p_emumode = true;
	p_emumode_value = buffer[0]-'0'+1;
     }
   
   buffer = (char*)config_get_string("Fullscreen", "");
   if(strcmp(buffer,""))
     {
	if(!strcmp(buffer, "true"))
	  p_fullscreen = true;
     }
   
   buffer = (char*)config_get_string("No Ask", "");
   if(strcmp(buffer,""))
     {
	if(!strcmp(buffer, "true"))
	  p_noask = true;
     }
   
   // Command Line Parameter - Parsing
   
   for(p=1; p<argc; p++)
     {
	if(argv[p][0] == '-')
	  {
	     if(!strcmp(argv[p], "--fullscreen"))
	       p_fullscreen = true;
	     else if(!strcmp(argv[p], "--help"))
	       p_help = true;
	     else if(!strcmp(argv[p], "--noask"))
	       p_noask = true;
	     else if(!strcmp(argv[p], "--interactive"))
	       p_interactive = true;
	     else if(!strcmp(argv[p], "--emumode"))
	       {
		  p++;
		  if(p < argc)
		    {
		       p_emumode_value = argv[p][0];
		       p_emumode = true;
		    }
	       }
	     else if(!strcmp(argv[p], "--gfx"))
	       {
		  p++;
		  if(p < argc)
		    {
		       buffer = plugin_name_by_filename(argv[p]);
		       if(buffer)
			 {
			    strcpy(plugins[100], buffer);
			    p_gfx = true;
			 }
		       else printf("specified GFX Plugin couldn't be loaded!\n");
		    }
	       }
	     else if(!strcmp(argv[p], "--audio"))
	       {
		  p++;
		  if(p < argc)
		    {
		       buffer = plugin_name_by_filename(argv[p]);
		       if(buffer)
			 {
			    strcpy(plugins[99], buffer);
			    p_audio = true;
			 }
		       else printf("specified Audio Plugin couldn't be loaded!\n");
		    }
	       }
	     else if(!strcmp(argv[p], "--input"))
	       {
		  p++;
		  if(p < argc)
		    {
		       buffer = plugin_name_by_filename(argv[p]);
		       if(buffer)
			 {
			    strcpy(plugins[98], buffer);
			    p_input = true;
			 }
		       else printf("specified Input Plugin couldn't be loaded!\n");
		    }
	       }
	     else if(!strcmp(argv[p], "--rsp"))
	       {
		  p++;
		  if(p < argc)
		    {
		       buffer = plugin_name_by_filename(argv[p]);
		       if(buffer)
			 {
			    strcpy(plugins[97], buffer);
			    p_rsp = true;
			 }
		       else printf("specified RSP Plugin couldn't be loaded!\n");
		    }
	       }
	  }
	else
	  {
	     strcpy(romfile, argv[p]);
	     fileloaded = true;
	  }
     }
   
   if(p_interactive)
     {
	p_emumode = 0;
	p_gfx = 0;
	p_audio = 0;
	p_input = 0;
	p_rsp = 0;
     }
   
   printf("\nMupen64 version : %s\n", VERSION);
   
   if (argc < 2 || p_help || p_error || fileloaded != true)
     {
	printf("\n\n"
	       "syntax: mupen64_nogui [parameter(s)] rom\n"
	       "\n"
	       "Parameters:\n"
	       "  --fullscreen       : turn fullscreen mode on\n"
	       "  --gfx (plugin)     : set gfx plugin to (plugin)\n"
	       "  --audio (plugin)   : set audio plugin to (plugin)\n"
	       "  --input (plugin)   : set input plugin to (plugin)\n"
	       "  --rsp (plugin)     : set rsp plugin to (plugin)\n"
	       "  --emumode (number) : set emu mode to: 1=interp./2=recomp./3=pure interp\n"
	       "  --noask            : don't ask to force load on bad dumps\n"
	       "  --interactive      : ask interactively for all plugins\n"
	       "\n"
	       "You can also use the Config-File from the Gui-Version\n"
	       "but there are aditional Parameters for the NO-GUI Version\n"
	       "\n");
	return 0;
     }
   
   if (rom_read(romfile))
     {
	if(rom) free(rom);
	if(ROM_HEADER) free(ROM_HEADER);
	return 1;
     }
   printf("Goodname:%s\n", ROM_SETTINGS.goodname);
   printf("16kb eeprom=%d\n", ROM_SETTINGS.eeprom_16kb);
   printf ("emulation mode:\n"
	   "     1. interpreter\n"
	   "     2. dynamic recompiler (default)\n"
           "     3. pure interpreter\n");
   
   if(p_emumode)
     c = p_emumode_value;
   else
     c = getchar();
   
   if (c == '1') dynacore=0;
   else if (c == '3') dynacore=2;
   else dynacore=1;
   
   SDL_Init(SDL_INIT_VIDEO);
   SDL_SetVideoMode(10, 10, 16, 0);
   SDL_ShowCursor(0);
   SDL_EnableKeyRepeat(0, 0);
   SDL_EnableUNICODE(1);
   init_memory();
   // --------------------- loading plugins ----------------------
   i=1;
   i1=1;
   printf("  Choose your gfx plugin : \n");
   while(plugin_type() != -1)
     {
	if (plugin_type() == PLUGIN_TYPE_GFX)
	  {
	     strcpy(plugins[i], plugin_next());
	     printf("%s (%s)\n", plugins[i], plugin_filename_by_name(plugins[i]));
	     i++;
	  }
	else
	  plugin_next();
     }
   
   if(p_gfx)
     i1 = 100;
   else
     {
	if(p_emumode)
	  getchar();
	/*c = getchar();
	 s[0] = c;
	 s[1] = 0;*/
	scanf("%10s", s);
	i1 = atoi(s);
     }
   
   plugin_rewind();
   old_i = i;
   printf("  Choose your audio plugin : \n");
   while(plugin_type() != -1)
     {
	if (plugin_type() == PLUGIN_TYPE_AUDIO)
	  {
	     strcpy(plugins[i], plugin_next());
	     printf("%s (%s)\n", plugins[i], plugin_filename_by_name(plugins[i]));
	     i++;
	  }
	else
	  plugin_next();
     }
   /*getchar();
   c = getchar();
   //getchar();
   s[0] = c;
   s[1] = 0;*/
   if(p_audio)
     i2 = 99;
   else
     {
	scanf("%10s", s);
	i2 = old_i + atoi(s) - 1;
     }
   
   plugin_rewind();
   old_i = i;
   printf("  Choose your input plugin : \n");
   while(plugin_type() != -1)
     {
	if (plugin_type() == PLUGIN_TYPE_CONTROLLER)
	  {
	     strcpy(plugins[i], plugin_next());
	     printf("%s (%s)\n", plugins[i], plugin_filename_by_name(plugins[i]));
	     i++;
	  }
	else
	  plugin_next();
     }
   /*getchar();
   c = getchar();
   //getchar();
   s[0] = c;
   s[1] = 0;*/
   if(p_input)
     i3 = 98;
   else
     {
	scanf("%10s", s);
	i3 = old_i + atoi(s) - 1;
     }
   
   plugin_rewind();
   old_i = i;
   printf("  Choose your RSP plugin : \n");
   while(plugin_type() != -1)
     {
	if (plugin_type() == PLUGIN_TYPE_RSP)
	  {
	     strcpy(plugins[i], plugin_next());
	     printf("%s (%s)\n", plugins[i], plugin_filename_by_name(plugins[i]));
	     i++;
	  }
	else
	  plugin_next();
     }
   /*getchar();
   c = getchar();
   getchar();
   s[0] = c;
   s[1] = 0;*/
   if(p_rsp)
     i4 = 97;
   else
     {
	scanf("%10s", s);
	i4 = old_i + atoi(s) - 1;
     }
   
   printf("\n\nSelected Plugins: %s, %s, %s, %s\n", plugins[i1], plugins[i2], plugins[3], plugins[i4]);
   
   plugin_load_plugins(plugins[i1], plugins[i2], plugins[i3], plugins[i4]);
   romOpen_gfx();
   romOpen_audio();
   romOpen_input();
   // ------------------------------------------------------------
   SDL_SetEventFilter(filter);
   
   if(p_fullscreen)
     changeWindow();
   
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
   free(ROM_HEADER);
   free_memory();
   return 0;
}
