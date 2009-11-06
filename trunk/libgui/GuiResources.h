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
		IMAGE_MENU_BACKGROUND
	};

private:
	Resources();
	~Resources();
	Image *defaultButtonImage, *defaultButtonFocusImage;
	Image *styleAButtonImage, *styleAButtonFocusImage;
	Image *styleAButtonSelectOffImage, *styleAButtonSelectOffFocusImage;
	Image *styleAButtonSelectOnImage, *styleAButtonSelectOnFocusImage;
	Image *menuBackgroundImage;

};

} //namespace menu 

#endif
