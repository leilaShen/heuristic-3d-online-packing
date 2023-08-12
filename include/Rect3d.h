/** @file Rect.h
	@author Jukka Jylänki
	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <vector>
#include <cassert>
#include <cstdlib>

#ifdef _DEBUG
/// debug_assert is an assert that also requires debug mode to be defined.
#define debug_assert(x) assert(x)
#else
#define debug_assert(x)
#endif

//using namespace std;

namespace rbp {

struct RectSize3d
{
	int width;
	int height;
    int depth;
};

struct Rect3d
{
	int x;
	int y;
    int z;
	int width;
	int height;
    int depth;
};

struct FreeRect3d{
	int x;
	int y;
	int z;
	int width;
	int height;
	int depth;
	//int supportWidth;
	//int supportHeight;
	//support x start
	int supportx0;
	//support x end
	int supportx1;
	//support y start
	int supporty0;
	//support y end
	int supporty1;
};

/// Performs a lexicographic compare on (rect short side, rect long side).
/// @return -1 if the smaller side of a is shorter than the smaller side of b, 1 if the other way around.
///   If they are equal, the larger side length is used as a tie-breaker.
///   If the rectangles are of same size, returns 0.
int CompareRectShortSide3d(const Rect3d &a, const Rect3d &b);

/// Performs a lexicographic compare on (x, y, width, height).
int NodeSortCmp3d(const Rect3d &a, const Rect3d &b);

/// Returns true if a is contained in b.
bool IsContainedIn3d(const Rect3d &a, const Rect3d &b);
bool IsContainedInFree3d(const FreeRect3d &a, const FreeRect3d &b);


class DisjointRectCollection3d
{
public:
	std::vector<Rect3d> rects;

	bool Add(const Rect3d &r)
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0 || r.depth == 0)
			return true;

		if (!Disjoint(r))
			return false;
		rects.push_back(r);
		return true;
	}

	void Clear()
	{
		rects.clear();
	}

	bool Disjoint(const Rect3d &r) const
	{
		// Degenerate rectangles are ignored.
		if (r.width == 0 || r.height == 0 || r.depth == 0)
			return true;

		for(size_t i = 0; i < rects.size(); ++i)
			if (!Disjoint(rects[i], r))
				return false;
		return true;
	}

	static bool Disjoint(const Rect3d &a, const Rect3d &b)
	{
		if (a.x + a.width <= b.x ||
			b.x + b.width <= a.x ||
			a.y + a.height <= b.y ||
			b.y + b.height <= a.y ||
            a.z + a.depth <= b.z ||
            b.z + b.depth <= a.z)
			return true;
		return false;
	}
};

}

