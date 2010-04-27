/**
 * Wii64 - Logo.cpp
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

#include "Logo.h"
#include "GraphicsGX.h"
#include <math.h>
#include "ogc/lwp_watchdog.h"

namespace menu {

#define LOGO_N_X1 30
#define LOGO_N_X2 13
#define LOGO_N_Y1 30
#define LOGO_N_Y2  5
#define LOGO_N_Z1 30
#define LOGO_N_Z2 13

// 'N' logo vertex data
s8 N_verts[] ATTRIBUTE_ALIGN (32) =
{ // x y z
  -LOGO_N_X1, -LOGO_N_Y1,  LOGO_N_Z1,		// 0 (side A, XY plane, Z=LOGO_N_Z1)
  -LOGO_N_X2, -LOGO_N_Y1,  LOGO_N_Z1,		// 1		6 7   8 9
   LOGO_N_X2, -LOGO_N_Y1,  LOGO_N_Z1,		// 2		  5    
   LOGO_N_X1, -LOGO_N_Y1,  LOGO_N_Z1,		// 3		      4  
   LOGO_N_X2, -LOGO_N_Y2,  LOGO_N_Z1,		// 4		0 1   2 3
  -LOGO_N_X2,  LOGO_N_Y2,  LOGO_N_Z1,		// 5
  -LOGO_N_X1,  LOGO_N_Y1,  LOGO_N_Z1,		// 6
  -LOGO_N_X2,  LOGO_N_Y1,  LOGO_N_Z1,		// 7
   LOGO_N_X2,  LOGO_N_Y1,  LOGO_N_Z1,		// 8
   LOGO_N_X1,  LOGO_N_Y1,  LOGO_N_Z1,		// 9
   LOGO_N_Z1, -LOGO_N_Y1,  LOGO_N_X1,		// 10 (side B, -ZY plane, X=LOGO_N_Z1)
   LOGO_N_Z1, -LOGO_N_Y1,  LOGO_N_X2,		// 11		6 7   8 9
   LOGO_N_Z1, -LOGO_N_Y1, -LOGO_N_X2,		// 12		  5    
   LOGO_N_Z1, -LOGO_N_Y1, -LOGO_N_X1,		// 13		      4  
   LOGO_N_Z1, -LOGO_N_Y2, -LOGO_N_X2,		// 14		0 1   2 3
   LOGO_N_Z1,  LOGO_N_Y2,  LOGO_N_X2,		// 15
   LOGO_N_Z1,  LOGO_N_Y1,  LOGO_N_X1,		// 16
   LOGO_N_Z1,  LOGO_N_Y1,  LOGO_N_X2,		// 17
   LOGO_N_Z1,  LOGO_N_Y1, -LOGO_N_X2,		// 18
   LOGO_N_Z1,  LOGO_N_Y1, -LOGO_N_X1,		// 19
   LOGO_N_X1, -LOGO_N_Y1, -LOGO_N_Z1,		// 20 (side C, -XY plane, Z=-LOGO_N_Z1)
   LOGO_N_X2, -LOGO_N_Y1, -LOGO_N_Z1,		// 21		6 7   8 9
  -LOGO_N_X2, -LOGO_N_Y1, -LOGO_N_Z1,		// 22		  5    
  -LOGO_N_X1, -LOGO_N_Y1, -LOGO_N_Z1,		// 23		      4  
  -LOGO_N_X2, -LOGO_N_Y2, -LOGO_N_Z1,		// 24		0 1   2 3
   LOGO_N_X2,  LOGO_N_Y2, -LOGO_N_Z1,		// 25
   LOGO_N_X1,  LOGO_N_Y1, -LOGO_N_Z1,		// 26
   LOGO_N_X2,  LOGO_N_Y1, -LOGO_N_Z1,		// 27
  -LOGO_N_X2,  LOGO_N_Y1, -LOGO_N_Z1,		// 28
  -LOGO_N_X1,  LOGO_N_Y1, -LOGO_N_Z1,		// 29
  -LOGO_N_Z1, -LOGO_N_Y1, -LOGO_N_X1,		// 30 (side D, ZY plane, X=-LOGO_N_Z1)
  -LOGO_N_Z1, -LOGO_N_Y1, -LOGO_N_X2,		// 31		6 7   8 9
  -LOGO_N_Z1, -LOGO_N_Y1,  LOGO_N_X2,		// 32		  5    
  -LOGO_N_Z1, -LOGO_N_Y1,  LOGO_N_X1,		// 33		      4  
  -LOGO_N_Z1, -LOGO_N_Y2,  LOGO_N_X2,		// 34		0 1   2 3
  -LOGO_N_Z1,  LOGO_N_Y2, -LOGO_N_X2,		// 35
  -LOGO_N_Z1,  LOGO_N_Y1, -LOGO_N_X1,		// 36
  -LOGO_N_Z1,  LOGO_N_Y1, -LOGO_N_X2,		// 37
  -LOGO_N_Z1,  LOGO_N_Y1,  LOGO_N_X2,		// 38
  -LOGO_N_Z1,  LOGO_N_Y1,  LOGO_N_X1,		// 39
  -LOGO_N_X2,  LOGO_N_Y1,  LOGO_N_Z2,		// 40,7A (top, XZ plane, Y=LOGO_N_Y1)
   LOGO_N_X2,  LOGO_N_Y1,  LOGO_N_Z2,		// 41,7B		
   LOGO_N_X2,  LOGO_N_Y1, -LOGO_N_Z2,		// 42,7C		  7D 7C
  -LOGO_N_X2,  LOGO_N_Y1, -LOGO_N_Z2,		// 43,7D		  7A 7B  
  -LOGO_N_X2,  LOGO_N_Y2,  LOGO_N_Z2,		// 44,5A (upper-middle, XZ plane, Y=LOGO_N_Y2)
   LOGO_N_X2,  LOGO_N_Y2,  LOGO_N_Z2,		// 45,5B		
   LOGO_N_X2,  LOGO_N_Y2, -LOGO_N_Z2,		// 46,5C		  5D 5C
  -LOGO_N_X2,  LOGO_N_Y2, -LOGO_N_Z2,		// 47,5D		  5A 5B  
  -LOGO_N_X2, -LOGO_N_Y2,  LOGO_N_Z2,		// 48,4D (lower-middle, XZ plane, Y=-LOGO_N_Y2)
   LOGO_N_X2, -LOGO_N_Y2,  LOGO_N_Z2,		// 49,4A		
   LOGO_N_X2, -LOGO_N_Y2, -LOGO_N_Z2,		// 50,4B		  4C 4B
  -LOGO_N_X2, -LOGO_N_Y2, -LOGO_N_Z2,		// 51,4C		  4D 4A  
  -LOGO_N_X2, -LOGO_N_Y1,  LOGO_N_Z2,		// 52,1A (bottom, XZ plane, Y=-LOGO_N_Y1)
   LOGO_N_X2, -LOGO_N_Y1,  LOGO_N_Z2,		// 53,1B		
   LOGO_N_X2, -LOGO_N_Y1, -LOGO_N_Z2,		// 54,1C		  1D 1C
  -LOGO_N_X2, -LOGO_N_Y1, -LOGO_N_Z2,		// 55,1D		  1A 1B  
};

#define LOGO_M_X0  0
#define LOGO_M_X1  7
#define LOGO_M_X2 15
#define LOGO_M_X3 30
#define LOGO_M_Y0  0
#define LOGO_M_Y1  8
#define LOGO_M_Y2 15
#define LOGO_M_Y3 30
#define LOGO_M_Z0  0
#define LOGO_M_Z1  7
#define LOGO_M_Z2 15
#define LOGO_M_Z3 30

// 'M' logo vertex data
s8 M_verts[] ATTRIBUTE_ALIGN (32) =
{ // x y z
	-LOGO_M_X3, -LOGO_M_Y3,  LOGO_M_Z3,		//  0, 0 (side A, XY plane, Z=LOGO_M_Z3)
	-LOGO_M_X2, -LOGO_M_Y3,  LOGO_M_Z3,		//  1, 1		9 A    B C
	 LOGO_M_X2, -LOGO_M_Y3,  LOGO_M_Z3,		//  2, 2			8
	 LOGO_M_X3, -LOGO_M_Y3,  LOGO_M_Z3,		//  3, 3		  6    7
	-LOGO_M_X1,  LOGO_M_Y0,  LOGO_M_Z3,		//  4, 4		    45  
	 LOGO_M_X1,  LOGO_M_Y0,  LOGO_M_Z3,		//  5, 5		0 1    2 3
	-LOGO_M_X2,  LOGO_M_Y1,  LOGO_M_Z3,		//  6, 6
	 LOGO_M_X2,  LOGO_M_Y1,  LOGO_M_Z3,		//  7, 7
	 LOGO_M_X0,  LOGO_M_Y2,  LOGO_M_Z3,		//  8, 8
	-LOGO_M_X3,  LOGO_M_Y3,  LOGO_M_Z3,		//  9, 9
	-LOGO_M_X2,  LOGO_M_Y3,  LOGO_M_Z3,		// 10, A
	 LOGO_M_X2,  LOGO_M_Y3,  LOGO_M_Z3,		// 11, B
	 LOGO_M_X3,  LOGO_M_Y3,  LOGO_M_Z3,		// 12, C

	 LOGO_M_Z3, -LOGO_M_Y3,  LOGO_M_X3, 	// 13, 0 (side B, -ZY plane, X=LOGO_M_Z3)
	 LOGO_M_Z3,	-LOGO_M_Y3,  LOGO_M_X2, 	// 14, 1		9 A    B C
	 LOGO_M_Z3,	-LOGO_M_Y3, -LOGO_M_X2, 	// 15, 2			8
	 LOGO_M_Z3,	-LOGO_M_Y3, -LOGO_M_X3, 	// 16, 3		  6    7
	 LOGO_M_Z3,	 LOGO_M_Y0,  LOGO_M_X1, 	// 17, 4		    45  
	 LOGO_M_Z3,	 LOGO_M_Y0, -LOGO_M_X1, 	// 18, 5		0 1    2 3
	 LOGO_M_Z3,  LOGO_M_Y1,  LOGO_M_X2,		// 19, 6
	 LOGO_M_Z3,	 LOGO_M_Y1, -LOGO_M_X2, 	// 20, 7
	 LOGO_M_Z3,	 LOGO_M_Y2, -LOGO_M_X0, 	// 21, 8
	 LOGO_M_Z3,	 LOGO_M_Y3,  LOGO_M_X3, 	// 22, 9
	 LOGO_M_Z3,	 LOGO_M_Y3,  LOGO_M_X2, 	// 23, A
	 LOGO_M_Z3,	 LOGO_M_Y3, -LOGO_M_X2, 	// 24, B
	 LOGO_M_Z3,	 LOGO_M_Y3, -LOGO_M_X3, 	// 25, C

	 LOGO_M_X3, -LOGO_M_Y3, -LOGO_M_Z3,		// 26, 0 (side C, -XY plane, Z=-LOGO_M_Z3)
	 LOGO_M_X2, -LOGO_M_Y3, -LOGO_M_Z3,		// 27, 1		9 A    B C
	-LOGO_M_X2, -LOGO_M_Y3, -LOGO_M_Z3,		// 28, 2			8
	-LOGO_M_X3, -LOGO_M_Y3, -LOGO_M_Z3,		// 29, 3		  6    7
	 LOGO_M_X1,  LOGO_M_Y0, -LOGO_M_Z3,		// 30, 4		    45  
	-LOGO_M_X1,  LOGO_M_Y0, -LOGO_M_Z3,		// 31, 5		0 1    2 3
	 LOGO_M_X2,  LOGO_M_Y1, -LOGO_M_Z3,		// 32, 6
	-LOGO_M_X2,  LOGO_M_Y1, -LOGO_M_Z3,		// 33, 7
	-LOGO_M_X0,  LOGO_M_Y2, -LOGO_M_Z3,		// 34, 8
	 LOGO_M_X3,  LOGO_M_Y3, -LOGO_M_Z3,		// 35, 9
	 LOGO_M_X2,  LOGO_M_Y3, -LOGO_M_Z3,		// 36, A
	-LOGO_M_X2,  LOGO_M_Y3, -LOGO_M_Z3,		// 37, B
	-LOGO_M_X3,  LOGO_M_Y3, -LOGO_M_Z3,		// 38, C

	-LOGO_M_Z3, -LOGO_M_Y3, -LOGO_M_X3, 	// 39, 0 (side D, ZY plane, X=-LOGO_M_Z3)
	-LOGO_M_Z3,	-LOGO_M_Y3, -LOGO_M_X2, 	// 40, 1		9 A    B C
	-LOGO_M_Z3,	-LOGO_M_Y3,  LOGO_M_X2, 	// 41, 2			8
	-LOGO_M_Z3,	-LOGO_M_Y3,  LOGO_M_X3, 	// 42, 3		  6    7
	-LOGO_M_Z3,	 LOGO_M_Y0, -LOGO_M_X1, 	// 43, 4		    45  
	-LOGO_M_Z3,	 LOGO_M_Y0,  LOGO_M_X1, 	// 44, 5		0 1    2 3
	-LOGO_M_Z3,  LOGO_M_Y1, -LOGO_M_X2,		// 45, 6
	-LOGO_M_Z3,	 LOGO_M_Y1,  LOGO_M_X2, 	// 46, 7
	-LOGO_M_Z3,	 LOGO_M_Y2,  LOGO_M_X0, 	// 47, 8
	-LOGO_M_Z3,	 LOGO_M_Y3, -LOGO_M_X3, 	// 48, 9
	-LOGO_M_Z3,	 LOGO_M_Y3, -LOGO_M_X2, 	// 49, A
	-LOGO_M_Z3,	 LOGO_M_Y3,  LOGO_M_X2, 	// 50, B
	-LOGO_M_Z3,	 LOGO_M_Y3,  LOGO_M_X3, 	// 51, C

	-LOGO_M_X2,  LOGO_M_Y3,  LOGO_M_Z2,		// 52,9A (top, XZ plane, Y=LOGO_M_Y3)
	 LOGO_M_X2,  LOGO_M_Y3,  LOGO_M_Z2,		// 53,9B		
	 LOGO_M_X2,  LOGO_M_Y3, -LOGO_M_Z2,		// 54,9C		  9D 9C
	-LOGO_M_X2,  LOGO_M_Y3, -LOGO_M_Z2,		// 55,9D		  9A 9B  

	 LOGO_M_X0,  LOGO_M_Y2,  LOGO_M_Z2,		// 56,8A (upper-middle, XZ plane, Y=LOGO_M_Y2)
	 LOGO_M_X2,  LOGO_M_Y2,  LOGO_M_Z0,		// 57,8B		       8C
	 LOGO_M_X0,  LOGO_M_Y2, -LOGO_M_Z2,		// 58,8C	        8D    8B
	-LOGO_M_X2,  LOGO_M_Y2,  LOGO_M_Z0,		// 59,8D	           8A     

	-LOGO_M_X2,  LOGO_M_Y1,  LOGO_M_Z2,		// 60,6A (center-middle, XZ plane, Y=LOGO_M_Y1)
	 LOGO_M_X2,  LOGO_M_Y1,  LOGO_M_Z2,		// 61,6B		
	 LOGO_M_X2,  LOGO_M_Y1, -LOGO_M_Z2,		// 62,6C		  6D 6C
	-LOGO_M_X2,  LOGO_M_Y1, -LOGO_M_Z2,		// 63,6D		  6A 6B  

	-LOGO_M_X1,  LOGO_M_Y0,  LOGO_M_Z2,		// 64,4A (lower-middle, XZ plane, Y=LOGO_M_Y0)
	 LOGO_M_X1,  LOGO_M_Y0,  LOGO_M_Z2,		// 65,4B		
	 LOGO_M_X2,  LOGO_M_Y0,  LOGO_M_Z1,		// 66,4C		  4F 4E
	 LOGO_M_X2,  LOGO_M_Y0, -LOGO_M_Z1,		// 67,4D	   4G       4D 
	 LOGO_M_X1,  LOGO_M_Y0, -LOGO_M_Z2,		// 68,4E       4H       4C
	-LOGO_M_X1,  LOGO_M_Y0, -LOGO_M_Z2,		// 69,4F		  4A 4B
	-LOGO_M_X2,  LOGO_M_Y0, -LOGO_M_Z1,		// 70,4G		  
	-LOGO_M_X2,  LOGO_M_Y0,  LOGO_M_Z1,		// 71,4H		    

	-LOGO_M_X2, -LOGO_M_Y3,  LOGO_M_Z2,		// 72,1A (bottom, XZ plane, Y=-LOGO_M_Y3)
	 LOGO_M_X2, -LOGO_M_Y3,  LOGO_M_Z2,		// 73,1B		
	 LOGO_M_X2, -LOGO_M_Y3, -LOGO_M_Z2,		// 74,1C		  1D 1C
	-LOGO_M_X2, -LOGO_M_Y3, -LOGO_M_Z2,		// 75,1D		  1A 1B  
};

#define LOGO_W_Y0  0
#define LOGO_W_Y1 16
#define LOGO_W_Y2 16
#define LOGO_W_Y3 32
#define LOGO_W_Y4 64
#define LOGO_W_YOFFSET 32
#define LOGO_W_YSCALE 4
#define LOGO_W_X0  0
#define LOGO_W_X1  7
#define LOGO_W_X2 15
#define LOGO_W_X3 30
#define LOGO_W_X2L1 (LOGO_W_X2+(LOGO_W_Y2/LOGO_W_YSCALE))
#define LOGO_W_X3L1 (LOGO_W_X3+(LOGO_W_Y2/LOGO_W_YSCALE))
#define LOGO_W_X2L2 (LOGO_W_X2+(LOGO_W_Y3/LOGO_W_YSCALE))
#define LOGO_W_X3L2 (LOGO_W_X3+(LOGO_W_Y3/LOGO_W_YSCALE))
#define LOGO_W_X2L3 (LOGO_W_X2+(LOGO_W_Y4/LOGO_W_YSCALE))
#define LOGO_W_X3L3 (LOGO_W_X3+(LOGO_W_Y4/LOGO_W_YSCALE))
#define LOGO_W_Z0  0
#define LOGO_W_Z1  7
#define LOGO_W_Z2 15
#define LOGO_W_Z3 30
#define LOGO_W_Z2L1 (LOGO_W_Z2+(LOGO_W_Y2/LOGO_W_YSCALE))
#define LOGO_W_Z3L1 (LOGO_W_Z3+(LOGO_W_Y2/LOGO_W_YSCALE))
#define LOGO_W_Z2L2 (LOGO_W_Z2+(LOGO_W_Y3/LOGO_W_YSCALE))
#define LOGO_W_Z3L2 (LOGO_W_Z3+(LOGO_W_Y3/LOGO_W_YSCALE))
#define LOGO_W_Z2L3 (LOGO_W_Z2+(LOGO_W_Y4/LOGO_W_YSCALE))
#define LOGO_W_Z3L3 (LOGO_W_Z3+(LOGO_W_Y4/LOGO_W_YSCALE))

// 'W' logo vertex data
s8 W_verts[] ATTRIBUTE_ALIGN (32) =
{ // x y z
	-LOGO_W_X3L3, -LOGO_W_Y4,  LOGO_W_Z3L3,		//  0, 0 (side A, XY plane, Z=LOGO_W_Z3)
	-LOGO_W_X2L3, -LOGO_W_Y4,  LOGO_W_Z3L3,		//  1, 1		   9 A   B C		-Y0
	 LOGO_W_X2L3, -LOGO_W_Y4,  LOGO_W_Z3L3,		//  2, 2		 	   8			-Y1, L1?
	 LOGO_W_X3L3, -LOGO_W_Y4,  LOGO_W_Z3L3,		//  3, 3		    6     7			-Y2, L1
	-LOGO_W_X1,   -LOGO_W_Y3,  LOGO_W_Z3L2,		//  4, 4		       45			-Y3, L2
	 LOGO_W_X1,   -LOGO_W_Y3,  LOGO_W_Z3L2,		//  5, 5		0 1         2 3		-Y4, L3
	-LOGO_W_X2L1, -LOGO_W_Y2,  LOGO_W_Z3L1,		//  6, 6
	 LOGO_W_X2L1, -LOGO_W_Y2,  LOGO_W_Z3L1,		//  7, 7
	 LOGO_W_X0,   -LOGO_W_Y1,  LOGO_W_Z3L1,		//  8, 8
	-LOGO_W_X3,   -LOGO_W_Y0,  LOGO_W_Z3,		//  9, 9
	-LOGO_W_X2,   -LOGO_W_Y0,  LOGO_W_Z3,		// 10, A
	 LOGO_W_X2,   -LOGO_W_Y0,  LOGO_W_Z3,		// 11, B
	 LOGO_W_X3,   -LOGO_W_Y0,  LOGO_W_Z3,		// 12, C

	 LOGO_W_Z3L3, -LOGO_W_Y4,  LOGO_W_X3L3, 	// 13, 0 (side B, -ZY plane, X=LOGO_W_Z3)
	 LOGO_W_Z3L3, -LOGO_W_Y4,  LOGO_W_X2L3, 	// 14, 1		   9 A   B C		-Y0
	 LOGO_W_Z3L3, -LOGO_W_Y4, -LOGO_W_X2L3, 	// 15, 2		 	   8			-Y1, L1?
	 LOGO_W_Z3L3, -LOGO_W_Y4, -LOGO_W_X3L3, 	// 16, 3		    6     7			-Y2, L1
	 LOGO_W_Z3L2, -LOGO_W_Y3,  LOGO_W_X1,   	// 17, 4		       45			-Y3, L2
	 LOGO_W_Z3L2, -LOGO_W_Y3, -LOGO_W_X1,   	// 18, 5		0 1         2 3		-Y4, L3
	 LOGO_W_Z3L1, -LOGO_W_Y2,  LOGO_W_X2L1,		// 19, 6
	 LOGO_W_Z3L1, -LOGO_W_Y2, -LOGO_W_X2L1, 	// 20, 7
	 LOGO_W_Z3L1, -LOGO_W_Y1, -LOGO_W_X0,   	// 21, 8
	 LOGO_W_Z3,   -LOGO_W_Y0,  LOGO_W_X3,   	// 22, 9
	 LOGO_W_Z3,   -LOGO_W_Y0,  LOGO_W_X2,   	// 23, A
	 LOGO_W_Z3,   -LOGO_W_Y0, -LOGO_W_X2,   	// 24, B
	 LOGO_W_Z3,   -LOGO_W_Y0, -LOGO_W_X3,   	// 25, C

	 LOGO_W_X3L3, -LOGO_W_Y4, -LOGO_W_Z3L3,		// 26, 0 (side C, -XY plane, Z=-LOGO_W_Z3)
	 LOGO_W_X2L3, -LOGO_W_Y4, -LOGO_W_Z3L3,		// 27, 1		   9 A   B C		-Y0
	-LOGO_W_X2L3, -LOGO_W_Y4, -LOGO_W_Z3L3,		// 28, 2		 	   8			-Y1, L1?
	-LOGO_W_X3L3, -LOGO_W_Y4, -LOGO_W_Z3L3,		// 29, 3		    6     7			-Y2, L1
	 LOGO_W_X1,   -LOGO_W_Y3, -LOGO_W_Z3L2,		// 30, 4		       45			-Y3, L2
	-LOGO_W_X1,   -LOGO_W_Y3, -LOGO_W_Z3L2,		// 31, 5		0 1         2 3		-Y4, L3
	 LOGO_W_X2L1, -LOGO_W_Y2, -LOGO_W_Z3L1,		// 32, 6
	-LOGO_W_X2L1, -LOGO_W_Y2, -LOGO_W_Z3L1,		// 33, 7
	-LOGO_W_X0,   -LOGO_W_Y1, -LOGO_W_Z3L1,		// 34, 8
	 LOGO_W_X3,   -LOGO_W_Y0, -LOGO_W_Z3,		// 35, 9
	 LOGO_W_X2,   -LOGO_W_Y0, -LOGO_W_Z3,		// 36, A
	-LOGO_W_X2,   -LOGO_W_Y0, -LOGO_W_Z3,		// 37, B
	-LOGO_W_X3,   -LOGO_W_Y0, -LOGO_W_Z3,		// 38, C

	-LOGO_W_Z3L3, -LOGO_W_Y4, -LOGO_W_X3L3, 	// 39, 0 (side D, ZY plane, X=-LOGO_W_Z3)
	-LOGO_W_Z3L3, -LOGO_W_Y4, -LOGO_W_X2L3, 	// 40, 1		9 A    B C
	-LOGO_W_Z3L3, -LOGO_W_Y4,  LOGO_W_X2L3, 	// 41, 2			8
	-LOGO_W_Z3L3, -LOGO_W_Y4,  LOGO_W_X3L3, 	// 42, 3		  6    7
	-LOGO_W_Z3L2, -LOGO_W_Y3, -LOGO_W_X1,   	// 43, 4		    45  
	-LOGO_W_Z3L2, -LOGO_W_Y3,  LOGO_W_X1,   	// 44, 5		0 1    2 3
	-LOGO_W_Z3L1, -LOGO_W_Y2, -LOGO_W_X2L1,		// 45, 6
	-LOGO_W_Z3L1, -LOGO_W_Y2,  LOGO_W_X2L1, 	// 46, 7
	-LOGO_W_Z3L1, -LOGO_W_Y1,  LOGO_W_X0,   	// 47, 8
	-LOGO_W_Z3,   -LOGO_W_Y0, -LOGO_W_X3,   	// 48, 9
	-LOGO_W_Z3,   -LOGO_W_Y0, -LOGO_W_X2,   	// 49, A
	-LOGO_W_Z3,   -LOGO_W_Y0,  LOGO_W_X2,   	// 50, B
	-LOGO_W_Z3,   -LOGO_W_Y0,  LOGO_W_X3,   	// 51, C

	-LOGO_W_X2,   -LOGO_W_Y0,  LOGO_W_Z2,		// 52,9A (top, XZ plane, Y=-LOGO_W_Y0)
	 LOGO_W_X2,   -LOGO_W_Y0,  LOGO_W_Z2,		// 53,9B		
	 LOGO_W_X2,   -LOGO_W_Y0, -LOGO_W_Z2,		// 54,9C		  9D 9C
	-LOGO_W_X2,   -LOGO_W_Y0, -LOGO_W_Z2,		// 55,9D		  9A 9B  

	 LOGO_W_X0,   -LOGO_W_Y1,  LOGO_W_Z2L1,		// 56,8A (upper-middle, XZ plane, Y=-LOGO_W_Y1, L1?)
	 LOGO_W_X2L1, -LOGO_W_Y1,  LOGO_W_Z0,  		// 57,8B		       8C
	 LOGO_W_X0,	  -LOGO_W_Y1, -LOGO_W_Z2L1,		// 58,8C	        8D    8B
	-LOGO_W_X2L1, -LOGO_W_Y1,  LOGO_W_Z0,  		// 59,8D	           8A     

	-LOGO_W_X2L1, -LOGO_W_Y2,  LOGO_W_Z2L1,		// 60,6A (center-middle, XZ plane, Y=-LOGO_W_Y2, L1)
	 LOGO_W_X2L1, -LOGO_W_Y2,  LOGO_W_Z2L1,		// 61,6B		
	 LOGO_W_X2L1, -LOGO_W_Y2, -LOGO_W_Z2L1,		// 62,6C		  6D 6C
	-LOGO_W_X2L1, -LOGO_W_Y2, -LOGO_W_Z2L1,		// 63,6D		  6A 6B  

	-LOGO_W_X1,   -LOGO_W_Y3,  LOGO_W_Z2L2,		// 64,4A (lower-middle, XZ plane, Y=-LOGO_W_Y3, L2)
	 LOGO_W_X1,   -LOGO_W_Y3,  LOGO_W_Z2L2,		// 65,4B		
	 LOGO_W_X2L2, -LOGO_W_Y3,  LOGO_W_Z1,		// 66,4C		  4F 4E
	 LOGO_W_X2L2, -LOGO_W_Y3, -LOGO_W_Z1,		// 67,4D	   4G       4D 
	 LOGO_W_X1,   -LOGO_W_Y3, -LOGO_W_Z2L2,		// 68,4E       4H       4C
	-LOGO_W_X1,   -LOGO_W_Y3, -LOGO_W_Z2L2,		// 69,4F		  4A 4B
	-LOGO_W_X2L2, -LOGO_W_Y3, -LOGO_W_Z1,		// 70,4G		  
	-LOGO_W_X2L2, -LOGO_W_Y3,  LOGO_W_Z1,		// 71,4H		    

	-LOGO_W_X2L3, -LOGO_W_Y4,  LOGO_W_Z2L3,		// 72,1A (bottom, XZ plane, Y=-LOGO_W_Y4, L3)
	 LOGO_W_X2L3, -LOGO_W_Y4,  LOGO_W_Z2L3,		// 73,1B		
	 LOGO_W_X2L3, -LOGO_W_Y4, -LOGO_W_Z2L3,		// 74,1C		  1D 1C
	-LOGO_W_X2L3, -LOGO_W_Y4, -LOGO_W_Z2L3,		// 75,1D		  1A 1B  
};

// N64 logo color data
u8 logo_colors[] ATTRIBUTE_ALIGN (32) =
{ // r, g, b, a
	//'N' logo colors
	  8, 147,  48, 255,		// 0 green
	  1,  29, 169, 255,		// 1 blue
	254,  32,  21, 255,		// 2 orange/red
	255, 192,   1, 255,		// 3 yellow/gold
	//'M' logo colors
	230,   1,   1, 255,		// 4 red
	190, 190, 190, 255,		// 5 light gray
	255, 255, 255, 128,		// 6 white
	//'W' logo colors
	243, 243, 243, 255,		// 7 white
	102, 112, 123, 255,		// 8 gray
	101, 193, 244, 192,		// 9 wii slot blue
};

#define LOGO_MODE_MAX 3

Logo::Logo()
		: x(0),
		  y(0),
		  size(1),
		  rotateAuto(0),
		  rotateX(0),
		  rotateY(0)
{
	setVisible(false);
	srand ( gettick() );
	logoMode = rand() % LOGO_MODE_MAX;
}

Logo::~Logo()
{
}

void Logo::setLocation(float newX, float newY, float newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

void Logo::setSize(float newSize)
{
	size = newSize;
}

void Logo::setMode(int mode)
{
	logoMode = mode;
}

void Logo::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

void Logo::drawComponent(Graphics& gfx)
{
	Mtx v, m, mv, tmp;            // view, model, modelview, and perspective matrices
	guVector cam = { 0.0F, 0.0F, 0.0F }, 
		up = {0.0F, 1.0F, 0.0F}, 
		look = {0.0F, 0.0F, -1.0F},
		axisX = {1.0F, 0.0F, 0.0F},
		axisY = {0.0F, 1.0F, 0.0F};
	s8 stickX,stickY;

	guLookAt (v, &cam, &up, &look);
	rotateAuto++;

	u16 pad0 = PAD_ButtonsDown(0);
	if (pad0 & PAD_TRIGGER_Z) logoMode = (logoMode+1) % 3;

	//libOGC was changed such that sticks are now clamped and don't have to be by us
	stickX = PAD_SubStickX(0);
	stickY = PAD_SubStickY(0);
//	if(stickX > 18 || stickX < -18) rotateX += stickX/32;
//	if(stickY > 18 || stickY < -18) rotateY += stickY/32;
	rotateX += stickX/32;
	rotateY += stickY/32;

	// move the logo out in front of us and rotate it
	guMtxIdentity (m);
	if(logoMode == LOGO_W)	
	{
		guMtxRotAxisDeg (tmp, &axisX, 25);			//change to isometric view
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisX, -rotateY);
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisY, -rotateX);
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisY, rotateAuto);		//slowly rotate logo
		guMtxConcat (m, tmp, m);
		guMtxScale (tmp, 0.8f, 1.0f, 0.8f);
		guMtxConcat (m, tmp, m);
		guMtxTrans (tmp, 0, LOGO_W_YOFFSET, 0);
		guMtxConcat (m, tmp, m);
	}
	else
	{
		guMtxRotAxisDeg (tmp, &axisX, 25);			//change to isometric view
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisX, 180);			//flip rightside-up
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisX, -rotateY);
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisY, rotateX);
		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisY, -rotateAuto);		//slowly rotate logo
		guMtxConcat (m, tmp, m);
	}
	guMtxTransApply (m, m, x, y, z);
	guMtxConcat (v, m, mv);
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_SetCullMode (GX_CULL_BACK); // show only the outside facing quads
	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

	GX_SetLineWidth(8,GX_TO_ZERO);

	// setup the vertex descriptor
	GX_ClearVtxDesc ();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
	
	// setup the vertex attribute table
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	
	// set the array stride
	if(logoMode == LOGO_N)		GX_SetArray (GX_VA_POS, N_verts, 3 * sizeof (s8));
	else if(logoMode == LOGO_M)	GX_SetArray (GX_VA_POS, M_verts, 3 * sizeof (s8));
	else if(logoMode == LOGO_W)	GX_SetArray (GX_VA_POS, W_verts, 3 * sizeof (s8));
	GX_SetArray (GX_VA_CLR0, logo_colors, 4 * sizeof (u8));
	
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);

	if (logoMode == LOGO_N)
	{
		// 'N'
		GX_Begin (GX_QUADS, GX_VTXFMT0, 160);  //40 quads, so 160 verts
		drawQuad ( 0,  6,  7,  1, 0); //Side A, green
		drawQuad ( 7,  4,  2,  5, 0);
		drawQuad ( 2,  8,  9,  3, 0);
		drawQuad (31, 35, 47, 55, 0); //Side A1
		drawQuad (55, 51, 42, 46, 0);
		drawQuad (50, 42, 18, 14, 0);
		drawQuad (10, 16, 17, 11, 1); //Side B, blue
		drawQuad (17, 14, 12, 15, 1);
		drawQuad (12, 18, 19, 13, 1);
		drawQuad ( 1,  5, 44, 52, 1); //Side B1
		drawQuad (52, 48, 43, 47, 1);
		drawQuad (51, 43, 28, 24, 1);
		drawQuad (20, 26, 27, 21, 0); //Side C, green
		drawQuad (27, 24, 22, 25, 0);
		drawQuad (22, 28, 29, 23, 0);
		drawQuad (11, 15, 45, 53, 0); //Side C1
		drawQuad (53, 49, 40, 44, 0);
		drawQuad (48, 40, 38, 34, 0);
		drawQuad (30, 36, 37, 31, 1); //Side D, blue
		drawQuad (37, 34, 32, 35, 1);
		drawQuad (32, 38, 39, 33, 1);
		drawQuad (21, 25, 46, 54, 1); //Side D1
		drawQuad (54, 50, 41, 45, 1);
		drawQuad (49, 41,  8,  4, 1);
		drawQuad ( 6, 38, 40,  7, 3); //Top, yellow
		drawQuad ( 8, 41, 17,  9, 3);
		drawQuad (42, 27, 19, 18, 3);
		drawQuad (37, 29, 28, 43, 3); 
		drawQuad ( 7, 40, 49,  4, 2); //Top, red(green?)
		drawQuad (17, 41, 50, 14, 0);
		drawQuad (27, 42, 51, 24, 2);
		drawQuad (37, 43, 48, 34, 0); 
		drawQuad ( 0,  1, 52, 32, 3); //Bottom, yellow
		drawQuad ( 3, 11, 53,  2, 3);
		drawQuad (13, 21, 54, 12, 3);
		drawQuad (23, 31, 55, 22, 3); 
		drawQuad ( 2, 53, 44,  5, 2); //Bottom, red(green?)
		drawQuad (12, 54, 45, 15, 0);
		drawQuad (22, 55, 46, 25, 2);
		drawQuad (32, 52, 47, 35, 0); 
		GX_End ();
	}
	else if (logoMode == LOGO_M)
	{
		// 'M'
		GX_Begin (GX_QUADS, GX_VTXFMT0, 272);  //68 quads, so 272 verts 40+12+16
		drawQuad ( 0,  9, 10,  1, 4); //Side A, red
		drawQuad ( 6, 10,  8,  4, 4);
		drawQuad ( 4,  8,  8,  5, 4);
		drawQuad ( 5,  8, 11,  7, 4);
		drawQuad ( 2, 11, 12,  3, 4);
		drawQuad (40, 45, 63, 75, 4); //Side A1
		drawQuad (63, 55, 58, 69, 4);
		drawQuad (69, 58, 58, 68, 4);
		drawQuad (68, 58, 54, 62, 4);
		drawQuad (62, 20, 15, 74, 4);

		drawQuad (13, 22, 23, 14, 4); //Side B, red
		drawQuad (19, 23, 21, 17, 4);
		drawQuad (17, 21, 21, 18, 4);
		drawQuad (18, 21, 24, 20, 4);
		drawQuad (15, 24, 25, 16, 4);
		drawQuad ( 1,  6, 60, 72, 4); //Side B1
		drawQuad (52, 59, 71, 60, 4);
		drawQuad (71, 59, 59, 70, 4);
		drawQuad (70, 59, 55, 63, 4);
		drawQuad (75, 63, 33, 28, 4);

		drawQuad (26, 35, 36, 27, 4); //Side C, red
		drawQuad (32, 36, 34, 30, 4);
		drawQuad (30, 34, 34, 31, 4);
		drawQuad (31, 34, 37, 33, 4);
		drawQuad (28, 37, 38, 29, 4);
		drawQuad (14, 19, 61, 73, 4); //Side C1
		drawQuad (61, 53, 56, 65, 4);
		drawQuad (65, 56, 56, 64, 4);
		drawQuad (64, 56, 52, 60, 4);
		drawQuad (72, 60, 46, 41, 4);

		drawQuad (39, 48, 49, 40, 4); //Side D, red
		drawQuad (45, 49, 47, 43, 4);
		drawQuad (43, 47, 47, 44, 4);
		drawQuad (44, 47, 50, 46, 4);
		drawQuad (41, 50, 51, 42, 4);
		drawQuad (27, 32, 62, 74, 4); //Side D1
		drawQuad (62, 54, 57, 67, 4);
		drawQuad (67, 57, 57, 66, 4);
		drawQuad (66, 57, 53, 61, 4);
		drawQuad (73, 61,  7,  2, 4);

		drawQuad (10,  9, 50, 52, 5); //Top, gray
		drawQuad (49, 48, 37, 55, 5);
		drawQuad (36, 35, 24, 54, 5);
		drawQuad (23, 22, 11, 53, 5);

		drawQuad (59, 52, 50, 47, 5); //Top-slanted, gray
		drawQuad (47, 49, 55, 59, 5);
		drawQuad (58, 55, 37, 34, 5);
		drawQuad (34, 36, 54, 58, 5);
		drawQuad (57, 54, 24, 21, 5);
		drawQuad (21, 23, 53, 57, 5);
		drawQuad (56, 53, 11,  8, 5);
		drawQuad ( 8, 10, 52, 56, 5);

		drawQuad ( 0,  1, 72, 41, 5); //Bottom, gray
		drawQuad (39, 40, 75, 28, 5);
		drawQuad (26, 27, 74, 15, 5);
		drawQuad (13, 14, 73,  2, 5);

		drawQuad ( 6,  4, 64, 60, 5); //Bottom-slanted, gray
		drawQuad ( 4,  5, 65, 64, 5);
		drawQuad ( 5,  7, 61, 65, 5);

		drawQuad (19, 17, 66, 61, 5);
		drawQuad (17, 18, 67, 66, 5);
		drawQuad (18, 20, 62, 67, 5);

		drawQuad (32, 30, 68, 62, 5);
		drawQuad (30, 31, 69, 68, 5);
		drawQuad (31, 33, 63, 69, 5);

		drawQuad (45, 43, 70, 63, 5);
		drawQuad (43, 44, 71, 70, 5);
		drawQuad (44, 46, 60, 71, 5);
		GX_End ();

		guMtxTransApply (m, m, 0, 0, -2);
		guMtxConcat (v, m, mv);
		// load the modelview matrix into matrix memory
		GX_LoadPosMtxImm (mv, GX_PNMTX0);

		GX_Begin (GX_LINES, GX_VTXFMT0, 216);

		drawQuad ( 0,  9,  9, 10, 6);	//Side A
		drawQuad (10,  8,  8, 11, 6);
		drawQuad (11, 12, 12,  3, 6);
		drawQuad ( 3,  2,  2,  7, 6);
		drawQuad ( 7,  5,  5,  4, 6);
		drawQuad ( 4,  6,  6,  1, 6);
		drawLine ( 1,  0, 6);

		drawQuad (22, 23, 23, 21, 6);	//Side B
		drawQuad (21, 24, 24, 25, 6);
		drawQuad (25, 16, 16, 15, 6);
		drawQuad (15, 20, 20, 18, 6);
		drawQuad (18, 17, 17, 19, 6);
		drawQuad (19, 14, 14, 13, 6);

		drawQuad (35, 36, 36, 34, 6);	//Side C
		drawQuad (34, 37, 37, 38, 6);
		drawQuad (38, 29, 29, 28, 6);
		drawQuad (28, 33, 33, 31, 6);
		drawQuad (31, 30, 30, 32, 6);
		drawQuad (32, 27, 27, 26, 6);

		drawQuad (48, 49, 49, 47, 6);	//Side D
		drawQuad (47, 50, 50, 51, 6);
		drawLine (42, 41, 6);
		drawQuad (41, 46, 46, 44, 6);
		drawQuad (44, 43, 43, 45, 6);
		drawQuad (45, 40, 40, 39, 6);

		drawQuad (10, 52, 11, 53, 6);	//Top 
		drawQuad (23, 53, 24, 54, 6);
		drawQuad (36, 54, 37, 55, 6);
		drawQuad (49, 55, 50, 52, 6);
		drawQuad ( 8, 56, 21, 57, 6);
		drawQuad (34, 58, 47, 59, 6);
		drawQuad (52, 56, 56, 53, 6);
		drawQuad (53, 57, 57, 54, 6);
		drawQuad (54, 58, 58, 55, 6);
		drawQuad (55, 59, 59, 52, 6);
		drawQuad (52, 72, 53, 73, 6);	//Center Vertical
		drawQuad (54, 74, 55, 75, 6);

		drawQuad ( 1, 72,  2, 73, 6);	//bottom
		drawQuad (14, 73, 15, 74, 6);
		drawQuad (27, 74, 28, 75, 6);
		drawQuad (40, 75, 41, 72, 6);

		drawQuad ( 6, 60,  4, 64, 6);	//middle, radial
		drawQuad ( 5, 65,  7, 61, 6);
		drawQuad (19, 61, 17, 66, 6);
		drawQuad (18, 67, 20, 62, 6);
		drawQuad (32, 62, 30, 68, 6);
		drawQuad (31, 69, 33, 63, 6);
		drawQuad (45, 63, 43, 70, 6);
		drawQuad (44, 71, 46, 60, 6);

		drawQuad (60, 64, 64, 65, 6);	//middle, circumference
		drawQuad (65, 61, 61, 66, 6);
		drawQuad (66, 67, 67, 62, 6);
		drawQuad (62, 68, 68, 69, 6);
		drawQuad (69, 63, 63, 70, 6);
		drawQuad (70, 71, 71, 60, 6);

		GX_End ();
	}
	else if (logoMode == LOGO_W)
	{
		// 'W'
		GX_Begin (GX_QUADS, GX_VTXFMT0, 272);  //68 quads, so 272 verts 40+12+16
		drawQuad ( 0,  9, 10,  1, 7); //Side A, red
		drawQuad ( 6, 10,  8,  4, 7);
		drawQuad ( 4,  8,  8,  5, 7);
		drawQuad ( 5,  8, 11,  7, 7);
		drawQuad ( 2, 11, 12,  3, 7);
		drawQuad (40, 45, 63, 75, 7); //Side A1
		drawQuad (63, 55, 58, 69, 7);
		drawQuad (69, 58, 58, 68, 7);
		drawQuad (68, 58, 54, 62, 7);
		drawQuad (62, 20, 15, 74, 7);

		drawQuad (13, 22, 23, 14, 7); //Side B, red
		drawQuad (19, 23, 21, 17, 7);
		drawQuad (17, 21, 21, 18, 7);
		drawQuad (18, 21, 24, 20, 7);
		drawQuad (15, 24, 25, 16, 7);
		drawQuad ( 1,  6, 60, 72, 7); //Side B1
		drawQuad (52, 59, 71, 60, 7);
		drawQuad (71, 59, 59, 70, 7);
		drawQuad (70, 59, 55, 63, 7);
		drawQuad (75, 63, 33, 28, 7);

		drawQuad (26, 35, 36, 27, 7); //Side C, red
		drawQuad (32, 36, 34, 30, 7);
		drawQuad (30, 34, 34, 31, 7);
		drawQuad (31, 34, 37, 33, 7);
		drawQuad (28, 37, 38, 29, 7);
		drawQuad (14, 19, 61, 73, 7); //Side C1
		drawQuad (61, 53, 56, 65, 7);
		drawQuad (65, 56, 56, 64, 7);
		drawQuad (64, 56, 52, 60, 7);
		drawQuad (72, 60, 46, 41, 7);

		drawQuad (39, 48, 49, 40, 7); //Side D, red
		drawQuad (45, 49, 47, 43, 7);
		drawQuad (43, 47, 47, 44, 7);
		drawQuad (44, 47, 50, 46, 7);
		drawQuad (41, 50, 51, 42, 7);
		drawQuad (27, 32, 62, 74, 7); //Side D1
		drawQuad (62, 54, 57, 67, 7);
		drawQuad (67, 57, 57, 66, 7);
		drawQuad (66, 57, 53, 61, 7);
		drawQuad (73, 61,  7,  2, 7);

		drawQuad (10,  9, 50, 52, 8); //Top, gray
		drawQuad (49, 48, 37, 55, 8);
		drawQuad (36, 35, 24, 54, 8);
		drawQuad (23, 22, 11, 53, 8);

		drawQuad (59, 52, 50, 47, 8); //Top-slanted, gray
		drawQuad (47, 49, 55, 59, 8);
		drawQuad (58, 55, 37, 34, 8);
		drawQuad (34, 36, 54, 58, 8);
		drawQuad (57, 54, 24, 21, 8);
		drawQuad (21, 23, 53, 57, 8);
		drawQuad (56, 53, 11,  8, 8);
		drawQuad ( 8, 10, 52, 56, 8);

		drawQuad ( 0,  1, 72, 41, 8); //Bottom, gray
		drawQuad (39, 40, 75, 28, 8);
		drawQuad (26, 27, 74, 15, 8);
		drawQuad (13, 14, 73,  2, 8);

		drawQuad ( 6,  4, 64, 60, 8); //Bottom-slanted, gray
		drawQuad ( 4,  5, 65, 64, 8);
		drawQuad ( 5,  7, 61, 65, 8);

		drawQuad (19, 17, 66, 61, 8);
		drawQuad (17, 18, 67, 66, 8);
		drawQuad (18, 20, 62, 67, 8);

		drawQuad (32, 30, 68, 62, 8);
		drawQuad (30, 31, 69, 68, 8);
		drawQuad (31, 33, 63, 69, 8);

		drawQuad (45, 43, 70, 63, 8);
		drawQuad (43, 44, 71, 70, 8);
		drawQuad (44, 46, 60, 71, 8);
		GX_End ();

		guMtxTransApply (m, m, 0, 0, -2);
		guMtxConcat (v, m, mv);
		// load the modelview matrix into matrix memory
		GX_LoadPosMtxImm (mv, GX_PNMTX0);

		GX_Begin (GX_LINES, GX_VTXFMT0, 216);

		drawQuad ( 0,  9,  9, 10, 9);	//Side A
		drawQuad (10,  8,  8, 11, 9);
		drawQuad (11, 12, 12,  3, 9);
		drawQuad ( 3,  2,  2,  7, 9);
		drawQuad ( 7,  5,  5,  4, 9);
		drawQuad ( 4,  6,  6,  1, 9);
		drawLine ( 1,  0, 9);

		drawQuad (22, 23, 23, 21, 9);	//Side B
		drawQuad (21, 24, 24, 25, 9);
		drawQuad (25, 16, 16, 15, 9);
		drawQuad (15, 20, 20, 18, 9);
		drawQuad (18, 17, 17, 19, 9);
		drawQuad (19, 14, 14, 13, 9);

		drawQuad (35, 36, 36, 34, 9);	//Side C
		drawQuad (34, 37, 37, 38, 9);
		drawQuad (38, 29, 29, 28, 9);
		drawQuad (28, 33, 33, 31, 9);
		drawQuad (31, 30, 30, 32, 9);
		drawQuad (32, 27, 27, 26, 9);

		drawQuad (48, 49, 49, 47, 9);	//Side D
		drawQuad (47, 50, 50, 51, 9);
		drawLine (42, 41, 9);
		drawQuad (41, 46, 46, 44, 9);
		drawQuad (44, 43, 43, 45, 9);
		drawQuad (45, 40, 40, 39, 9);

		drawQuad (10, 52, 11, 53, 9);	//Top 
		drawQuad (23, 53, 24, 54, 9);
		drawQuad (36, 54, 37, 55, 9);
		drawQuad (49, 55, 50, 52, 9);
		drawQuad ( 8, 56, 21, 57, 9);
		drawQuad (34, 58, 47, 59, 9);
		drawQuad (52, 56, 56, 53, 9);
		drawQuad (53, 57, 57, 54, 9);
		drawQuad (54, 58, 58, 55, 9);
		drawQuad (55, 59, 59, 52, 9);
		drawQuad (52, 72, 53, 73, 9);	//Center Vertical
		drawQuad (54, 74, 55, 75, 9);

		drawQuad ( 1, 72,  2, 73, 9);	//bottom
		drawQuad (14, 73, 15, 74, 9);
		drawQuad (27, 74, 28, 75, 9);
		drawQuad (40, 75, 41, 72, 9);

		drawQuad ( 6, 60,  4, 64, 9);	//middle, radial
		drawQuad ( 5, 65,  7, 61, 9);
		drawQuad (19, 61, 17, 66, 9);
		drawQuad (18, 67, 20, 62, 9);
		drawQuad (32, 62, 30, 68, 9);
		drawQuad (31, 69, 33, 63, 9);
		drawQuad (45, 63, 43, 70, 9);
		drawQuad (44, 71, 46, 60, 9);

		drawQuad (60, 64, 64, 65, 9);	//middle, circumference
		drawQuad (65, 61, 61, 66, 9);
		drawQuad (66, 67, 67, 62, 9);
		drawQuad (62, 68, 68, 69, 9);
		drawQuad (69, 63, 63, 70, 9);
		drawQuad (70, 71, 71, 60, 9);

		GX_End ();
	}
		
	//Reset GX state:
	GX_SetLineWidth(6,GX_TO_ZERO);
	gfx.drawInit();
}

void Logo::drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 c)
{
	// draws a quad from 4 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	// one 8bit color idx
	GX_Color1x8 (c);
	GX_Position1x8 (v1);
	GX_Color1x8 (c);
	GX_Position1x8 (v2);
	GX_Color1x8 (c);
	GX_Position1x8 (v3);
	GX_Color1x8 (c);
}

void Logo::drawLine(u8 v0, u8 v1, u8 c)
{
	// draws a line from 2 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	// one 8bit color idx
	GX_Color1x8 (c);
	GX_Position1x8 (v1);
	GX_Color1x8 (c);
}

} //namespace menu 
