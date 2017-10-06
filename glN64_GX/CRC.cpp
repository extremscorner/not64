/**
 * glN64_GX - CRC.cpp
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
#define XXH_PRIVATE_API
#define XXH_FORCE_MEMORY_ACCESS 2
#define XXH_FORCE_NATIVE_FORMAT 1
#define XXH_FORCE_ALIGN_CHECK 0
#include "../main/xxhash.h"

#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned long CRCTable[ 256 * 4 ];

DWORD Reflect( DWORD ref, char ch )
{
     DWORD value = 0;

     // Swap bit 0 for bit 7
     // bit 1 for bit 6, etc.
     for (int i = 1; i < (ch + 1); i++)
     {
          if(ref & 1)
               value |= 1 << (ch - i);
          ref >>= 1;
     }
     return value;
}

void CRC_BuildTable()
{
    DWORD crc;

    for (int i = 0; i < 256; i++)
	{
        crc = Reflect( i, 8 ) << 24;
        for (int j = 0; j < 8; j++)
			crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);
        
        CRCTable[i] = Reflect( crc, 32 );
    }

    for (int i = 0; i < 256; i++)
    {
        for (int j = 0; j < 3; j++)
            CRCTable[256 * (j + 1) + i] = (CRCTable[256 * j + i] >> 8) ^ CRCTable[CRCTable[256 * j + i] & 0xFF];
}
}

DWORD CRC_Calculate( DWORD crc, void *buffer, DWORD count )
{
    BYTE *p;
	DWORD orig = crc;

    p = (BYTE*) buffer;

    while (count > 3)
    {
        crc ^= *(unsigned int*) p; p += 4;
        crc = CRCTable[3 * 256 + (crc & 0xFF)]
            ^ CRCTable[2 * 256 + ((crc >> 8) & 0xFF)]
            ^ CRCTable[1 * 256 + ((crc >> 16) & 0xFF)]
            ^ CRCTable[0 * 256 + ((crc >> 24))];

        count -= 4;
    }

	while (count--) 
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

    return crc ^ orig;
}

DWORD CRC_CalculatePalette( DWORD crc, void *buffer, DWORD count )
{
    BYTE *p;
	DWORD orig = crc;

    p = (BYTE*) buffer;
    while (count--)
	{
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

		p += 6;
	}

    return crc ^ orig;
}

DWORD Hash_Calculate( DWORD hash, void *buffer, DWORD count )
{
    return XXH32(buffer, count, hash);
}
