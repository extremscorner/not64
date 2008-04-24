#include <ogc/ipc.h>

void killWiimote(void){

	u8  bmRequestType = 0x20;
	u8  bmRequest     = 0;
	u16 wValue        = 0;
	u16 wIndex        = 0;
	u16 wLength       = 0x0300;
	u8  unknown       = 0;
	u8  data[3] = { 0x03, 0x0c, 0x00 };
	
	ioctlv* resetBTVector = malloc( 6*sizeof(ioctlv) + 1*sizeof(ioctlv) );
	resetBTVector[0] = (ioctlv) { &bmRequestType, sizeof(bmRequestType) };
	resetBTVector[1] = (ioctlv) { &bmRequest,     sizeof(bmRequest)     };
	resetBTVector[2] = (ioctlv) { &wValue,        sizeof(wValue)        };
	resetBTVector[3] = (ioctlv) { &wIndex,        sizeof(wIndex)        };
	resetBTVector[4] = (ioctlv) { &wLength,       sizeof(wLength)       };
	resetBTVector[5] = (ioctlv) { &unknown,       sizeof(unknown)       };
	resetBTVector[6] = (ioctlv) { &data,          sizeof(data)          };
	
	int btfd = IOS_Open("/dev/usb/oh1/57e/305", 2);
	IOS_Ioctlv(btfd, 0, 6, 1, resetBTVector);
	IOS_Close(btfd);
	
	free(resetBTVector);
}

