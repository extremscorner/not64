/**
 * Mupen64 - color.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
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
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifndef COLOR_H
#define COLOR_H

class Color16
{
   float r;
   float g;
   float b;
   float a;
   
 public:
   Color16() {}
   ~Color16() {}
   
   Color16(const Color16 &c)
     {
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
     }
   
   Color16 operator*(float n)
     {
	Color16 temp;
	temp.r = n*r;
	temp.g = n*g;
	temp.b = n*b;
	return temp;
     }
   
   Color16 operator+(const Color16& c)
     {
	Color16 temp;
	temp.r = r+c.r;
	temp.g = g+c.g;
	temp.b = b+c.b;
	return temp;
     }
   
   Color16& operator=(int c)
     {
	r = (c >> 11) & 0x1F;
	g = (c >> 6) & 0x1F;
	b = (c >> 1) & 0x1F;
	a = c&1;
	return *this;
     }
   
   Color16& operator=(short c)
     {
	r = (c >> 11) & 0x1F;
	g = (c >> 6) & 0x1F;
	b = (c >> 1) & 0x1F;
	a = c&1;
	return *this;
     }
   
   operator int()
     {
	return ((int)r << 11) | ((int)g << 6) | ((int)b << 1);
     }
};

class Color32
{
   float r;
   float g;
   float b;
   float a;
   
 public:
   Color32() {}
   ~Color32() {}
   
   Color32(const Color32 &c)
     {
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
     }
   
   Color32(int c)
     {
	r = (c >> 24) & 0xFF;
	g = (c >> 16) & 0xFF;
	b = (c >> 8) & 0xFF;
	a = c&0xFF;
     }
   
   Color32(float _r, float _g, float _b, float _a)
     {
	r = _r;
	g = _g;
	b = _b;
	a = _a;
     }
   
   Color32& operator=(const Color32 &c)
     {
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
	return *this;
     }
   
   Color32& operator=(int c)
     {
	r = (c >> 24) & 0xFF;
	g = (c >> 16) & 0xFF;
	b = (c >> 8) & 0xFF;
	a = c&0xFF;
	return *this;
     }
   
   Color32 operator-(const Color32& c)
     {
	Color32 res;
	res.r = r - c.r;
	res.g = g - c.g;
	res.b = b - c.b;
	res.a = a;
	return res;
     }
   
   Color32 operator+=(const Color32& c)
     {
	*this = *this + c;
	return *this;
     }
   
   Color32 operator+(const Color32& c)
     {
	Color32 res;
	res.r = r + c.r;
	res.g = g + c.g;
	res.b = b + c.b;
	res.a = a;
	return res;
     }
   
   Color32 operator*(const Color32& c)
     {
	Color32 res;
	res.r = c.r * r / 255.0f;
	res.g = c.g * g / 255.0f;
	res.b = c.b * b / 255.0f;
	res.a = a;
	return res;
     }
   
   Color32 operator*(int n)
     {
	float c;
	Color32 res;
	c = n == 0xFF ? 0x100 : n;
	res.r = c * r / 255.0f;
	res.g = c * g / 255.0f;
	res.b = c * b / 255.0f;
	res.a = a;
	return res;
     }
   
   Color32 operator*(float f)
     {
	Color32 res;
	res.r = f * r;
	res.g = f * g;
	res.b = f * b;
	res.a = a;
	return res;
     }
   
   Color32 operator/(const Color32& c)
     {
	Color32 res;
	res.r = r / (c.r / 255.0f);
	res.g = g / (c.g / 255.0f);
	res.b = b / (c.b / 255.0f);
	res.a = a;
	return res;
     }
   
   Color32 operator/(int n)
     {
	float c;
	Color32 res;
	c = n == 0xFF ? 0x100 : n;
	res.r = r / (c / 255.0f);
	res.g = g / (c / 255.0f);
	res.b = b / (c / 255.0f);
	res.a = a;
	return res;
     }
   
   Color32 operator/(float f)
     {
	Color32 res;
	res.r = r / f;
	res.g = g / f;
	res.b = b / f;
	res.a = a;
	return res;
     }
   
   void clamp()
     {
	if (r > 255.0f) r = 255.0f;
	if (g > 255.0f) g = 255.0f;
	if (b > 255.0f) b = 255.0f;
	if (a > 255.0f) a = 255.0f;
     }
   
   float *getAlphap()
     {
	return &a;
     }
   
   float getR()
     {
	return r;
     }
   
   void setR(float _r)
     {
	r = _r;
     }
   
   float getG()
     {
	return g;
     }
   
   void setG(float _g)
     {
	g = _g;
     }
   
   float getB()
     {
	return b;
     }
   
   void setB(float _b)
     {
	b = _b;
     }
   
   float getAlpha()
     {
	return a;
     }
   
   void setAlpha(float alpha)
     {
	a = alpha;
     }
   
   void display()
     {
	printf("color:%d, %d, %d, %d\n", (int)r, (int)g, (int)b, (int)a);
     }
   
   operator int()
     {
	return ((int)r << 24) | ((int)g << 16) | ((int)b << 8) | (int)a;
     }
};

#endif
