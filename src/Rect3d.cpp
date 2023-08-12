/** @file Rect.cpp
	@author Jukka Jylänki
	This work is released to Public Domain, do whatever you want with it.
*/
#include <utility>

#include "../include/Rect3d.h"

namespace rbp {

bool IsContainedIn3d(const Rect3d &a, const Rect3d &b)
{
	return a.x >= b.x && a.y >= b.y 
		&& a.x+a.width <= b.x+b.width 
		&& a.y+a.height <= b.y+b.height
		&& a.z + a.depth <= b.z + b.depth;
}

bool IsContainedInFree3d(const FreeRect3d &a, const FreeRect3d &b)
{
	return a.x >= b.x && a.y >= b.y 
		&& a.x+a.width <= b.x+b.width 
		&& a.y+a.height <= b.y+b.height 
		&& a.z >= b.z && a.z + a.depth >= b.z + b.depth;
}

}