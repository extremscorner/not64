/**
 * Wii64 - GuiResources.h
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

#ifndef GUIRESOURCES_H
#define GUIRESOURCES_H

#include "GuiTypes.h"

namespace menu {

class Resources
{
public:
	Image* getImage(int image);
	static Resources& getInstance()
	{
		static Resources obj;
		return obj;
	}

	enum Images
	{
		IMAGE_DEFAULT_BUTTON=1,
		IMAGE_DEFAULT_BUTTONFOCUS,
		IMAGE_STYLEA_BUTTON,
		IMAGE_STYLEA_BUTTONFOCUS,
		IMAGE_STYLEA_BUTTONSELECTOFF,
		IMAGE_STYLEA_BUTTONSELECTOFFFOCUS,
		IMAGE_STYLEA_BUTTONSELECTON,
		IMAGE_STYLEA_BUTTONSELECTONFOCUS,
		IMAGE_MENU_BACKGROUND,
		IMAGE_LOGO,
		IMAGE_CONTROLLER_EMPTY,
		IMAGE_CONTROLLER_GAMECUBE,
		IMAGE_CONTROLLER_CLASSIC,
		IMAGE_CONTROLLER_WIIMOTENUNCHUCK
	};

private:
	Resources();
	~Resources();
	Image *defaultButtonImage, *defaultButtonFocusImage;
	Image *styleAButtonImage, *styleAButtonFocusImage;
	Image *styleAButtonSelectOffImage, *styleAButtonSelectOffFocusImage;
	Image *styleAButtonSelectOnImage, *styleAButtonSelectOnFocusImage;
	Image *menuBackgroundImage;
	Image *logoImage;
	Image *controllerEmptyImage, *controllerGamecubeImage;
	Image *controllerClassicImage, *controllerWiimoteNunchuckImage;

};

} //namespace menu 

#endif
