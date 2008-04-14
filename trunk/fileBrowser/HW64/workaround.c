#include <stdio.h>
#include <string.h>
#include <ogcsys.h>

#include "../../gui/GUI.h"
#include "../../main/gc_dvd.h"

struct _toc
{
	u32 bootInfoCnt;
	u32 partInfoOff;
};

struct _pinfo
{
	u32 offset;
	u32 len;
};

static vu32 dvddone = 0;
static u32 *__dvd_Tmd = NULL;
static struct _toc *__dvd_gameToc = NULL;
static struct _pinfo *__dvd_partInfo = NULL;
static struct _pinfo *__dvd_bootGameInfo = NULL;

static dvddiskid *g_diskID = (dvddiskid*)0x81700000;

static char _gameTocBuffer[0x20] ATTRIBUTE_ALIGN(32);
static char _gamePartInfo[0x20] ATTRIBUTE_ALIGN(32);
static char _gameTmdBuffer[18944] ATTRIBUTE_ALIGN(32);
static char _gameIOSViews[48384] ATTRIBUTE_ALIGN(32);

static void __dvd_readidcb(s32 result)
{
	printf("__dvd_readidcb(%d)\n",result);
	//hexdump(g_diskID,32);
	dvddone = result;
}

int getTitle(void** tik, unsigned int* tikSize, void** tmd, unsigned int* tmdSize, void** cert, unsigned int* certSize) {
#if 1 
        s32 ret;
        u32 i;
		char txt[128];
	GUI_clear();
	ret = WiiDVD_Init();

	WiiDVD_Reset();

	ret = WiiDVD_ReadID(g_diskID);

	__dvd_gameToc = (struct _toc*)_gameTocBuffer;
	ret = WiiDVDReadUnEncrypted(_gameTocBuffer,0x20,0x000040000);
	//hexdump(_gameTocBuffer,0x20);
	if(ret < 0) GUI_print("Reading _gameTocBuffer failed\n");
	else{
		sprintf(txt, "__dvd_gameToc->partInfoOff\n\t= %08x\n", __dvd_gameToc->partInfoOff);
		GUI_print(txt);
	}

	__dvd_partInfo = (struct _pinfo*)_gamePartInfo;
	ret = WiiDVDReadUnEncrypted(_gamePartInfo,0x20,__dvd_gameToc->partInfoOff);
	//hexdump(_gamePartInfo,0x20);
	if(ret < 0) GUI_print("Reading _gamePartInfo failed\n");

	i = 0;
	__dvd_bootGameInfo = NULL;
	sprintf(txt, "__dvd_gameToc->bootInfoCnt: %d\n",__dvd_gameToc->bootInfoCnt);
	GUI_print(txt);

	while(i<__dvd_gameToc->bootInfoCnt) {
		sprintf(txt, "__dvd_partInfo[i].len: %d\n",__dvd_partInfo[i].len);
		GUI_print(txt);
		if(__dvd_partInfo[i].len==0) {
			__dvd_bootGameInfo = __dvd_partInfo+i;
		}
		i++;
	}
	
	sprintf(txt, "__dvd_bootGameInfo->offset: %08x\n", __dvd_bootGameInfo->offset);
	GUI_print(txt);
	
	GUI_draw();
	while(!PAD_ButtonsHeld(0));

	//printf("__dvd_bootGameInfo: %p\n",__dvd_bootGameInfo);
	//printf("__dvd_bootGameInfo->offset: %08x\n",__dvd_bootGameInfo->offset);

	__dvd_Tmd = (u32*)_gameTmdBuffer;
	ret = WiiDVD_LowOpenPartition(__dvd_bootGameInfo->offset,NULL,0,NULL,(void*)_gameTmdBuffer);
	//printf("ret: %d\n",ret);
	if(ret >= 0){
		*tik = _gameTmdBuffer;
		*tikSize = 0x2a4;
		struct {
			u32 tmd_size;
			u32 tmd_offset;
			u32 cert_size;
			u32 cert_offset;
		} b;
		memcpy(&b, _gameTmdBuffer+0x2a4, sizeof(b));
		*tmdSize = b.tmd_size;
		*tmd = _gameTmdBuffer + b.tmd_offset;
		*certSize = b.cert_size;
		*cert = _gameTmdBuffer + b.cert_offset;
	}
	
	return ret;
#endif
}

