/**
 * Mupen64 - rom.h
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

#ifndef ROM_H
#define ROM_H

#include "../fileBrowser/fileBrowser.h"

int rom_read(fileBrowser_file*);
int fill_header(fileBrowser_file*);
void calculateMD5(fileBrowser_file*, unsigned char digest[16]);
extern unsigned char *rom;
#ifndef __PPC__
extern int taille_rom;
#else
extern int rom_length;
#endif

typedef struct _rom_header
{
   unsigned char init_PI_BSB_DOM1_LAT_REG;
   unsigned char init_PI_BSB_DOM1_PGS_REG;
   unsigned char init_PI_BSB_DOM1_PWD_REG;
   unsigned char init_PI_BSB_DOM1_PGS_REG2;
   unsigned long ClockRate;
   unsigned long PC;
   unsigned long Release;
   unsigned long CRC1;
   unsigned long CRC2;
   unsigned long Unknown[2];
   unsigned char nom[20];
   unsigned long unknown;
   unsigned long Manufacturer_ID;
   unsigned short Cartridge_ID;
   unsigned short Country_code;
   unsigned long Boot_Code[1008];
} rom_header;
extern rom_header *ROM_HEADER;

typedef struct _rom_settings
{
   char goodname[256];
   int eeprom_16kb;
   char MD5[33];
} rom_settings;
extern rom_settings ROM_SETTINGS;

int init_byte_swap(unsigned int magicWord);

void countrycodestring(unsigned short countrycode, char *string);
char *saveregionstr();

#endif
