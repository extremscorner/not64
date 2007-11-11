/* PakIO.h - Defines for Raw Data handling
   A gutted version of PakIO.h from N-Rage`s Dinput8 Plugin
    (C) 2002, 2006  Norbert Wladyka (Released under the GPLv2)
 */

#ifndef PAKIO_H
#define PAKIO_H

//************Raw Data***********//
   //byte 1 = number of bytes to send
   //byte 2 = number of bytes to recieve
   //byte 3 = Command Type

	// get status
#define RD_GETSTATUS		0x00
	// read button values
#define RD_READKEYS			0x01
	// read from controllerpak
#define RD_READPAK			0x02
	// write to controllerpack
#define RD_WRITEPAK			0x03
	// reset controller
#define RD_RESETCONTROLLER	0xff
	// read eeprom
#define RD_READEEPROM		0x04
	// write eeprom
#define RD_WRITEEPROM		0x05

	// Codes for retrieving status
    // 0x010300 - A1B2C3FF

	//A1
	// Default GamePad
#define RD_ABSOLUTE			0x01
#define RD_RELATIVE			0x02
	// Default GamePad
#define RD_GAMEPAD			0x04

	//B2
#define RD_EEPROM			0x80 
#define RD_NOEEPROM			0x00

	//C3
	// No Plugin in Controller
#define RD_NOPLUGIN			0x00
	// Plugin in Controller (Mempack, RumblePack etc)
#define RD_PLUGIN			0x01
	// Pak interface was uninitialized before the call
#define RD_NOTINITIALIZED	0x02
	// Address of last Pak I/O was invalid
#define RD_ADDRCRCERR		0x04
	// eeprom busy
#define RD_EEPROMBUSY		0x80

// The Error values are as follows:
// 0x01ER00 - ........

	//ER
	// no error, operation successful.
#define RD_OK				0x00
	// error, device not present for specified command.
#define RD_ERROR			0x80
	// error, unable to send/recieve the number bytes for command type.
#define RD_WRONGSIZE		0x40

	// the address where rumble-commands are sent to
	// this is really 0xC01B but our addressing code truncates the last several bits.
#define PAK_IO_RUMBLE		0xC000

#endif
