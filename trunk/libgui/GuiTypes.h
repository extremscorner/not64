#ifndef GUIDEFS_H
#define GUIDEFS_H

#include <gccore.h>
#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif
#include <vector>
#include <stack>
#include <string>
#include <malloc.h>

namespace menu {

class Graphics;
class Component;
class Frame;
class Button;
class Input;
class Cursor;
class Focus;
class Image;
class IplFont;

typedef	std::vector<Frame*> FrameList;
typedef std::vector<Component*> ComponentList;
typedef std::stack<float> FloatStack;
//typedef std::stack<Mtx> MatrixStack;

} //namespace menu 

#endif
