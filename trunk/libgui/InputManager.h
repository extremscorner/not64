#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Input
{
public:
	void refreshInput();
#ifdef HW_RVL
	WPADData* getWpad();
#endif
	PADStatus* getPad();
	void clearInputData();
	static Input& getInstance()
	{
		static Input obj;
		return obj;
	}

private:
	Input();
	~Input();
	PADStatus gcPad[4];
#ifdef HW_RVL
	WPADData wiiPad[4];
	int wiiPadError[4];
#endif

};

} //namespace menu 

#endif
