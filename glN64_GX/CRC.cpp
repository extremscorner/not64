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

#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned long CRCTable[ 256 ];

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

    for (int i = 0; i <= 255; i++)
	{
        crc = Reflect( i, 8 ) << 24;
        for (int j = 0; j < 8; j++)
			crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);
        
        CRCTable[i] = Reflect( crc, 32 );
    }
}

DWORD CRC_Calculate( DWORD crc, void *buffer, DWORD count )
{
    BYTE *p;
	DWORD orig = crc;

    p = (BYTE*) buffer;
	while (count--) 
#ifndef _BIG_ENDIAN
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
#else // !_BIG_ENDIAN -> Big Endian fix - necessary for Ucode detection.
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *(BYTE*)((int)p++ ^ 3)]; //This fixes the endian problem for uc_crc
#endif // _BIG_ENDIAN

    return crc ^ orig;
}

DWORD CRC_CalculatePalette( DWORD crc, void *buffer, DWORD count )
{
    BYTE *p;
	DWORD orig = crc;

    p = (BYTE*) buffer;
    while (count--)
	{
#ifndef _BIG_ENDIAN
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

		p += 6;
#else // !_BIG_ENDIAN -> Big Endian fix  
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *(BYTE*)((int)p + 1)];
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *(BYTE*)((int)p)];

		p += 8;
#endif // _BIG_ENDIAN
	}

    return crc ^ orig;
}
