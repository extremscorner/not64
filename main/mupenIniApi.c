/**
 * Mupen64 - mupenIniApi.c
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

#include "mupenIniApi.h"
#include "guifuncs.h"

#include <stdio.h>
#include <zlib.h>
#include <string.h>
#include <stdlib.h>

typedef struct _iniElem
{
   mupenEntry entry;
   struct _iniElem* next_entry;
   struct _iniElem* next_crc;
   struct _iniElem* next_MD5;
} iniElem;

typedef struct
{
   char *comment;
   iniElem* CRC_lists[256];
   iniElem* MD5_lists[256];
   iniElem* list;
} iniFile;

static iniFile ini = {NULL};

mupenEntry * emptyEntry ;

static int split_property(char *s)
{
   int i = 0;
   while(s[i] != '=' && s[i] != 0) i++;
   if (s[i] == 0) return -1;
   s[i] = 0;
   return i;
}

static char* get_ini_path()
{
   static char *path = NULL;
   if (path == NULL)
     {
	path = malloc(strlen(get_currentpath())+1+strlen("mupen64.ini"));
	strcpy(path, get_currentpath());
	strcat(path, "mupen64.ini");
     }
   return path;
}

void ini_openFile()
{
   gzFile f;
   char buf[256];
   int i=0;
   iniElem *cur = NULL;
   
   if (ini.comment != NULL) return;
   
   memset( &emptyEntry,0,sizeof(emptyEntry));
   
   f = gzopen(get_ini_path(), "rb");
   if (f==NULL) return;
   
   do
     {
	gzgets(f, buf, 255);
	if (buf[0] != '[')
	  {
	     i+= strlen(buf);
	     if (ini.comment == NULL) 
	       {
		  ini.comment = (char*)malloc(i+1);
		  strcpy(ini.comment, buf);
	       }
	     else 
	       {
		  ini.comment = (char*)realloc(ini.comment, i+1);
		  strcat(ini.comment, buf);
	       }
	  }
     }
   while (buf[0] != '[' && !gzeof(f));
   
   for (i=0; i<255; i++)
     {
    	ini.CRC_lists[i] = NULL;
     }
   ini.list = NULL;
   
   do
     {
	if (buf[0] == '[')
	  {
	     if (ini.list == NULL)
	       {
		  ini.list = (iniElem*)malloc(sizeof(iniElem));
		  ini.list->next_entry = NULL;
		  ini.list->next_crc = NULL;
		  ini.list->next_MD5 = NULL;
		  cur = ini.list;
	       }
	     else
	       {
		  cur->next_entry = (iniElem*)malloc(sizeof(iniElem));
		  cur = cur->next_entry;
		  cur->next_crc = NULL;
		  cur->next_MD5 = NULL;
	       }
	     i = strlen(buf);
	     while(buf[i] != ']') i--;
	     buf[i] = 0;
	     strncpy(cur->entry.MD5, buf+1, 32);
	     cur->entry.MD5[32] = '\0';
	     buf[3] = 0;
	     sscanf(buf+1, "%X", &i);
	     
	     if (ini.MD5_lists[i] == NULL)
	       ini.MD5_lists[i] = cur;
	     else
	       {
		  iniElem *aux = ini.MD5_lists[i];
		  cur->next_MD5 = aux;
		  ini.MD5_lists[i] = cur;
	       }
	     cur->entry.eeprom16kb = 0;
	     strcpy(cur->entry.refMD5, "");
	     strcpy(cur->entry.comments, "");
	  }
	else
	  {
	     i = split_property(buf);
	     if (i != -1)
	       {
		  if (!strcmp(buf, "Good Name"))
		    {
		       if (buf[i+1+strlen(buf+i+1)-1] == '\n')
			 buf[i+1+strlen(buf+i+1)-1] = '\0';
		       if (buf[i+1+strlen(buf+i+1)-1] == '\r')
			 buf[i+1+strlen(buf+i+1)-1] = '\0';
		       strncpy(cur->entry.goodname, buf+i+1, 99);
		    }
		  else if (!strcmp(buf, "Header Code"))
		    {
		       strncpy(cur->entry.CRC, buf+i+1, 21);
		       cur->entry.CRC[21] = '\0';
		       buf[i+3] = 0;
		       sscanf(buf+i+1, "%X", &i);
		       
		       if (ini.CRC_lists[i] == NULL)
			 ini.CRC_lists[i] = cur;
		       else
			 {
			    iniElem *aux = ini.CRC_lists[i];
			    cur->next_crc = aux;
			    ini.CRC_lists[i] = cur;
			 }
		    }
		  else if (!strcmp(buf, "Reference"))
		    {
		       strncpy(cur->entry.refMD5, buf+i+1, 32);
		       cur->entry.refMD5[32] = '\0';
		    }
		  else if (!strcmp(buf, "Eeprom"))
		    {
		       if (!strncmp(buf+i+1, "16k", 3))
			 cur->entry.eeprom16kb = 1;
		    }
		  else if (!strcmp(buf, "Comments"))
		    {
		       if (buf[i+1+strlen(buf+i+1)-1] == '\n')
			 buf[i+1+strlen(buf+i+1)-1] = '\0';
		       if (buf[i+1+strlen(buf+i+1)-1] == '\r')
			 buf[i+1+strlen(buf+i+1)-1] = '\0';
		       strcpy(cur->entry.comments, buf+i+1);
		    }
	       }
	  }
	gzgets(f, buf, 255);
     }
   while (!gzeof(f));
   
   gzclose(f);
}

void ini_closeFile()
{
   if (ini.comment == NULL) return ;
   
   free(ini.comment);
   ini.comment = NULL;
   while(ini.list != NULL)
	 {
	    iniElem *aux = ini.list->next_entry;
	    free(ini.list);
	    ini.list = aux;
	 }
}

void ini_updateFile(int compress)
{
   gzFile zf = NULL;
   FILE *f = NULL;
   iniElem *aux;
   
   if (ini.comment == NULL) return ;
   
   if (compress) 
     {
	zf = gzopen(get_ini_path(), "wb");
	gzprintf(zf, "%s", ini.comment);
     }
   else
     {
	f = fopen(get_ini_path(), "wb");
	fprintf(f, "%s", ini.comment);
     }
   
   aux = ini.list;
   while (aux != NULL)
     {
	if (compress) 
	  {
	     gzprintf(zf, "[%s]\n", aux->entry.MD5);
	     gzprintf(zf, "Good Name=%s\n", aux->entry.goodname);
	     gzprintf(zf, "Header Code=%s\n", aux->entry.CRC);
	     if (strcmp(aux->entry.refMD5, ""))
	       gzprintf(zf, "Reference=%s\n", aux->entry.refMD5);
	     if (aux->entry.eeprom16kb == 1)
	       gzprintf(zf, "Eeprom=16k\n");
	     if (strcmp(aux->entry.comments, ""))
	       gzprintf(zf, "Comments=%s\n", aux->entry.comments);
	     gzprintf(zf, "\n");
	  }
	else
	  {
	     fprintf(f, "[%s]\n", aux->entry.MD5);
	     fprintf(f, "Good Name=%s\n", aux->entry.goodname);
	     fprintf(f, "Header Code=%s\n", aux->entry.CRC);
	     if (strcmp(aux->entry.refMD5, ""))
	       fprintf(f, "Reference=%s\n", aux->entry.refMD5);
	     if (aux->entry.eeprom16kb == 1)
	       fprintf(f, "Eeprom=16k\n");
	     if (strcmp(aux->entry.comments, ""))
	       fprintf(f, "Comments=%s\n", aux->entry.comments);
	     fprintf(f, "\n");
	  }
	
	aux = aux->next_entry;
     }
   
   if (compress) gzclose(zf);
   else fclose(f);
}

mupenEntry* ini_search_by_md5(const char *md5)
{
   char t[3];
   int i;
   iniElem *aux;
   
   if (ini.comment == NULL) return emptyEntry;
   
   t[0] = md5[0];
   t[1] = md5[1];
   t[2] = 0;
   sscanf(t, "%X", &i);
   aux = ini.MD5_lists[i];
   while (aux != NULL && strncmp(aux->entry.MD5, md5, 32))
     aux = aux->next_MD5;
   if (aux == NULL) return NULL;
   return &(aux->entry);
}

mupenEntry* ini_search_by_CRC(const char *crc)
{
   char t[3];
   int i;
   iniElem *aux;
   
   if (ini.comment == NULL) return emptyEntry;
   
   t[0] = crc[0];
   t[1] = crc[1];
   t[2] = 0;
   sscanf(t, "%X", &i);
   aux = ini.CRC_lists[i];
   while (aux != NULL && strncmp(aux->entry.CRC, crc, 21))
     aux = aux->next_crc;
   if (aux == NULL) return NULL;
   if (strcmp(aux->entry.refMD5, ""))
     {
	mupenEntry* temp = ini_search_by_md5(aux->entry.refMD5);
	if (strncmp(aux->entry.CRC, temp->CRC, 21))
	  return &(aux->entry);
	else
	  return temp;
     }
   else
     return &(aux->entry);
}
