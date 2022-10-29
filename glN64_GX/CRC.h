/**
 * glN64_GX - CRC.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef __LINUX__
# include <windows.h>
#else
# include "../main/winlnxdefs.h"
#endif // __LINUX__
#define XXH_INLINE_ALL
#define XXH_FORCE_NATIVE_FORMAT 1
#include "../main/xxhash.h"

void CRC_BuildTable();

DWORD CRC_Calculate( DWORD crc, void *buffer, DWORD count );
DWORD CRC_CalculatePalette( DWORD crc, void *buffer, DWORD count );

inline DWORD Hash_Calculate( DWORD hash, void *buffer, DWORD count )
{
	return XXH32(buffer, count, hash);
}
