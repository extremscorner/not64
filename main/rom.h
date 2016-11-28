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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
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
	unsigned int ClockRate;
	unsigned int PC;
	unsigned int Release;
	unsigned int CRC1;
	unsigned int CRC2;
	unsigned int Unknown[2];
	unsigned char Name[20];
	unsigned int unknown;
	unsigned int Manufacturer_ID;
	unsigned short Cartridge_ID;
	unsigned short Country_code;
} rom_header;
extern rom_header ROM_HEADER;

typedef struct _rom_settings
{
   char goodname[256];
   char isEEPROM16k;
} rom_settings;
extern rom_settings ROM_SETTINGS;

int init_byte_swap(unsigned int magicWord);

void countrycodestring(unsigned short countrycode, char *string);
char *saveregionstr();

#endif
