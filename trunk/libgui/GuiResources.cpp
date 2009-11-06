#include "GuiResources.h"
#include "Image.h"
#include "resources.h"
#include "../menu/MenuResources.h"

namespace menu {

Resources::Resources()
{
	defaultButtonImage = new Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	defaultButtonFocusImage = new Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
//	styleAButtonImage = new Image(StyleAButtonTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
//	styleAButtonFocusImage = new Image(StyleAButtonFocusTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
//	styleAButtonSelectOffImage = new Image(StyleAButtonSelectOffTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
//	styleAButtonSelectOffFocusImage = new Image(StyleAButtonSelectOffFocusTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
//	styleAButtonSelectOnImage = new Image(StyleAButtonSelectOnTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
//	styleAButtonSelectOnFocusImage = new Image(StyleAButtonSelectOnFocusTexture, 8, 56, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, StyleAButtonTlut, GX_TL_RGB5A3, GX_TLUT0, GX_TLUT_256);
	styleAButtonImage = new Image(StyleAButtonTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonFocusImage = new Image(StyleAButtonFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOffImage = new Image(StyleAButtonSelectOffTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOffFocusImage = new Image(StyleAButtonSelectOffFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOnImage = new Image(StyleAButtonSelectOnTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOnFocusImage = new Image(StyleAButtonSelectOnFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	menuBackgroundImage = new Image(BackgroundTexture, 848, 480, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);

}

Resources::~Resources()
{
	delete defaultButtonImage;
	delete defaultButtonFocusImage;
	delete styleAButtonImage;
	delete styleAButtonFocusImage;
	delete styleAButtonSelectOffImage;
	delete styleAButtonSelectOffFocusImage;
	delete styleAButtonSelectOnImage;
	delete styleAButtonSelectOnFocusImage;
	delete menuBackgroundImage;
}

Image* Resources::getImage(int image)
{
	Image* returnImage = NULL;
	switch (image)
	{
	case IMAGE_DEFAULT_BUTTON:
		returnImage = defaultButtonImage;
		break;
	case IMAGE_DEFAULT_BUTTONFOCUS:
		returnImage = defaultButtonFocusImage;
		break;
	case IMAGE_STYLEA_BUTTON:
		returnImage = styleAButtonImage;
		break;
	case IMAGE_STYLEA_BUTTONFOCUS:
		returnImage = styleAButtonFocusImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTOFF:
		returnImage = styleAButtonSelectOffImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTOFFFOCUS:
		returnImage = styleAButtonSelectOffFocusImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTON:
		returnImage = styleAButtonSelectOnImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTONFOCUS:
		returnImage = styleAButtonSelectOnFocusImage;
		break;
	case IMAGE_MENU_BACKGROUND:
		returnImage = menuBackgroundImage;
		break;
	}
	return returnImage;
}

} //namespace menu 
