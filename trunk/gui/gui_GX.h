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
//   char GUItext [10][50];
} GUIinfo;


void GUI_setFB(unsigned int*, unsigned int*);
void GUI_init();
void GUI_drawScreen();
void GUI_displayText();
int GUI_loadBGtex();
void GUI_drawLogo();
   
#endif
