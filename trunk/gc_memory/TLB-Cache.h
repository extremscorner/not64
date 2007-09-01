/* TLB-Cache.h - This is how the TLB LUT should be accessed, this way it won't waste RAM
   by Mike Slegeir for Mupen64-GC
 */

#ifndef TLB_CACHE_H
#define TLB_CACHE_H

void TLBCache_init(void);
void TLBCache_deinit(void);

unsigned int inline TLBCache_get_r(unsigned int page);
unsigned int inline TLBCache_get_w(unsigned int page);

void inline TLBCache_set_r(unsigned int page, unsigned int val);
void inline TLBCache_set_w(unsigned int page, unsigned int val);

#endif

