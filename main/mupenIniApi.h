/**
 * Mupen64 - mupenIniApi.h
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

#ifndef MUPENINIAPI_H
#define MUPENINIAPI_H

typedef struct
{
   char goodname[100];
   int eeprom16kb;
   char MD5[33];
   char CRC[22];
   char refMD5[33];
   char comments[200];
} mupenEntry;

void ini_openFile();
void ini_closeFile();
void ini_updateFile(int compress);
mupenEntry* ini_search_by_md5(const char *md5);
mupenEntry* ini_search_by_CRC(const char *CRC);

#endif
