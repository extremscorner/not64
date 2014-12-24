/* Copyright 2013 tueidj All Rights Reserved
 * This code may not be used in any project
 * without explicit permission from the author.
 */

#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <ogc/machine/processor.h>

#include "../gc_memory/MEM2.h"
#include "wii_vm.h"

#include <stdio.h>

// maximum virtual memory size
#define MAX_VM_SIZE      (256*1024*1024)
// maximum physical memory size
#define MAX_MEM_SIZE     ( 16*1024*1024)
// minimum physical memory size
#define MIN_MEM_SIZE     (256*1024)
// page size as defined by hardware
#define PAGE_SIZE        4096
#define PAGE_MASK        (~(PAGE_SIZE-1))

#define VM_VSID          0
#define VM_SEGMENT       0x70000000

// use 64KB for PTEs
#define HTABMASK         0
#define PTE_SIZE         ((HTABMASK+1)*65536)
#define PTE_COUNT        (PTE_SIZE>>3)

#define VM_FILENAME      "/tmp/pagefile.sys"

// keeps a record of each currently mapped page
typedef union
{
	u32 data;
	struct
	{
		// is this a valid physical page?
		u32 valid      :  1;
		// can this page be flushed?
		u32 locked     :  1;
		// does this page contain changes?
		u32 dirty      :  1;
		// PTE for this physical page
		u32 pte_index  : 13;
		// virtual page index for this physical page
		u32 page_index : 16;
	};
} p_map;

// maps VM addresses to mapped pages
typedef struct
{
	// data must be fetched when paging in?
	u16 committed  :  1;
	// physical page index for this virtual page
	u16 p_map_index: 12;
} vm_map;

typedef union
{
	u32 data[2];
	struct
	{
		u32 valid  :  1;
		u32 VSID   : 24;
		u32 hash   :  1;
		u32 API    :  6;

		u32 RPN    : 20;
		u32 pad0   :  3;
		u32 R      :  1;
		u32 C      :  1;
		u32 WIMG   :  4;
		u32 pad1   :  1;
		u32 PP     :  2;
	};
} PTE;
typedef PTE* PTEG;

typedef u8 vm_page[PAGE_SIZE];

static p_map phys_map[4096];
static vm_map virt_map[65536];
static u16 pmap_max, pmap_head;

static PTE* HTABORG;
static vm_page* VM_Base;
static vm_page* MEM_Base = (vm_page*)(ROMCACHE_LO);

static int pagefile_fd = -1;
static mutex_t vm_mutex = LWP_MUTEX_NULL;
static u32 vm_initialized = 0;

static __inline__ void tlbie(void* p)
{
	asm volatile("tlbie %0" :: "r"(p));
}

static u16 locate_oldest(void)
{
	u16 head = pmap_head;

	for(;;++head)
	{
		PTE *p;

		if (head >= pmap_max)
			head = 0;

		if (!phys_map[head].valid || phys_map[head].locked)
			continue;

		p = HTABORG+phys_map[head].pte_index;
		tlbie(VM_Base+phys_map[head].page_index);

		if (p->C)
		{
			p->C = 0;
			phys_map[head].dirty = 1;
			continue;
		}

		if (p->R)
		{
			p->R = 0;
			continue;
		}

		p->data[0] = 0;

		pmap_head = head+1;
		return head;
	}
}

static PTE* StorePTE(PTEG pteg, u32 virtual, u32 physical, u8 WIMG, u8 PP, int secondary)
{
	int i;
	PTE p = {{0}};

	p.valid = 1;
	p.VSID = VM_VSID;
	p.hash = secondary ? 1:0;
	p.API = virtual >> 22;
	p.RPN = physical >> 12;
	p.WIMG = WIMG;
	p.PP = PP;

	for (i=0; i < 8; i++)
	{
		if (pteg[i].data[0] == p.data[0])
		{
//			printf("Error: address %08x already had a PTE entry\n", virtual);
//			abort();
		}
		else if (pteg[i].valid)
			continue;

		asm volatile("tlbie %0" : : "r"(virtual));
		pteg[i].data[1] = p.data[1];
		pteg[i].data[0] = p.data[0];
//		if (i || secondary)
//			printf("PTE for address %08x/%08x in PTEG %p index %d (%s)\n", virtual, physical, pteg, i, secondary ? "secondary" : "primary");
		return pteg+i;
	}

	return NULL;
}

static PTEG CalcPTEG(u32 virtual, int secondary)
{
	uint32_t segment_index = (virtual >> 12) & 0xFFFF;
	u32 ptr = MEM_VIRTUAL_TO_PHYSICAL(HTABORG);
	u32 hash = segment_index ^ VM_VSID;

	if (secondary) hash = ~hash;

	hash &= (HTABMASK << 10) | 0x3FF;
	ptr |= hash << 6;

	return (PTEG)MEM_PHYSICAL_TO_K0(ptr);
}

static PTE* insert_pte(u16 index, u32 physical, u8 WIMG, u8 PP)
{
	PTE *pte;
	int i;
	u32 virtual = (u32)(VM_Base+index);

	for (i=0; i < 2; i++)
	{
		PTEG pteg = CalcPTEG(virtual, i);
		pte = StorePTE(pteg, virtual, physical, WIMG, PP, i);
		if (pte)
			return pte;
	}

//	printf("Failed to insert PTE for %p\n", VM_Base+index);
//	abort();

	return NULL;
}

static void tlbia(void)
{
	int i;
	for (i=0; i < 64; i++)
		asm volatile("tlbie %0" :: "r" (i*PAGE_SIZE));
}

void LoadingBar_showBar(float percent, const char* string);
void __exception_sethandler(u32 nExcept, void (*pHndl)());
extern void default_exceptionhandler();
extern void dsi_exceptionhandler();

void* VM_Init(size_t VMSize, size_t MEMSize)
{
	u32 i;
	u16 index, v_index;
	STACK_ALIGN(fstats,st,1,32);

	if (vm_initialized)
	{
		vm_initialized++;
		return VM_Base;
	}

	// parameter checking
	if (VMSize>MAX_VM_SIZE || MEMSize<MIN_MEM_SIZE || MEMSize>MAX_MEM_SIZE)
	{
		errno = EINVAL;
		return NULL;
	}

	VMSize = (VMSize+PAGE_SIZE-1)&PAGE_MASK;
	MEMSize = (MEMSize+PAGE_SIZE-1)&PAGE_MASK;
	VM_Base = (vm_page*)(0x80000000 - VMSize);
	pmap_max = MEMSize / PAGE_SIZE;

//	printf("VMSize %08x MEMSize %08x VM_Base %p pmap_max %u\n", VMSize, MEMSize, VM_Base, pmap_max);

	if (VMSize <= MEMSize)
	{
		errno = EINVAL;
		return NULL;
	}

	if (LWP_MutexInit(&vm_mutex, 0) != 0)
	{
		errno = ENOLCK;
		return NULL;
	}

	ISFS_Initialize();
	// doesn't matter if this fails, will be caught when file is opened
	ISFS_CreateFile(VM_FILENAME, 0, ISFS_OPEN_RW, ISFS_OPEN_RW, ISFS_OPEN_RW);

	pagefile_fd = ISFS_Open(VM_FILENAME, ISFS_OPEN_RW);
	if (pagefile_fd < 0)
	{
		errno = ENOENT;
		return NULL;
	}

	tlbia();
	DCZeroRange(MEM_Base, MEMSize);
	HTABORG = (PTE*)(((u32)MEM_Base+0xFFFF)&~0xFFFF);
//	printf("HTABORG: %p\n", HTABORG);

	/* Attempt to make the pagefile the correct size.
	 * it's not enough to just check for free space;
	 * IOS might download something in the background,
	 * plus we need to be able to quickly seek to any page
	 * within the file.
	 */
	ISFS_Seek(pagefile_fd, 0, SEEK_END);
	ISFS_GetFileStats(pagefile_fd, st);
	for (i=st->file_length; i<VMSize;)
	{
		u32 to_write = VMSize - i;
		if (to_write > MEMSize)
			to_write = MEMSize;

		LoadingBar_showBar((float)i/VMSize, "Growing NAND pagefile");
		if (ISFS_Write(pagefile_fd, MEM_Base, to_write) != to_write)
		{
			errno = ENOSPC;
			return NULL;
		}
//		printf("Wrote %u bytes to offset %u\n", to_write, i);
		i += to_write;
	}

	// initial commit: map pmap_max pages to fill PTEs with valid RPNs
	for (index=0,v_index=0; index<pmap_max; ++index,++v_index)
	{
		if ((PTE*)(MEM_Base+index) == HTABORG)
		{
//			printf("p_map hole: %u -> %u\n", index, index+(PTE_SIZE/PAGE_SIZE));
			for (i=0; i<(PTE_SIZE/PAGE_SIZE); ++i,++index)
				phys_map[index].valid = 0;

			--index, --v_index;
			continue;
		}

		phys_map[index].valid = 1;
		phys_map[index].locked = 0;
		phys_map[index].dirty = 0;
		phys_map[index].page_index = v_index;
		phys_map[index].pte_index = insert_pte(v_index, MEM_VIRTUAL_TO_PHYSICAL(MEM_Base+index), 0, 0b10) - HTABORG;
		virt_map[v_index].committed = 0;
		virt_map[v_index].p_map_index = index;
	}

	// all indexes up to 65536
	for (; v_index; ++v_index)
	{
		virt_map[v_index].committed = 0;
		virt_map[v_index].p_map_index = pmap_max;
	}

	pmap_head = 0;

	// set SDR1
	mtspr(25, MEM_VIRTUAL_TO_PHYSICAL(HTABORG)|HTABMASK);
//	printf("SDR1: %08x\n", MEM_VIRTUAL_TO_PHYSICAL(HTABORG));
	// enable SR
	asm volatile("mtsrin %0,%1" :: "r"(VM_VSID), "r"(VM_Base));
	// hook DSI
	__exception_sethandler(EX_DSI, dsi_exceptionhandler);

	vm_initialized = 1;

	return VM_Base;
}

void VM_Deinit(void)
{
	if (--vm_initialized)
		return;

	// disable SR
	asm volatile("mtsrin %0,%1" :: "r"(0x80000000), "r"(VM_Base));
	// restore default DSI handler
	__exception_sethandler(EX_DSI, default_exceptionhandler);

	if (vm_mutex != LWP_MUTEX_NULL)
	{
		LWP_MutexDestroy(vm_mutex);
		vm_mutex = LWP_MUTEX_NULL;
	}

	if (pagefile_fd)
	{
		ISFS_Close(pagefile_fd);
		pagefile_fd = -1;
	}
}

void VM_InvalidateAll(void)
{
	u16 index;
	u32 irq;

	if (!vm_initialized)
		return;

	LWP_MutexLock(vm_mutex);

	_CPU_ISR_Disable(irq);

	tlbia();

	for (index=0; index < pmap_max; index++)
	{
		if (phys_map[index].valid)
		{
			PTE *p = HTABORG+phys_map[index].pte_index;

			// clear reference bits
			p->R = 0;
			p->C = 0;
			// unlock
			phys_map[index].locked = 0;
			// page is clean
			phys_map[index].dirty = 0;
			// clear physical memory
			DCZeroRange(MEM_Base+index, PAGE_SIZE);
		}

		virt_map[index].committed = 0;
	}

	for (; index; index++)
		virt_map[index].committed = 0;

	pmap_head = 0;

	_CPU_ISR_Restore(irq);

//	printf("VM was invalidated\n");

	LWP_MutexUnlock(vm_mutex);
}

int vm_dsi_handler(u32 DSISR, u32 DAR)
{
	u16 virt_index;
	u16 phys_index;
	u16 flush_v_index;

	if (DAR<(u32)VM_Base || DAR>=0x80000000)
		return 0;
	if ((DSISR&~0x02000000)!=0x40000000)
		return 0;
	if (!vm_initialized)
		return 0;

	LWP_MutexLock(vm_mutex);

	DAR &= ~0xFFF;
	virt_index = (vm_page*)DAR - VM_Base;

	phys_index = locate_oldest();

	// purge phys_index if it's dirty
	if (phys_map[phys_index].dirty)
	{
		unsigned int pages_to_flush;

		flush_v_index = phys_map[phys_index].page_index;
		virt_map[flush_v_index].committed = 1;
		// mark this virtual page as unmapped
		virt_map[flush_v_index].p_map_index = pmap_max;
		phys_map[phys_index].dirty = 0;

		// optimize by flushing up to four dirty pages at once
		for (pages_to_flush=1; pages_to_flush < 4; pages_to_flush++)
		{
			PTE *p;

			// check for end of physical mem
			if (phys_index+pages_to_flush >= pmap_max)
				break;

			// check for p_map hole
			if (!phys_map[pmap_head].valid)
				break;

			// make sure physical pages hold consecutive virtual pages
			if (phys_map[pmap_head].page_index != flush_v_index+pages_to_flush)
				break;

			// page should only be flushed if it's dirty
			p = HTABORG+phys_map[pmap_head].pte_index;
			if (p->C)
			{
				tlbie(VM_Base+phys_map[pmap_head].page_index);
				p->C = 0;
			}
			else if (!phys_map[pmap_head].dirty)
				break;

			virt_map[flush_v_index+pages_to_flush].committed = 1;
			phys_map[pmap_head].dirty = 0;
			pmap_head++;
		}

		ISFS_Seek(pagefile_fd, flush_v_index*PAGE_SIZE, SEEK_SET);
		ISFS_Write(pagefile_fd, MEM_Base+phys_index, PAGE_SIZE*pages_to_flush);
//		printf("VM page %d was purged (%d)\n", phys_map[phys_index].page_index, pages_to_flush);
	}

	// fetch virtual_index if it has been previously committed
	if (virt_map[virt_index].committed)
	{
		ISFS_Seek(pagefile_fd, virt_index*PAGE_SIZE, SEEK_SET);
		ISFS_Read(pagefile_fd, MEM_Base+phys_index, PAGE_SIZE);
//		printf("VM page %d was fetched\n", virt_index);
	}
	else
		DCZeroRange(MEM_Base+phys_index, PAGE_SIZE);

//	printf("VM page %u (0x%08x) replaced page %u (%p) @ %p\n", virt_index, DAR, phys_map[phys_index].page_index, VM_Base+phys_map[phys_index].page_index, MEM_Base+phys_index);

	virt_map[virt_index].p_map_index = phys_index;
	phys_map[phys_index].page_index = virt_index;
	phys_map[phys_index].pte_index = insert_pte(virt_index, MEM_VIRTUAL_TO_PHYSICAL(MEM_Base+phys_index), 0, 0b10) - HTABORG;

	LWP_MutexUnlock(vm_mutex);

	return 1;
}
