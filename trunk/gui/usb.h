///////////////////////////////////////////////////////////////////////////////////////////////
//								USB Gecko - USB library	(c) Nuke							 //
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __USB_H
#define __USB_H

///////////////////////////////////////////////////////////////////////////////////////////////
//									Functions												 

/*	channel 	device 	freq 	offset 	Description
		0 			0 	4 	  	Memory Card (Slot A)
		1		 	0 	4 	  	Memory Card (Slot B)
*/

#define exi_chan0sr			*(volatile unsigned int*) 0xCC006800 // Channel 0 Status Register
#define exi_chan1sr			*(volatile unsigned int*) 0xCC006814 // Channel 1 Status Register
#define exi_chan2sr			*(volatile unsigned int*) 0xCC006828 // Channel 2 Status Register
#define exi_chan0cr			*(volatile unsigned int*) 0xCC00680c // Channel 0 Control Register
#define exi_chan1cr			*(volatile unsigned int*) 0xCC006820 // Channel 1 Control Register
#define exi_chan2cr			*(volatile unsigned int*) 0xCC006834 // Channel 2 Control Register
#define exi_chan0data		*(volatile unsigned int*) 0xCC006810 // Channel 0 Immediate Data
#define exi_chan1data		*(volatile unsigned int*) 0xCC006824 // Channel 1 Immediate Data
#define exi_chan2data		*(volatile unsigned int*) 0xCC006838 // Channel 2 Immediate Data
#define exi_chan0dmasta		*(volatile unsigned int*) 0xCC006804 // Channel 0 DMA Start address
#define exi_chan1dmasta		*(volatile unsigned int*) 0xCC006818 // Channel 1 DMA Start address
#define exi_chan2dmasta		*(volatile unsigned int*) 0xCC00682c // Channel 2 DMA Start address
#define exi_chan0dmalen		*(volatile unsigned int*) 0xCC006808 // Channel 0 DMA Length
#define exi_chan1dmalen		*(volatile unsigned int*) 0xCC00681c // Channel 1 DMA Length
#define exi_chan2dmalen		*(volatile unsigned int*) 0xCC006830 // Channel 2 DMA Length


void usb_flush();
unsigned int usb_checkgecko();
unsigned int usb_sendbyte (unsigned char sendbyte);
unsigned int usb_receivebyte (unsigned char* receivebyte);
void usb_sendbuffer (void *buffer, unsigned int size);
void usb_receivebuffer (void *buffer, unsigned int size);
unsigned int usb_checksendstatus();
unsigned int usb_checkreceivestatus();

///////////////////////////////////////////////////////////////////////////////////////////////

#endif
