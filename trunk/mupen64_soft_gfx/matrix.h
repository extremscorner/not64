/**
 * Mupen64 - matrix.h
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

#ifndef MATRIX_H
#define MATRIX_H

template<class T, int s> class Matrix
{
   T m[s][s];
   Matrix<T,s>* stack;
   
 public:
   Matrix() : stack(NULL) {}
   ~Matrix() { if(stack) delete stack; }
   
   Matrix(const Matrix<T,s>& o) : stack(NULL)
     {
	for (int i=0; i<s; i++)
	  for (int j=0; j<s; j++)
	    m[i][j] = o.m[i][j];
     }
   
   Matrix<T,s>& operator=(const Matrix<T,s>& o)
     {
	for (int i=0; i<s; i++)
	  for (int j=0; j<s; j++)
	    m[i][j] = o.m[i][j];
	return *this;
     }
   
   T& operator() (int i, int j)
     {
	return m[i][j];
     }
   
   Matrix<T,s> operator*(const Matrix<T,s>& o) const
     {
	Matrix<T,s> temp;
	for (int i=0; i<s; i++)
	  {
	     for (int j=0; j<s; j++)
	       {
		  temp.m[i][j] = 0;
		  for (int k=0; k<s; k++)
		    temp.m[i][j] = temp.m[i][j] + m[i][k] * o.m[k][j];
	       }
	  }
	return temp;
     }
   
   Vector<T,s> operator*(Vector<T,s>& v) const
     {
	Vector<T,s> temp;
	for (int i=0; i<s; i++)
	  {
	     temp[i] = 0;
	     for (int j=0; j<s; j++)
	       temp[i] = temp[i] + m[i][j] * v[j];
	  }
	return temp;
     }
   
   friend Vector<T,s> operator*(Vector<T,s>& v, Matrix<T,s>& m)
     {
	Vector<T,s> temp;
	for (int i=0; i<s; i++)
	  {
	     temp[i] = 0;
	     for (int j=0; j<s; j++)
	       temp[i] += v[j] * m.m[j][i];
	  }
	return temp;
     }
   
   void push()
     {
	Matrix<T,s>* temp = new Matrix<T,s>(*this);
	temp->stack = stack;
	stack = temp;
     }
   
   void pop()
     {
	*this = *stack;
	Matrix<T,s>* temp = stack->stack;
	stack->stack = NULL;
	delete stack;
	stack = temp;
     }
};

#endif
