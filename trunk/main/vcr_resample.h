#include "../config.h"

#ifdef VCR_SUPPORT

#ifndef __VCR_RESAMPLE__
#define __VCR_RESAMPLE__

int VCR_resample( short **dst, int dst_freq,
                  const short *src, int src_freq, int src_bitrate, int src_len );

#endif // __VCR_RESAMPLE__

#endif // VCR_SUPPORT
