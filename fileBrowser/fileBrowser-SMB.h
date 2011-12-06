/**
 * WiiSX - fileBrowser-SMB.h
 * Copyright (C) 2010 emu_kidid
 * 
 * fileBrowser module for Samba based shares
 *
 * WiiSX homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
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
**/


#ifndef FILE_BROWSER_SMB_H
#define FILE_BROWSER_SMB_H

#include "fileBrowser.h"

// error codes
#define SMB_NETINITERR -110
#define SMB_SMBCFGERR  -111
#define SMB_SMBERR -112

extern fileBrowser_file topLevel_SMB;

int fileBrowser_SMB_readDir(fileBrowser_file*, fileBrowser_file**);
int fileBrowser_SMB_readFile(fileBrowser_file*, void*, unsigned int);
int fileBrowser_SMB_seekFile(fileBrowser_file*, unsigned int, unsigned int);
int fileBrowser_SMB_init(fileBrowser_file* file);
int fileBrowser_SMB_deinit(fileBrowser_file* file);

void init_network_thread();
void pause_netinit_thread();
void resume_netinit_thread();

#endif

