///////////////////////////////////////////////////////////////////////////////////////////////
//					USB Gecko - USB library	- (c) Nuke	-  www.usbgecko.com					 //
///////////////////////////////////////////////////////////////////////////////////////////////

#include "usb.h"

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_flush											 //	 
//			Flushes the FIFO, Use at the start of your program to avoid trash				 //
///////////////////////////////////////////////////////////////////////////////////////////////
void usb_flush()
{
 unsigned char tempbyte;
 while (usb_receivebyte (&tempbyte) == 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_sendbyte										 //	 
//					Send byte to Gamecube/Wii over EXI memory card port						 //
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int usb_sendbyte (unsigned char sendbyte)
{
	unsigned int i = 0;
	exi_chan1sr = 0x000000D0;						// Memory Card Port B (Channel 1, Device 0, Frequnecy 3 (32Mhz Clock)
	exi_chan1data = 0xB0000000 | (sendbyte<<20);	
	exi_chan1cr = 0x19;								
	while((exi_chan1cr)&1);							
	i = exi_chan1data;								
	exi_chan1sr = 0x0;  
	if (i&0x04000000){
		return 1;									// Return 1 if byte was sent
	}   
    return 0;										
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_receivebyte									 //	 
//				Receive byte from Gamecube/Wii over EXI memory card port					 //
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int usb_receivebyte (unsigned char* receivebyte)
{
	unsigned int i = 0;
	exi_chan1sr = 0x000000D0;			// Memory Card Port B (Channel 1, Device 0, Frequnecy 3 (32Mhz Clock)
	exi_chan1data = 0xA0000000;			// A - Send Read byte Command
	exi_chan1cr = 0x19;					
	while((exi_chan1cr)&1);				
	i = exi_chan1data;					
	exi_chan1sr = 0x0;  
	if (i&0x08000000){
	    *receivebyte=(i>>16)&0xff;		
	    return 1;						// Return 1 if went ok
	} 
	return 0;							
	
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								 Function: usb_sendbuffer	 								 //	 
//								Simple buffer send routine									 //
///////////////////////////////////////////////////////////////////////////////////////////////
void usb_sendbuffer (void *buffer, unsigned int size)
{
	unsigned char *sendbyte = (unsigned char*) buffer;
	unsigned int bytesleft = size;		// Number of bytes left
	unsigned int returnvalue;

	while (bytesleft  > 0)
	{
		returnvalue = usb_sendbyte(*sendbyte);		// ok send it
		if(returnvalue == 1) {						// Was transfer valid?
				sendbyte++;							// yes, so Increase buffer
				bytesleft--;						// and decrease counterr
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//							  Function: usb_sendbuffer	 									 //	 
//						Simple buffer receive routine with extra check						 //
///////////////////////////////////////////////////////////////////////////////////////////////
void usb_receivebuffer (void *buffer, unsigned int size)
{
	unsigned char *receivebyte = (unsigned char*)buffer;
	unsigned int bytesleft = size;						// Number of bytes left
	unsigned int returnvalue;

	while (bytesleft > 0)
	{
		if(usb_checkreceivestatus()) {
			returnvalue = usb_receivebyte(receivebyte);	// ok grab byte off the fifo
			if(returnvalue == 1) {						// Was transfer valid?
				receivebyte++;							// yes, so Increase buffer
				bytesleft--;							// and decrease counter
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_checksendstatus	 							 //	 
//									Not really needed										 //
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int usb_checksendstatus()
{
	unsigned int i = 0;
	exi_chan1sr = 0x000000D0;						// Memory Card Port B (Channel 1, Device 0, Frequnecy 3 (32Mhz Clock)
	exi_chan1data = 0xC0000000;						// C - Check were ready to send
	exi_chan1cr = 0x19;								
	while((exi_chan1cr)&1);							
	i = exi_chan1data;									
	exi_chan1sr = 0x0;  
	if (i&0x04000000){
		return 1;									// Returns 1 if adapter is ready to send
	}   
    return 0;										
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_checkreceivestatus 							 //	 
//				Required if receiving hard speed packets to stop overflow of FIFO			 //
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int usb_checkreceivestatus()
{
	unsigned int i = 0;
	exi_chan1sr = 0x000000D0;						// Memory Card Port B (Channel 1, Device 0, Frequnecy 3 (32Mhz Clock)
	exi_chan1data = 0xD0000000;						// D - Check were ready to receive
	exi_chan1cr = 0x19;								
	while((exi_chan1cr)&1);							   	
	i = exi_chan1data;								
	exi_chan1sr = 0x0;  
	if (i&0x04000000){
		return 1;									// Returns 1 if adapter is ready to receive
	}   
    return 0;										
}

///////////////////////////////////////////////////////////////////////////////////////////////
//								Function: usb_checkgecko									 //	 
//			Check that gecko is attached, use to detect before flushing						 //
///////////////////////////////////////////////////////////////////////////////////////////////
unsigned int usb_checkgecko()
{
	unsigned int i = 0;
	exi_chan1sr = 0x000000D0;						// Memory Card Port B (Channel 1, Device 0, Frequnecy 3 (32Mhz Clock)
	exi_chan1data = 0x90000000;						// 9 - Check gecko is attached to PC
	exi_chan1cr = 0x19;								
	while((exi_chan1cr)&1);							
	i = exi_chan1data;									
	exi_chan1sr = 0x0; 
	if (i==0x04700000){
		return 1;									// Returns 1 if adapter is ready to receive
	}   
    return 0;										// Else Gecko Adapter is not plugged in
}
