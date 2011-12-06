#ifndef __FASTMEMCPY_H__
#define __FASTMEMCPY_H__

#ifdef __cplusplus
	extern "C" {
#endif

void *fast_memcpy(void *to, const void *from, size_t len);

#ifdef __cplusplus
	}
#endif

#endif
