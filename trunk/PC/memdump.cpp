//**********************************************************************************//
//								USB Gecko DVD simulator								//
//**********************************************************************************//

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "stdafx.h"
#include "ftd2xx.h"

/*** Function Prototypes ***/
void gecko_opendevice();
int gecko_readbytes(LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned);
int gecko_writebytes(LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten);
void send_dvd_data(unsigned int offset,unsigned int size);
unsigned int convert_int(unsigned int in);
void send_dol(char *name);

/*** Global Variables ***/
/* FTDI Variables */
FT_HANDLE fthandle;	// Handle of the device to be opened and used for all functions
FT_STATUS status;	// Variable needed for FTDI Library functions
DWORD TxSent;
DWORD RxSent;
FILE *fp;
FILE *dol;
#define USB_PACKET_SIZE 0x400
int returnvalue;
char buffer[65536];

unsigned int convert_int(unsigned int in)
{
  unsigned int out;
  char *p_in = (char *) &in;
  char *p_out = (char *) &out;
  p_out[0] = p_in[3];
  p_out[1] = p_in[2];
  p_out[2] = p_in[1];
  p_out[3] = p_in[0];  
  return out;
}

int main (int argc, char *argv[])
{
	unsigned int size = 0;

	printf("USBBoot Mupen64GC Tools v0.1\n\n");
    printf("Connecting to USB Gecko...\n");

	gecko_opendevice();

	printf("Connected to Gamecube / Wii Console\n\n");	//Connected & Ready
	
	while(1) {
		//clear packet and size of last one
		size = 0;
		memset(&buffer,0,USB_PACKET_SIZE);

		//grab size
		gecko_readbytes(&size, 4, &RxSent);
		size = convert_int(size);
		//printf("%i bytes incoming!\n",size);

		//grab packet according to size
		gecko_readbytes(&buffer, size, &RxSent);
		printf("%s",buffer);
	}		
	status = FT_Close(fthandle);
	return (0);	// Display and return

}

//////////////////////////////////////////////////////////////////////////////////////
//							 Gecko Open Device										//
//			Opens the USB based on the serial number and sets up the chip			//			
//////////////////////////////////////////////////////////////////////////////////////
void gecko_opendevice()
{
		// Open by Serial Number
		status = FT_OpenEx("GECKUSB0", FT_OPEN_BY_SERIAL_NUMBER, &fthandle);
			if(status != FT_OK){
				printf("Error: Couldn't connect to USB Gecko. Please check Installation\n");
				exit(0);
				}
		// Reset the Device
		status = FT_ResetDevice(fthandle);
			if(status != FT_OK){
				printf("Error: Couldnt Reset Device %d\n",status);
				status = FT_Close(fthandle);
				exit(0);
				}
		// Set a infinite timeout
		status = FT_SetTimeouts(fthandle,0,0);
			if(status != FT_OK){
				printf("Error: Timeouts failed to set %d\n",status);
				status = FT_Close(fthandle);
				exit(0);
				}	
		// Purge RX buffer
		status = FT_Purge(fthandle,FT_PURGE_RX);
			if(status != FT_OK){
				printf("Error: Problem clearing buffers %d\n",status);
				status = FT_Close(fthandle);
				exit(0);
				}
		// Purge TX buffer
		status = FT_Purge(fthandle,FT_PURGE_TX);
			if(status != FT_OK){
				printf("Error: Problem clearing buffers %d\n",status);
				status = FT_Close(fthandle);
				exit(0);
				}
		// Set packet size in bytes - 65536 packet is maximum packet size (USB 2.0)
		status = FT_SetUSBParameters(fthandle,65536,0);
			if(status != FT_OK){
				printf("Error: Couldnt Set USB Parameters %d\n",status);
				status = FT_Close(fthandle);
				exit(0);
				}
		Sleep(150);		// Allow some time
}



//////////////////////////////////////////////////////////////////////////////////////
//								Gecko Read Byte										//
//////////////////////////////////////////////////////////////////////////////////////
int gecko_readbytes(LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned)
{
	// read data based on FTDI D2XX USB 2.0 API
	status = FT_Read(fthandle, lpBuffer, dwBytesToRead, lpdwBytesReturned);
	
	if (status == FT_OK)								// Check read ok
	{
		if(*lpdwBytesReturned != dwBytesToRead){		// Check to see if we have received our data
			return 2;									// Can be used for packet retry code ect
		}
	}
	else
	{
		printf("Error: Read Error. Closing\n");
		status = FT_Close(fthandle);					// Close device if fatal error
		fclose(fp);
		exit(0);
	}
	return 1;											// Packet Sent
		
}

//////////////////////////////////////////////////////////////////////////////////////
//								Gecko Write Byte									//
//				This function will write a packet of bytes to the console			//
//	 Returns 2 if packet is not returned correctly, Returns 0 if error, 1 byte ok	//
//////////////////////////////////////////////////////////////////////////////////////
int gecko_writebytes(LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten)
{
	// write data based on FTDI D2XX USB 2.0 API
	status = FT_Write(fthandle, lpBuffer, dwBytesToWrite, lpdwBytesWritten);
	
	if (status == FT_OK)							// Check write ok
	{
		if(*lpdwBytesWritten != dwBytesToWrite){	// Check to see if we have sent our data
			return 2;								// Can be used for packet retry code ect
		}
	}
	else {
		printf("Error: Write Error. Closing.\n");
		status = FT_Close(fthandle);				// Close device if fatal error
		fclose(fp);
		exit(0);
	}
	return 1;										// Packet Sent
}



