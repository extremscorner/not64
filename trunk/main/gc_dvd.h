/* gc_dvd.h - dvd basic functions for iso9660 parsing, code borrowed from qoob mp3 player
   by emu_kidid for Mupen64-GC
 */

#ifndef GC_DVD_H
#define GC_DVD_H

#include <stdint.h>
//Used in ISO9660 Parsing
#define NO_FILES -1
#define NO_ISO9660_DISC -2
#define FATAL_ERROR -3
#define MAXIMUM_ENTRIES_PER_DIR 2048

typedef struct
{
	char name[128];
	int flags;
	int sector, size;
} file_entry; 

typedef struct
{
	file_entry file[MAXIMUM_ENTRIES_PER_DIR];
} file_entries; 

extern file_entries *DVDToc;

void dvd_motor_off();
unsigned int dvd_get_error(void);
int dvd_read_directoryentries(uint64_t offset, int size);
void read_directory(int sector, int len);
int read_safe(void* dst, uint64_t offset, int len);
int read_direntry(unsigned char* direntry);
int read_sector(void* buffer, uint32_t sector);
int dvd_read(void* dst,unsigned int len, unsigned int offset);
int is_unicode;
int files;
int dvd_read_id();

struct pvd_s
{
	char id[8];
	char system_id[32];
	char volume_id[32];
	char zero[8];
	unsigned long total_sector_le, total_sect_be;
	char zero2[32];
	unsigned long volume_set_size, volume_seq_nr;
	unsigned short sector_size_le, sector_size_be;
	unsigned long path_table_len_le, path_table_len_be;
	unsigned long path_table_le, path_table_2nd_le;
	unsigned long path_table_be, path_table_2nd_be;
	unsigned char root_direntry[34];
	char volume_set_id[128], publisher_id[128], data_preparer_id[128], application_id[128];
	char copyright_file_id[37], abstract_file_id[37], bibliographical_file_id[37];
	// some additional dates, but we don't care for them :)
}  __attribute__((packed));

#endif

