#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ogc/dvd.h>
#include <malloc.h>
#include <string.h>
#include <gccore.h>
#include "gc_dvd.h"
#ifdef WII
#include <di/di.h>
#endif

extern int dvdInitialized;
int last_current_dir = -1;
volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
file_entries *DVDToc = NULL; //Dynamically allocate this

#ifdef HW_DOL
int dvd_read_id()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0x80000000;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	while (dvd[7] & 1);
	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

#else

int dvd_read_id(){
	return 0;
}
#endif

#ifdef HW_DOL
unsigned int dvd_get_error(void)
{
	dvd[2] = 0xE0000000;
	dvd[8] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
	return dvd[8];
}
#else
unsigned int dvd_get_error(void)
{
	unsigned int val;
	DI_GetError(&val);
	return val;
}
#endif

#ifdef HW_DOL
void dvd_motor_off()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xe3000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
}
#else
void dvd_motor_off(){
}
#endif


#ifdef HW_DOL
int dvd_read(void* dst, unsigned int len, unsigned int offset)
{
	if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
		dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0xA8000000;
	dvd[3] = offset >> 2;
	dvd[4] = len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
	DCInvalidateRange(dst, len);
	while (dvd[7] & 1);

	if (dvd[0] & 0x4)
		return 1;
	return 0;
}
#endif

#ifdef HW_DOL
int read_sector(void* buffer, uint32_t sector)
{
	return dvd_read(buffer, 2048, sector * 2048);
}
#else
int read_sector(void* buffer, uint32_t sector){
	uint8_t *read_buf = (uint8_t*)memalign(32,0x800);
	int ret;
	int retrycount = 0x20;

	while(DI_GetStatus() & DVD_INIT);

	ret = DI_ReadDVD(read_buf, 1, sector);

	memcpy(buffer, read_buf, 0x800);
	free(read_buf);
	return ret;
}
#endif

int read_safe(void* dst, uint64_t offset, int len)
{
	int ol = len;
	int ret = 0;	
  unsigned char* sector_buffer = (unsigned char*)memalign(32,2048);
	while (len)
	{
		uint32_t sector = offset / 2048;
		ret |= read_sector(sector_buffer, sector);
		uint32_t off = offset & 2047;

		int rl = 2048 - off;
		if (rl > len)
			rl = len;
		memcpy(dst, sector_buffer + off, rl);	

		offset += rl;
		len -= rl;
		dst += rl;
	}
	free(sector_buffer);
	if(ret)
  	return -1;
  	
  if(dvd_get_error())
  	  dvdInitialized=0;

	return ol;
}

int read_direntry(unsigned char* direntry)
{
       int nrb = *direntry++;
       ++direntry;

       int sector;

       direntry += 4;
       sector = (*direntry++) << 24;
       sector |= (*direntry++) << 16;
       sector |= (*direntry++) << 8;
       sector |= (*direntry++);        

       int size;

       direntry += 4;

       size = (*direntry++) << 24;
       size |= (*direntry++) << 16;
       size |= (*direntry++) << 8;
       size |= (*direntry++);

       direntry += 7; // skip date

       int flags = *direntry++;
       ++direntry; ++direntry; direntry += 4;

       int nl = *direntry++;

       char* name = DVDToc->file[files].name;

       DVDToc->file[files].sector = sector;
       DVDToc->file[files].size = size;
       DVDToc->file[files].flags = flags;

       if ((nl == 1) && (direntry[0] == 1)) // ".."
       {
               DVDToc->file[files].name[0] = 0;
               if (last_current_dir != sector)
                       files++;
       }
       else if ((nl == 1) && (direntry[0] == 0))
       {
               last_current_dir = sector;
       }
       else
       {
               if (is_unicode)
               {
                       int i;
                       for (i = 0; i < (nl / 2); ++i)
                               name[i] = direntry[i * 2 + 1];
                       name[i] = 0;
                       nl = i;
               }
               else
               {
                       memcpy(name, direntry, nl);
                       name[nl] = 0;
               }

               if (!(flags & 2))
               {
                       if (name[nl - 2] == ';')
                               name[nl - 2] = 0;

                       int i = nl;
                       while (i >= 0)
                               if (name[i] == '.')
                                       break;
                               else
                                       --i;

                       ++i;

               }
               else
               {
                       name[nl++] = '/';
                       name[nl] = 0;
               }

               files++;
       }

       return nrb;
}



void read_directory(int sector, int len)
{
  int ptr = 0;
  unsigned char *sector_buffer = (unsigned char*)memalign(32,2048);
  read_sector(sector_buffer, sector);
  
  files = 0;
  memset(DVDToc,0,sizeof(file_entries));
  while (len > 0)
  {
    ptr += read_direntry(sector_buffer + ptr);
    if (ptr >= 2048 || !sector_buffer[ptr])
    {
      len -= 2048;
      sector++;
      read_sector(sector_buffer, sector);
      ptr = 0;
    }
  }
  free(sector_buffer);
}

int dvd_read_directoryentries(uint64_t offset, int size) {
  int sector = 16;
  unsigned char *bufferDVD = (unsigned char*)memalign(32,2048);
  struct pvd_s* pvd = 0;
  struct pvd_s* svd = 0;
  
  if(DVDToc)
  {
    free(DVDToc);
    DVDToc = NULL;
  }
  DVDToc = memalign(32,sizeof(file_entries));
  
  while (sector < 32)
  {
    if (read_sector(bufferDVD, sector))
    {
      free(bufferDVD);
      free(DVDToc);
      DVDToc = NULL;
      return FATAL_ERROR;
    }
    if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\2CD001\1", 8))
    {
      svd = (void*)bufferDVD;
      break;
    }
    ++sector;
  }
  
  
  if (!svd)
  {
    sector = 16;
    while (sector < 32)
    {
      if (read_sector(bufferDVD, sector))
      {
        free(bufferDVD);
        free(DVDToc);
        DVDToc = NULL;
        return FATAL_ERROR;
      }
      
      if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\1CD001\1", 8))
      {
        pvd = (void*)bufferDVD;
        break;
      }
      ++sector;
    }
  }
  
  if ((!pvd) && (!svd))
  {
    free(bufferDVD);
    free(DVDToc);
    DVDToc = NULL;
    return NO_ISO9660_DISC;
  }
  
  files = 0;
  if (svd)
  {
    is_unicode = 1;
    read_direntry(svd->root_direntry);
  }
  else
  {
    is_unicode = 0;
    read_direntry(pvd->root_direntry);
  }
  
  if((size + offset) == 0)  // enter root
    read_directory(DVDToc->file[0].sector, DVDToc->file[0].size);
  else
    read_directory((offset/2048), size);

  free(bufferDVD);
  if(files>0)
    return files;
  return NO_FILES;
}




