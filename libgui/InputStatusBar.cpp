/**
 * Wii64 - InputStatusBar.cpp
 * Copyright (C) 2009 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include "InputStatusBar.h"
#include "GuiResources.h"
#include "GraphicsGX.h"
#include "IPLFont.h"
#include "Image.h"
#include "FocusManager.h"
#include <math.h>
#include <gccore.h>
#include "../main/wii64config.h"

extern "C" {
#include "../gc_input/controller.h"
#include "../main/rom.h"
}

namespace menu {

InputStatusBar::InputStatusBar(float x, float y)
		: x(x),
		  y(y)
/*		: active(false),
		  selected(false),
		  normalImage(0),
		  focusImage(0),
		  selectedImage(0),
		  selectedFocusImage(0),
		  buttonText(label),
		  buttonStyle(style),
		  labelMode(LABEL_CENTER),
		  labelScissor(0),
		  StartTime(0),
		  x(x),
		  y(y),
		  width(width),
		  height(height),
		  clickedFunc(0),
		  returnFunc(0)*/
{
						//Focus color			Inactive color		  Active color			Selected color		  Label color
//	GXColor colors[5] = {{255, 100, 100, 255}, {255, 255, 255,  70}, {255, 255, 255, 130}, {255, 255, 255, 255}, {255, 255, 255, 255}};

}

InputStatusBar::~InputStatusBar()
{
}

extern "C" BOOL hasLoadedROM;
extern "C" char autoSave;
extern "C" BOOL sramWritten;
extern "C" BOOL eepromWritten;
extern "C" BOOL mempakWritten;
extern "C" BOOL flashramWritten;

void InputStatusBar::drawComponent(Graphics& gfx)
{
	int box_x = 50;
	int box_y = 0;
	int width = 235;
	int height = 340;
	int labelScissor = 5;
	GXColor activeColor = (GXColor) {255, 255, 255, 255};
	GXColor inactiveColor = (GXColor) {192, 192, 192, 192};
	GXColor controllerColors[5] = {	{  1,  29, 169, 255}, //blue
									{229,  29,  19, 255}, //orange/red
									{  8, 147,  48, 255}, //green
									{255, 192,   1, 255}, //yellow/gold
									{150, 150, 255, 255}};
//	char statusText[50];
	Image* statusIcon = NULL;
	//Draw Status Info Box
	GXColor boxColor = (GXColor) {87, 90, 100,128};
	gfx.setTEV(GX_PASSCLR);
	gfx.enableBlending(true);
	gfx.setColor(boxColor);
	gfx.fillRect(box_x, box_y, width, height);
	//Write ROM Status Info
	gfx.enableScissor(box_x + labelScissor, box_y, width - 2*labelScissor, height);
	char buffer [51];
	int text_y = 100;
	if(!hasLoadedROM)
	{
		IplFont::getInstance().drawInit(activeColor);
		sprintf(buffer,"No ROM Loaded");
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.8, false);
	}
	else
	{
		IplFont::getInstance().drawInit(activeColor);
		sprintf(buffer,"%s",ROM_SETTINGS.goodname);
//		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.8, false);
		text_y += 20*IplFont::getInstance().drawStringWrap((int) box_x + 15, (int) text_y, buffer, 0.8, false, width - 2*15, 20);
	    countrycodestring(ROM_HEADER->Country_code&0xFF, buffer);
		text_y += 13;
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.7, false);
		if (autoSave)
			sprintf(buffer,"AutoSave Enabled");
		else if (!flashramWritten && !sramWritten && !eepromWritten && !mempakWritten)
			sprintf(buffer,"Nothing to Save");
		else
			sprintf(buffer,"Game Needs Saving");
		text_y += 25;
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.7, false);
	}
	gfx.disableScissor();
	//Update controller availability
	for (int i = 0; i < 4; i++)
	{
		switch (padType[i])
		{
		case PADTYPE_GAMECUBE:
			controller_GC.available[(int)padAssign[i]] = (gc_connected & (1<<padAssign[i])) ? 1 : 0;
			if (controller_GC.available[(int)padAssign[i]])
			{
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
			}
			else
			{
				gfx.setColor(inactiveColor);
				IplFont::getInstance().drawInit(inactiveColor);
			}
			statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_GAMECUBE);
//			sprintf (statusText, "Pad%d: GC%d", i+1, padAssign[i]+1);
			break;
#ifdef HW_RVL
		case PADTYPE_WII:
			u32 type;
			s32 err;
			err = WPAD_Probe((int)padAssign[i], &type);
			controller_Classic.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_CLASSIC) ? 1 : 0;
			controller_WiimoteNunchuk.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_NUNCHUK) ? 1 : 0;
			controller_Wiimote.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_NONE) ? 1 : 0;
			if (controller_Classic.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_Classic, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_CLASSIC);
//				sprintf (statusText, "Pad%d: CC%d", i+1, padAssign[i]+1);
			}
			else if (controller_WiimoteNunchuk.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_WiimoteNunchuk, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTENUNCHUCK);
//				sprintf (statusText, "Pad%d: WM+N%d", i+1, padAssign[i]+1);
			}
			else if (controller_Wiimote.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_Wiimote, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTE);
//				sprintf (statusText, "Pad%d: WM%d", i+1, padAssign[i]+1);
			}
			else
			{
				gfx.setColor(inactiveColor);
				IplFont::getInstance().drawInit(inactiveColor);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTE);
//				sprintf (statusText, "Pad%d: Wii%d", i+1, padAssign[i]+1);
			}

			break;
#endif
		case PADTYPE_NONE:
			gfx.setColor(inactiveColor);
			IplFont::getInstance().drawInit(inactiveColor);
			statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_EMPTY);
//			sprintf (statusText, "Pad%d: None", i+1);
			break;
		}
//		IplFont::getInstance().drawString((int) 540, (int) 150+30*i, statusText, 1.0, true);
//		IplFont::getInstance().drawString((int) box_x+width/2, (int) 215+30*i, statusText, 1.0, true);
		int base_x = box_x + 14 + 53*i;
		int base_y = 260;
		//draw numbers
		sprintf(buffer,"%d",i+1);
		IplFont::getInstance().drawString((int) base_x+36, (int) base_y+10, buffer, 0.8, true);
		if (padType[i]!=PADTYPE_NONE)
		{
			sprintf(buffer,"%d",padAssign[i]+1);
			IplFont::getInstance().drawString((int) base_x+37, (int) base_y+52, buffer, 0.8, true);
		}
		//draw icon
		statusIcon->activateImage(GX_TEXMAP0);
		GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
		GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_RASA,GX_CA_TEXA,GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		gfx.enableBlending(true);
		gfx.drawImage(0, base_x, base_y, 48, 64, 0, 1, 0, 1);
	}

	//draw logo
	Resources::getInstance().getImage(Resources::IMAGE_LOGO)->activateImage(GX_TEXMAP0);
	gfx.setTEV(GX_REPLACE);
	gfx.enableBlending(true);
#ifdef HW_RVL
	gfx.drawImage(0, 75, 380, 144, 52, 0, 1, 0, 1);
#else
	gfx.drawImage(0, 75, 380, 192, 52, 0, 1, 0, 1);
#endif
	gfx.setTEV(GX_PASSCLR);

/*
//	printf("Button drawComponent\n");

	gfx.setColor(inactiveColor);

	//activate relevant texture
	if(active)
	{
		//draw normalImage with/without gray mask and with alpha test on
		//printf("Button Active\n");
		gfx.setColor(activeColor);
	}
	if(getFocus())
	{
		//draw focus indicator (extra border for button?)
		//printf("Button in Focus\n");
		gfx.setColor(focusColor);
	}
	//draw buttonLabel?

	gfx.enableBlending(true);
//	gfx.setTEV(GX_PASSCLR);
	gfx.setTEV(GX_MODULATE);

//	gfx.setColor(focusColor);
	gfx.setDepth(-10.0f);
	gfx.newModelView();
	gfx.loadModelView();
	gfx.loadOrthographic();

	switch (buttonStyle)
	{
	case BUTTON_DEFAULT:
//	gfx.fillRect(x, y, width, height);
		normalImage->activateImage(GX_TEXMAP0);
		gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
		gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
		gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
		gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);
//	gfx.drawImage(0, x, y, width, height, 0.0, 1.0, 0.0, 1.0);

		if (selected)
		{
			gfx.setColor(selectedColor);
			if(selectedImage) selectedImage->activateImage(GX_TEXMAP0);
			gfx.drawImage(0, x, y, width/2, height/2, 0.0, width/16.0, 0.0, height/16.0);
			gfx.drawImage(0, x+width/2, y, width/2, height/2, width/16.0, 0.0, 0.0, height/16.0);
			gfx.drawImage(0, x, y+height/2, width/2, height/2, 0.0, width/16.0, height/16.0, 0.0);
			gfx.drawImage(0, x+width/2, y+height/2, width/2, height/2, width/16.0, 0.0, height/16.0, 0.0);
		}
		break;
	case BUTTON_STYLEA_NORMAL:
		if (getFocus())	focusImage->activateImage(GX_TEXMAP0);
		else			normalImage->activateImage(GX_TEXMAP0);
		gfx.drawImage(0, x, y, width/2, height, 0.0, width/8.0, 0.0, 1.0);
		gfx.drawImage(0, x+width/2, y, width/2, height, width/8.0, 0.0, 0.0, 1.0);
		break;
	case BUTTON_STYLEA_SELECT:
		if (selected)
		{
			if (getFocus())	selectedFocusImage->activateImage(GX_TEXMAP0);
			else			selectedImage->activateImage(GX_TEXMAP0);
		}
		else
		{
			if (getFocus())	focusImage->activateImage(GX_TEXMAP0);
			else			normalImage->activateImage(GX_TEXMAP0);
		}
		gfx.drawImage(0, x, y, width/2, height, 0.0, width/8.0, 0.0, 1.0);
		gfx.drawImage(0, x+width/2, y, width/2, height, width/8.0, 0.0, 0.0, 1.0);
		break;
	}

	if (buttonText)
	{
		int strWidth, strHeight;
		unsigned long CurrentTime;
		float scrollWidth, time_sec, scrollOffset;
		gfx.enableScissor(x + labelScissor, y, width - 2*labelScissor, height);
		if(active)	IplFont::getInstance().drawInit(labelColor);
		else		IplFont::getInstance().drawInit(inactiveColor);
		switch (labelMode)
		{
			case LABEL_CENTER:
				IplFont::getInstance().drawString((int) (x+width/2), (int) (y+height/2), *buttonText, 1.0, true);
				break;
			case LABEL_LEFT:
				strWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0);
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				IplFont::getInstance().drawString((int) (x+labelScissor), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				break;
			case LABEL_SCROLL:
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				scrollWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0)-width+2*labelScissor;
				scrollWidth = scrollWidth < 0.0f ? 0.0 : scrollWidth;
				CurrentTime = ticks_to_microsecs(gettick());
				time_sec = (float)(CurrentTime - StartTime)/1000000.0f;
				if (time_sec > SCROLL_PERIOD) StartTime = ticks_to_microsecs(gettick());
				scrollOffset = fabsf(fmodf(time_sec,SCROLL_PERIOD)-SCROLL_PERIOD/2)/(SCROLL_PERIOD/2);
				IplFont::getInstance().drawString((int) (x+labelScissor-(int)(scrollOffset*scrollWidth)), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				break;
			case LABEL_SCROLLONFOCUS:
				if(getFocus())
				{
					strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
					scrollWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0)-width+2*labelScissor;
					scrollWidth = scrollWidth < 0.0f ? 0.0 : scrollWidth;
					CurrentTime = ticks_to_microsecs(gettick());
					time_sec = (float)(CurrentTime - StartTime)/1000000.0f;
					if (time_sec > SCROLL_PERIOD) StartTime = ticks_to_microsecs(gettick());
					scrollOffset = fabsf(fmodf(time_sec,SCROLL_PERIOD)-SCROLL_PERIOD/2)/(SCROLL_PERIOD/2);
					IplFont::getInstance().drawString((int) (x+labelScissor-(int)(scrollOffset*scrollWidth)), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				}
				else
				{
				strWidth = IplFont::getInstance().getStringWidth(*buttonText, 1.0);
				strHeight = IplFont::getInstance().getStringHeight(*buttonText, 1.0);
				IplFont::getInstance().drawString((int) (x+labelScissor), (int) (y+(height-strHeight)/2), *buttonText, 1.0, false);
				}
				break;
		}
		gfx.disableScissor();
	}
*/
}

void InputStatusBar::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

Component* InputStatusBar::updateFocus(int direction, int buttonsPressed)
{
	return NULL;
}

} //namespace menu 
