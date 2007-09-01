/**
 * Mupen64 - tf.cpp
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

#include <stdio.h>

#include "tf.h"

TF::TF()
{
}

TF::~TF()
{
}

void TF::setTextureFilter(int value)
{
   textureFilter = value;
}

void TF::setTextureConvert(int value)
{
   textureConvert = value;
}

int TF::getTextureConvert()
{
   return textureConvert;
}

Color32 TF::filter(Color32 nearestTexels[4], float nearestTexelsDistance[4])
{
   if (textureFilter == 0)
     {
	float min = nearestTexelsDistance[0];
	int minIndex = 0;
	for (int i=0; i<4; i++)
	  {
	     if (nearestTexelsDistance[i] < min)
	       {
		  min = nearestTexelsDistance[i];
		  minIndex = i;
	       }
	  }
	return nearestTexels[minIndex];
     }
   else if (textureFilter == 2)
     {
	float total = 0;
	float max = nearestTexelsDistance[0];
	int maxIndex = 0;
	for (int i=0; i<4; i++)
	  {
	     total += nearestTexelsDistance[i];
	     if (nearestTexelsDistance[i] > max)
	       {
		  max = nearestTexelsDistance[i];
		  maxIndex = i;
	       }
	  }
	total -= max;
	
	
	Color32 filtered(0, 0, 0, 0);
	for (int i=0; i<4; i++)
	  {
	     if (i != maxIndex)
	       {
		  float coef = (1-(nearestTexelsDistance[i]/total))/2;
		  filtered += nearestTexels[i]*coef;
		  filtered.setAlpha(filtered.getAlpha() + coef*nearestTexels[i].getAlpha());
	       }
	  }
	
	
	return filtered;
     }
   else
     printf("TF:textureFilter=%x\n", textureFilter);
   return 0;
}
