/**
 * Mupen64 - vector.h
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

#ifndef VEKTOR_H
#define VEKTOR_H

template<class T, int s> class Vektor
{
   T element[s];
   
 public:
   Vektor() {}
   ~Vektor() {}
   
   Vektor(const Vektor<T,s>& v)
     {
	for (int i=0; i<s; i++)
	  element[i] = v.element[i];
     }
   
   Vektor<T,s>& operator= (const Vektor<T,s>& v)
     {
	for (int i=0; i<s; i++)
	  element[i] = v.element[i];
	return *this;
     }
   
   T scalar(const Vektor<T,s>& v) const
     {
	T res = 0;
	for (int i=0; i<s; i++)
	  res += element[i] * v.element[i];
	return res;
     }
   
   void normalize()
     {
	T norm = 0;
	for(int i=0; i<s; i++) norm += element[i] * element[i];
	norm = sqrtf(norm);
	for(int i=0; i<s; i++) element[i] = element[i] / norm;
     }
   
   void setVektor(T* array)
     {
	for (int i=0; i<s; i++)
	  element[i] = array[i];
     }
   
   T& operator[] (int i)
     {
	return element[i];
     }
};

#endif
