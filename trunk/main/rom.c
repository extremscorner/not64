/**
 * Mupen64 - rom.c
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

/* This is the functions that load a rom into memory, it loads a roms
 * in multiple formats, gzipped or not. It searches the rom, in the roms
 * subdirectory or in the path specified in the path.cfg file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <ctype.h>

#include "rom.h"
#include "../memory/memory.h"
#include "unzip.h"
#include "guifuncs.h"
#include "md5.h"
#include "mupenIniApi.h"
#include "guifuncs.h"

static FILE *rom_file;
static gzFile *z_rom_file;
static unzFile zip;
static unz_file_info pfile_info;
static int i, tmp, z;

int taille_rom;
unsigned char *rom;
rom_header *ROM_HEADER;
rom_settings ROM_SETTINGS;

static void findsize()
{
   if (!z)
     {
	fseek(rom_file, 0L, SEEK_END);
	taille_rom=ftell(rom_file);
	printf ("rom size: %d bytes (or %d Mb or %d Megabits)\n", 
		taille_rom, taille_rom/1024/1024, taille_rom/1024/1024*8);
	fseek(rom_file, 0L, SEEK_SET);
     }
   else if (z == 1)
     {
	taille_rom=0;
	rom=malloc(100000);
	for(;;)
	  {
	     i = gzread(z_rom_file, rom, 100000);
	     taille_rom += i;
	     printf ("rom size: %d bytes (or %d Mb or %d Megabits)\r",
		     taille_rom, taille_rom/1024/1024, taille_rom/1024/1024*8);
	     fflush(stdout);
	     if (!i) break;
	  }
	free(rom);
	rom = NULL;
	printf("\n");
	gzseek(z_rom_file, 0L, SEEK_SET);
     }
}

static int find_file(char *argv)
{
   z=0;
   i=strlen(argv);
     {
	unsigned char buf[4];
	char szFileName[255], extraField[255], szComment[255];
	zip = unzOpen(argv);
	if (zip != NULL) 
	  {
	     unzGoToFirstFile(zip);
	     do
	       {
		  unzGetCurrentFileInfo(zip, &pfile_info, szFileName, 255,
					extraField, 255, szComment, 255);
		  unzOpenCurrentFile(zip);
		  if (pfile_info.uncompressed_size >= 4)
		    {
		       unzReadCurrentFile(zip, buf, 4);
		       if ((*((unsigned long*)buf) != 0x40123780) &&
			   (*((unsigned long*)buf) != 0x12408037) &&
			   (*((unsigned long*)buf) != 0x80371240))
			 {
			    unzCloseCurrentFile(zip);
			 }
		       else
			 {
			    taille_rom = pfile_info.uncompressed_size;
			    unzCloseCurrentFile(zip);
			    z = 2;
			    return 0;
			 }
		    }
	       }
	     while (unzGoToNextFile(zip) != UNZ_END_OF_LIST_OF_FILE);
	     unzClose(zip);
	     return 1;
	  }
     }
   if((i>3) && (argv[i-3]=='.') && 
      (tolower(argv[i-2])=='g') && (tolower(argv[i-1])=='z'))
     argv[i-3]=0;
   rom_file=NULL;
   z_rom_file=NULL;
   rom_file=fopen(argv, "rb");
   if (rom_file == NULL)
     {
	z_rom_file=gzopen(strcat(argv, ".gz"), "rb");
	if (z_rom_file == NULL)
	  {
	     argv[i-3]=0;
	     z_rom_file=gzopen(strcat(argv, ".GZ"), "rb");
	     if (z_rom_file == NULL) return 1;
	  }
	z = 1;
     }
   return 0;
}

int rom_read(const char *argv)
{
   md5_state_t state;
   md5_byte_t digest[16];
   mupenEntry *entry;
   char buf[1024], arg[1024], *s;
   
   strncpy(arg, argv, 1000);
   if (find_file(arg))
     {
	strncpy(arg, "roms/", 1000);
	if (find_file(strncat(arg, argv, 1000)))
	  {
	     rom_file=fopen("path.cfg", "rb");
	     if(rom_file) fscanf(rom_file, "%1000s", buf);
	     else buf[0]=0;
	     if(rom_file) fclose(rom_file);
	     strncpy(arg, argv, 1000);
	     if (find_file(strcat(buf, arg)))
	       {
		  printf ("file not found or wrong path\n");
		  return 1;
	       }
	  }
     }
   printf ("file found\n");
/*------------------------------------------------------------------------*/   
   findsize();
   
   if (rom) free(rom);
   rom = malloc(taille_rom);

   tmp=0;
   if (!z)
     {
	for (i=0; i<taille_rom;i+=fread(rom+i, 1, 1000, rom_file))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_loading_progress(tmp);
	       }
	  }
     }
   else if (z == 1)
     {
	for (i=0; i<taille_rom;i+=gzread(z_rom_file, rom+i, 1000))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_loading_progress(tmp);
	       }
	  }
     }
   else
     {
	unzOpenCurrentFile(zip);
	for (i=0; i<taille_rom;i+=unzReadCurrentFile(zip, rom+i, 1000))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_loading_progress(tmp);
	       }
	  }
	unzCloseCurrentFile(zip);
     }
   if (!z) fclose(rom_file);
   else if (z==1) gzclose(z_rom_file);
   else unzClose(zip);
   
   if (rom[0]==0x37)
     {
	printf ("byteswaping rom...\n");
	for (i=0; i<(taille_rom/2); i++)
	  {
	     tmp=rom[i*2];
	     rom[i*2]=rom[i*2+1];
	     rom[i*2+1]=tmp;
	  }
	printf ("rom byteswaped\n");
     }
   if (rom[0]==0x40)
     {
	for (i=0; i<(taille_rom/4); i++)
	  {
	     tmp=rom[i*4];
	     rom[i*4]=rom[i*4+3];
	     rom[i*4+3]=tmp;
	     tmp=rom[i*4+1];
	     rom[i*4+1]=rom[i*4+2];
	     rom[i*4+2]=tmp;
	  }
	printf("rom byteswaped\n");
     }
   else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
     {
	printf("wrong file format !\n");
	free(rom);
	rom = NULL;
	return 1;
     }
   printf("rom loaded succesfully\n");
  
   if (!ROM_HEADER) ROM_HEADER = malloc(sizeof(rom_header));
   memcpy(ROM_HEADER, rom, sizeof(rom_header));
   display_loading_progress(100);
   printf ("%x %x %x %x\n", ROM_HEADER->init_PI_BSB_DOM1_LAT_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PGS_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PWD_REG,
	   ROM_HEADER->init_PI_BSB_DOM1_PGS_REG2);
   printf("ClockRate=%x\n", sl((unsigned int)ROM_HEADER->ClockRate));
   printf("Version:%x\n", sl((unsigned int)ROM_HEADER->Release));
   printf("CRC: %x %x\n", sl((unsigned int)ROM_HEADER->CRC1), sl((unsigned int)ROM_HEADER->CRC2));
   printf ("name: %s\n", ROM_HEADER->nom);
   if (sl(ROM_HEADER->Manufacturer_ID) == 'N') printf ("Manufacturer: Nintendo\n");
   else printf ("Manufacturer: %x\n", (unsigned int)(ROM_HEADER->Manufacturer_ID));
   printf("Cartridge_ID: %x\n", ROM_HEADER->Cartridge_ID);
   switch(ROM_HEADER->Country_code)
     {
      case 0x0044:
	printf("Country : Germany\n");
	break;
      case 0x0045:
	printf("Country : United States\n");
	break;
      case 0x004A:
	printf("Country : Japan\n");
	break;
      case 0x0050:
	printf("European cartridge\n");
	break;
      case 0x0055:
	printf("Country : Australie\n");
      default:
	printf("Country Code : %x\n", ROM_HEADER->Country_code);
     }
   printf ("size: %d\n", (unsigned int)(sizeof(rom_header)));
   printf ("PC= %x\n", sl((unsigned int)ROM_HEADER->PC));
   
   // loading rom settings and checking if it's a good dump
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)rom, taille_rom);
   md5_finish(&state, digest);
   printf("md5 code:");
   for (i=0; i<16; i++) printf("%02X", digest[i]);
   printf("\n");
   
   ini_openFile();
   
   for (i=0; i<16; i++) sprintf(arg+i*2, "%02X", digest[i]);
   arg[32] = 0;
   strcpy(ROM_SETTINGS.MD5, arg);
   if ((entry = ini_search_by_md5(arg)) == NULL)
     {
	char mycrc[1024];
	printf("%x\n", (int)entry);
	sprintf(mycrc, "%08X-%08X-C%02X",
		(int)sl(ROM_HEADER->CRC1), (int)sl(ROM_HEADER->CRC2),
		ROM_HEADER->Country_code);
	if ((entry = ini_search_by_CRC(mycrc)) == NULL)
	  {
	     strcpy(ROM_SETTINGS.goodname, ROM_HEADER->nom);
	     strcat(ROM_SETTINGS.goodname, " (unknown rom)");
	     printf("%s\n", ROM_SETTINGS.goodname);
	     ROM_SETTINGS.eeprom_16kb = 0;
	     return 0;
	  }
	else
	  {
	     if (!ask_bad())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	     strcpy(ROM_SETTINGS.goodname, entry->goodname);
	     strcat(ROM_SETTINGS.goodname, " (bad dump)");
	     if (strcmp(entry->refMD5, ""))
	       entry = ini_search_by_md5(entry->refMD5);
	     ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
	     return 0;
	  }
     }
   s=entry->goodname;
   for (i=strlen(s); i > 0 && s[i-1] != '['; i--);
   if (i != 0)
     {
	if (s[i] == 'T' || s[i] == 't' || s[i] == 'h' || s[i] == 'f' || s[i] == 'o')
	  {
	     if (!ask_hack())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	  }
	if (s[i] == 'b')
	  {
	     if (!ask_bad())
	       {
		  free(rom);
		  rom = NULL;
		  free(ROM_HEADER);
		  ROM_HEADER = NULL;
		  return 1;
	       }
	  }
     }
   strcpy(ROM_SETTINGS.goodname, entry->goodname);
   
   if (strcmp(entry->refMD5, ""))
     entry = ini_search_by_md5(entry->refMD5);
   ROM_SETTINGS.eeprom_16kb = entry->eeprom16kb;
   printf("eeprom type:%d\n", ROM_SETTINGS.eeprom_16kb);
   
   return 0;
}

int fill_header(const char *argv)
{
   char arg[1024];
   strncpy(arg, argv, 1000);
   if (find_file(arg))
     {
	printf ("file not found or wrong path\n");
	return 0;
     }
/*------------------------------------------------------------------------*/   
   findsize();
   if (rom) free(rom);
   rom = malloc(0x40);
   
   tmp=0;
   
   if (!z)
     fread(rom, 1, 0x40, rom_file);
   else if (z == 1)
     gzread(z_rom_file, rom, 0x40);
   else
     {
	unzOpenCurrentFile(zip);
	unzReadCurrentFile(zip, rom, 0x40);
	unzCloseCurrentFile(zip);
     }
   if (!z) fclose(rom_file);
   else if (z==1) gzclose(z_rom_file);
   else unzClose(zip);
   
   if (rom[0]==0x37)
     {
	for (i=0; i<(0x40/2); i++)
	  {
	     tmp=rom[i*2];
	     rom[i*2]=rom[i*2+1];
	     rom[i*2+1]=tmp;
	  }
     }
   if (rom[0]==0x40)
     {
	for (i=0; i<(0x40/4); i++)
	  {
	     tmp=rom[i*4];
	     rom[i*4]=rom[i*4+3];
	     rom[i*4+3]=tmp;
	     tmp=rom[i*4+1];
	     rom[i*4+1]=rom[i*4+2];
	     rom[i*4+2]=tmp;
	  }
     }
   else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
     {
	free(rom);
	rom = NULL;
	return 0;
     }
   
   if (ROM_HEADER == NULL) ROM_HEADER= malloc(sizeof(rom_header));
   memcpy(ROM_HEADER, rom, 0x40);
   free(rom);
   rom = NULL;
   return taille_rom;
}

void calculateMD5(const char *argv, unsigned char digest[16])
{
   md5_state_t state;
   char arg[1024];
   
   strncpy(arg, argv, 1000);
   if (find_file(arg))
     {
	printf("file not found or wrong path\n");
	return;
     }
/*------------------------------------------------------------------------*/   
   findsize();
   if (rom) free(rom);
   rom = malloc(taille_rom);
   
   tmp=0;
   if (!z)
     {
	for (i=0; i<taille_rom;i+=fread(rom+i, 1, 1000, rom_file))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_MD5calculating_progress(tmp);
	       }
	  }
     }
   else if (z == 1)
     {
	for (i=0; i<taille_rom;i+=gzread(z_rom_file, rom+i, 1000))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_MD5calculating_progress(tmp);
	       }
	  }
     }
   else
     {
	unzOpenCurrentFile(zip);
	for (i=0; i<taille_rom;i+=unzReadCurrentFile(zip, rom+i, 1000))
	  {
	     if (tmp!=(int)((i/(float)taille_rom)*100))
	       {
		  tmp=(int)(i/(float)(taille_rom)*100);
		  display_MD5calculating_progress(tmp);
	       }
	  }
	unzCloseCurrentFile(zip);
     }
   if (!z) fclose(rom_file);
   else if (z==1) gzclose(z_rom_file);
   else unzClose(zip);
   
   display_MD5calculating_progress(100);
   
   if (rom[0]==0x37)
     {
	printf ("byteswaping rom...\n");
	for (i=0; i<(taille_rom/2); i++)
	  {
	     tmp=rom[i*2];
	     rom[i*2]=rom[i*2+1];
	     rom[i*2+1]=tmp;
	  }
	printf ("rom byteswaped\n");
     }
   if (rom[0]==0x40)
     {
	for (i=0; i<(taille_rom/4); i++)
	  {
	     tmp=rom[i*4];
	     rom[i*4]=rom[i*4+3];
	     rom[i*4+3]=tmp;
	     tmp=rom[i*4+1];
	     rom[i*4+1]=rom[i*4+2];
	     rom[i*4+2]=tmp;
	  }
	printf("rom byteswaped\n");
     }
   else if ((rom[0] != 0x80) || (rom[1] != 0x37) || (rom[2] != 0x12) || (rom[3] != 0x40))
     {
	printf("wrong file format !\n");
	free(rom);
	rom = NULL;
	return;
     }
   md5_init(&state);
   md5_append(&state, (const md5_byte_t *)rom, taille_rom);
   md5_finish(&state, digest);
   display_MD5calculating_progress(-1);
   free(rom);
   rom = NULL;
}
