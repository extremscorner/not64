/* controller-WiimoteNunchuk.c - Wiimote + Nunchuk input module
   by Mike Slegeir for Mupen64-GC
 */

#include <string.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"

#ifndef PI
#define PI 3.14159f
#endif

enum { STICK_X, STICK_Y };
static int getStickValue(joystick_t* j, int axis, int maxAbsValue){
	double angle = PI * j->ang/180.0f;
	double magnitude = (j->mag > 1.0f) ? 1.0f :
	                    (j->mag < -1.0f) ? -1.0f : j->mag;
	double value;
	if(axis == STICK_X)
		value = magnitude * sin( angle );
	else
		value = magnitude * cos( angle );
	return (int)(value * maxAbsValue);
}

static int _GetKeys(int Control, BUTTONS * Keys )
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected nunchuck controller
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_NUNCHUK){
		controller_WiimoteNunchuk.available[Control] = 1;
	} else {
		controller_WiimoteNunchuk.available[Control] = 0;
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_CLASSIC){
			controller_Classic.available[Control] = 1;
		}
		return 0;
	}

	int b  = wpad->btns_h;
	int d2 = b & WPAD_BUTTON_2;
	c->R_DPAD       = (b & WPAD_BUTTON_RIGHT && d2)  ? 1 : 0;
	c->L_DPAD       = (b & WPAD_BUTTON_LEFT  && d2)  ? 1 : 0;
	c->D_DPAD       = (b & WPAD_BUTTON_DOWN  && d2)  ? 1 : 0;
	c->U_DPAD       = (b & WPAD_BUTTON_UP    && d2)  ? 1 : 0;
	c->START_BUTTON = (b & WPAD_BUTTON_HOME)         ? 1 : 0;
	c->B_BUTTON     = (b & (WPAD_BUTTON_MINUS | WPAD_BUTTON_PLUS)) ? 1 : 0;
	c->A_BUTTON     = (b & WPAD_BUTTON_A)            ? 1 : 0;

	c->Z_TRIG       = (b & WPAD_NUNCHUK_BUTTON_Z)    ? 1 : 0;
	c->R_TRIG       = (b & WPAD_BUTTON_B)            ? 1 : 0;
	c->L_TRIG       = (b & WPAD_NUNCHUK_BUTTON_C)    ? 1 : 0;

	c->R_CBUTTON    = (b & WPAD_BUTTON_RIGHT)        ? 1 : 0;
	c->L_CBUTTON    = (b & WPAD_BUTTON_LEFT)         ? 1 : 0;
	c->D_CBUTTON    = (b & WPAD_BUTTON_DOWN)         ? 1 : 0;
	c->U_CBUTTON    = (b & WPAD_BUTTON_UP)           ? 1 : 0;

	c->X_AXIS       = getStickValue(&wpad->exp.nunchuk.js, STICK_X, 127);
	c->Y_AXIS       = getStickValue(&wpad->exp.nunchuk.js, STICK_Y, 127);

	// 1+2 quits to menu
	return (b & WPAD_BUTTON_1) && (b & WPAD_BUTTON_2);
}

static void pause(int Control){ }

static void resume(int Control){ }

static void rumble(int Control, int rumble){ }

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void init(void);

controller_t controller_WiimoteNunchuk =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0}
	 };

static void init(void){
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		WPADData* wpad = WPAD_Data(i);
		// Only use a connected nunchuk
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_WiimoteNunchuk.available[i] = 0;
	}
}
