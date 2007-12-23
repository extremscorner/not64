/* gui_GX.h - gui that uses GX graphics
   by sepp256 for Mupen64-GC
 */

#ifndef GUI_GX_H
#define GUI_GX_H

#include "GUI.h"
#include <gccore.h>

typedef struct
{
   unsigned int* xfb[2];
   int which_fb;
   int width;
   int height;
} GUIinfo;


void GUI_setFB(unsigned int*, unsigned int*);
void GUI_init();
void GUI_toggle();
void GUI_main();
void GUI_displayText();
int GUI_loadBGtex();
void GUI_drawWiiN64(float, float, float, float);
void GUI_drawLogo(float, float, float);
   
#endif
