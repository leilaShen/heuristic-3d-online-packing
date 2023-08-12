/** @file GuillotineBinPack.cpp
	@author Jukka Jylänki
	@brief Implements different bin packer algorithms that use the GUILLOTINE data structure.
	This work is released to Public Domain, do whatever you want with it.
*/
#include <algorithm>
#include <utility>
#include <iostream>
#include <limits>

#include <cassert>
#include <cstring>
#include <cmath>

#include "../include/GuillotineBinPack3d.h"

namespace rbp {

using namespace std;

GuillotineBinPack3d::GuillotineBinPack3d()
:binWidth(0),
binHeight(0),
binDepth(0)
{
}

GuillotineBinPack3d::GuillotineBinPack3d(int width, int height, int depth)
{
	Init(width, height, depth);
}

void GuillotineBinPack3d::Init(int width, int height, int depth)
{
	binWidth = width;
	binHeight = height;
    binDepth = depth;

#ifdef _DEBUG
	disjointRects.Clear();
#endif

	// Clear any memory of previously packed rectangles.
	usedRectangles.clear();

	// We start with a single big free rectangle that spans the whole bin.
	Rect3d n;
	n.x = 0;
	n.y = 0;
    n.z = 0;
	n.width = width;
	n.height = height;
    n.depth = depth;

	freeRectangles.clear();
	freeRectangles.push_back(n);
}

void GuillotineBinPack3d::Insert(std::vector<RectSize3d> &rects, bool merge, 
	FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod)
{
	// Remember variables about the best packing choice we have made so far during the iteration process.
	int bestFreeRect = 0;
	int bestRect = 0;
	bool bestFlipped = false;

	// Pack rectangles one at a time until we have cleared the rects array of all rectangles.
	// rects will get destroyed in the process.
	while(rects.size() > 0)
	{
		// Stores the penalty score of the best rectangle placement - bigger=worse, smaller=better.
		int bestScore = std::numeric_limits<int>::max();

		for(size_t i = 0; i < freeRectangles.size(); ++i)
		{
			for(size_t j = 0; j < rects.size(); ++j)
			{
				// If this rectangle is a perfect match, we pick it instantly.
				if (rects[j].width == freeRectangles[i].width && rects[j].height == freeRectangles[i].height && rects[j].depth == freeRectangles[i].depth)
				{
					bestFreeRect = i;
					bestRect = j;
					bestFlipped = false;
					bestScore = std::numeric_limits<int>::min();
					i = freeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
					break;
				}
				// If flipping this rectangle is a perfect match, pick that then.
				else if (rects[j].height == freeRectangles[i].width && rects[j].width == freeRectangles[i].height && rects[j].depth == freeRectangles[i].depth)
				{
					bestFreeRect = i;
					bestRect = j;
					bestFlipped = true;
					bestScore = std::numeric_limits<int>::min();
					i = freeRectangles.size(); // Force a jump out of the outer loop as well - we got an instant fit.
					break;
				}
				// Try if we can fit the rectangle upright.
				else if (rects[j].width <= freeRectangles[i].width && rects[j].height <= freeRectangles[i].height && rects[j].depth <= freeRectangles[j].depth)
				{
					int score = ScoreByHeuristic(rects[j].width, rects[j].height, rects[j].depth, freeRectangles[i], rectChoice);
					if (score < bestScore)
					{
						bestFreeRect = i;
						bestRect = j;
						bestFlipped = false;
						bestScore = score;
					}
				}
				// If not, then perhaps flipping sideways will make it fit?
				else if (rects[j].height <= freeRectangles[i].width && rects[j].width <= freeRectangles[i].height && rects[j].depth <= freeRectangles[i].depth)
				{
					int score = ScoreByHeuristic(rects[j].height, rects[j].width, rects[j].depth, freeRectangles[i], rectChoice);
					if (score < bestScore)
					{
						bestFreeRect = i;
						bestRect = j;
						bestFlipped = true;
						bestScore = score;
					}
				}
			}
		}

		// If we didn't manage to find any rectangle to pack, abort.
		if (bestScore == std::numeric_limits<int>::max())
			return;

		// Otherwise, we're good to go and do the actual packing.
		Rect3d newNode;
		newNode.x = freeRectangles[bestFreeRect].x;
		newNode.y = freeRectangles[bestFreeRect].y;
        newNode.z = freeRectangles[bestFreeRect].z;
		newNode.width = rects[bestRect].width;
		newNode.height = rects[bestRect].height;
        newNode.depth = rects[bestRect].depth;

		if (bestFlipped)
			std::swap(newNode.width, newNode.height);

		// Remove the free space we lost in the bin.
		SplitFreeRectByHeuristic(freeRectangles[bestFreeRect], newNode, splitMethod);
		freeRectangles.erase(freeRectangles.begin() + bestFreeRect);

		// Remove the rectangle we just packed from the input list.
		rects.erase(rects.begin() + bestRect);

		// Perform a Rectangle Merge step if desired.
		if (merge)
			MergeFreeList();

		// Remember the new used rectangle.
		usedRectangles.push_back(newNode);

		// Check that we're really producing correct packings here.
		debug_assert(disjointRects.Add(newNode) == true);
	}
}

/// @return True if r fits inside freeRect (possibly rotated).
bool Fits(const RectSize3d &r, const Rect3d &freeRect)
{
	return (r.width <= freeRect.width && r.height <= freeRect.height && r.depth <= freeRect.depth) ||
		(r.height <= freeRect.width && r.width <= freeRect.height && r.depth <= freeRect.depth);
}

/// @return True if r fits perfectly inside freeRect, i.e. the leftover area is 0.
bool FitsPerfectly(const RectSize3d &r, const Rect3d &freeRect)
{
	return (r.width == freeRect.width && r.height == freeRect.height && r.depth == freeRect.depth) ||
		(r.height == freeRect.width && r.width == freeRect.height && r.depth == freeRect.depth);
}

/*
// A helper function for GUILLOTINE-MAXFITTING. Counts how many rectangles fit into the given rectangle
// after it has been split.
void CountNumFitting(const Rect &freeRect, int width, int height, const std::vector<RectSize> &rects, 
	int usedRectIndex, bool splitHorizontal, int &score1, int &score2)
{
	const int w = freeRect.width - width;
	const int h = freeRect.height - height;
	Rect bottom;
	bottom.x = freeRect.x;
	bottom.y = freeRect.y + height;
	bottom.height = h;
	Rect right;
	right.x = freeRect.x + width;
	right.y = freeRect.y;
	right.width = w;
	if (splitHorizontal)
	{
		bottom.width = freeRect.width;
		right.height = height;
	}
	else // Split vertically
	{
		bottom.width = width;
		right.height = freeRect.height;
	}
	int fitBottom = 0;
	int fitRight = 0;
	for(size_t i = 0; i < rects.size(); ++i)
		if (i != usedRectIndex)
		{
			if (FitsPerfectly(rects[i], bottom))
				fitBottom |= 0x10000000;
			if (FitsPerfectly(rects[i], right))
				fitRight |= 0x10000000;
			if (Fits(rects[i], bottom))
				++fitBottom;
			if (Fits(rects[i], right))
				++fitRight;
		}
	score1 = min(fitBottom, fitRight);
	score2 = max(fitBottom, fitRight);
}
*/
/*
// Implements GUILLOTINE-MAXFITTING, an experimental heuristic that's really cool but didn't quite work in practice.
void GuillotineBinPack::InsertMaxFitting(std::vector<RectSize> &rects, std::vector<Rect> &dst, bool merge, 
	FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod)
{
	dst.clear();
	int bestRect = 0;
	bool bestFlipped = false;
	bool bestSplitHorizontal = false;
	// Pick rectangles one at a time and pack the one that leaves the most choices still open.
	while(rects.size() > 0 && freeRectangles.size() > 0)
	{
		int bestScore1 = -1;
		int bestScore2 = -1;
		///\todo Different sort predicates.
		clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), CompareRectShortSide);
		Rect &freeRect = freeRectangles[0];
		for(size_t j = 0; j < rects.size(); ++j)
		{
			int score1;
			int score2;
			if (rects[j].width == freeRect.width && rects[j].height == freeRect.height)
			{
				bestRect = j;
				bestFlipped = false;
				bestScore1 = bestScore2 = std::numeric_limits<int>::max();
				break;
			}
			else if (rects[j].width <= freeRect.width && rects[j].height <= freeRect.height)
			{
				CountNumFitting(freeRect, rects[j].width, rects[j].height, rects, j, false, score1, score2);
				if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
				{
					bestRect = j;
					bestScore1 = score1;
					bestScore2 = score2;
					bestFlipped = false;
					bestSplitHorizontal = false;
				}
				CountNumFitting(freeRect, rects[j].width, rects[j].height, rects, j, true, score1, score2);
				if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
				{
					bestRect = j;
					bestScore1 = score1;
					bestScore2 = score2;
					bestFlipped = false;
					bestSplitHorizontal = true;
				}
			}
			if (rects[j].height == freeRect.width && rects[j].width == freeRect.height)
			{
				bestRect = j;
				bestFlipped = true;
				bestScore1 = bestScore2 = std::numeric_limits<int>::max();
				break;
			}
			else if (rects[j].height <= freeRect.width && rects[j].width <= freeRect.height)
			{
				CountNumFitting(freeRect, rects[j].height, rects[j].width, rects, j, false, score1, score2);
				if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
				{
					bestRect = j;
					bestScore1 = score1;
					bestScore2 = score2;
					bestFlipped = true;
					bestSplitHorizontal = false;
				}
				CountNumFitting(freeRect, rects[j].height, rects[j].width, rects, j, true, score1, score2);
				if (score1 > bestScore1 || (score1 == bestScore1 && score2 > bestScore2))
				{
					bestRect = j;
					bestScore1 = score1;
					bestScore2 = score2;
					bestFlipped = true;
					bestSplitHorizontal = true;
				}
			}
		}
		if (bestScore1 >= 0)
		{
			Rect newNode;
			newNode.x = freeRect.x;
			newNode.y = freeRect.y;
			newNode.width = rects[bestRect].width;
			newNode.height = rects[bestRect].height;
			if (bestFlipped)
				std::swap(newNode.width, newNode.height);
			assert(disjointRects.Disjoint(newNode));
			SplitFreeRectAlongAxis(freeRect, newNode, bestSplitHorizontal);
			rects.erase(rects.begin() + bestRect);
			if (merge)
				MergeFreeList();
			usedRectangles.push_back(newNode);
#ifdef _DEBUG
			disjointRects.Add(newNode);
#endif
		}
		freeRectangles.erase(freeRectangles.begin());
	}
}
*/

Rect3d GuillotineBinPack3d::Insert(int width, int height, int depth, bool merge, FreeRectChoiceHeuristic rectChoice, 
	GuillotineSplitHeuristic splitMethod)
{
	// Find where to put the new rectangle.
	int freeNodeIndex = 0;
	Rect3d newRect = FindPositionForNewNode(width, height, depth, rectChoice, &freeNodeIndex);

	// Abort if we didn't have enough space in the bin.
	if (newRect.height == 0)
		return newRect;

	// Remove the space that was just consumed by the new rectangle.
	SplitFreeRectByHeuristic(freeRectangles[freeNodeIndex], newRect, splitMethod);
	freeRectangles.erase(freeRectangles.begin() + freeNodeIndex);

	// Perform a Rectangle Merge step if desired.
	if (merge)
		MergeFreeList();

	// Remember the new used rectangle.
	usedRectangles.push_back(newRect);

	// Check that we're really producing correct packings here.
	debug_assert(disjointRects.Add(newRect) == true);

	return newRect;
}

/// Computes the ratio of used surface area to the total bin area.
float GuillotineBinPack3d::Occupancy() const
{
	///\todo The occupancy rate could be cached/tracked incrementally instead
	///      of looping through the list of packed rectangles here.
	unsigned long usedSurfaceArea = 0;
	for(size_t i = 0; i < usedRectangles.size(); ++i)
		usedSurfaceArea += usedRectangles[i].width * usedRectangles[i].height * usedRectangles[i].depth;

	return (float)usedSurfaceArea / (binWidth * binHeight * binDepth);
}

/// Returns the heuristic score value for placing a rectangle of size width*height into freeRect. Does not try to rotate.
int GuillotineBinPack3d::ScoreByHeuristic(int width, int height, int depth, const Rect3d &freeRect, FreeRectChoiceHeuristic rectChoice)
{
	switch(rectChoice)
	{
	case RectBestAreaFit: return ScoreBestAreaFit(width, height,depth, freeRect);
	case RectBestShortSideFit: return ScoreBestShortSideFit(width, height,depth, freeRect);
	case RectBestLongSideFit: return ScoreBestLongSideFit(width, height, depth,freeRect);
	case RectWorstAreaFit: return ScoreWorstAreaFit(width, height, depth, freeRect);
	case RectWorstShortSideFit: return ScoreWorstShortSideFit(width, height, depth,freeRect);
	case RectWorstLongSideFit: return ScoreWorstLongSideFit(width, height, depth, freeRect);
	default: assert(false); return std::numeric_limits<int>::max();
	}
}

int GuillotineBinPack3d::ScoreBestAreaFit(int width, int height, int depth, const Rect3d &freeRect)
{
	return freeRect.width * freeRect.height * freeRect.depth - width * height * depth;
}

int GuillotineBinPack3d::ScoreBestShortSideFit(int width, int height, int depth, const Rect3d &freeRect)
{
	int leftoverHoriz = abs(freeRect.width - width);
	int leftoverVert = abs(freeRect.height - height);
    int leftoverDepth = abs(freeRect.depth - depth);
	int leftover = min(leftoverHoriz, leftoverVert);
    leftover = min(leftover, leftoverDepth);
	return leftover;
}

int GuillotineBinPack3d::ScoreBestLongSideFit(int width, int height, int depth, const Rect3d &freeRect)
{
	int leftoverHoriz = abs(freeRect.width - width);
	int leftoverVert = abs(freeRect.height - height);
    int leftoverDepth = abs(freeRect.depth - depth);
	int leftover = max(leftoverHoriz, leftoverVert);
    leftover = max(leftover, leftoverDepth);
	return leftover;
}

int GuillotineBinPack3d::ScoreWorstAreaFit(int width, int height, int depth, const Rect3d &freeRect)
{
	return -ScoreBestAreaFit(width, height, depth, freeRect);
}

int GuillotineBinPack3d::ScoreWorstShortSideFit(int width, int height, int depth, const Rect3d &freeRect)
{
	return -ScoreBestShortSideFit(width, height, depth, freeRect);
}

int GuillotineBinPack3d::ScoreWorstLongSideFit(int width, int height, int depth, const Rect3d &freeRect)
{
	return -ScoreBestLongSideFit(width, height, depth, freeRect);
}

Rect3d GuillotineBinPack3d::FindPositionForNewNode(int width, int height, int depth, FreeRectChoiceHeuristic rectChoice, int *nodeIndex)
{
	Rect3d bestNode;
	memset(&bestNode, 0, sizeof(Rect3d));

	int bestScore = std::numeric_limits<int>::max();
	auto sortFreeRectsByZ=[this](const Rect3d& rect1, const Rect3d& rect2){
		long rect1_hash_val = rect1.x + rect1.y * this->binWidth + rect1.z * this->binWidth * this->binHeight;
		long rect2_hash_val = rect2.x + rect2.y * this->binWidth + rect2.z * this->binWidth * this->binHeight;
		return rect1_hash_val < rect2_hash_val;
	};
	std::cout << "----------------------------------------------" << std::endl;
	std::sort(std::begin(freeRectangles), std::end(freeRectangles), sortFreeRectsByZ);
    std::cout << freeRectangles[0].x << "," << freeRectangles[0].y << "," << freeRectangles[0].z << std::endl;
	if(freeRectangles.size() > 1){
		std::cout << freeRectangles[1].x << "," << freeRectangles[1].y << "," << freeRectangles[1].z << std::endl;
	}
	if(freeRectangles.size() > 2){
		std::cout << freeRectangles[2].x << "," << freeRectangles[2].y << "," << freeRectangles[2].z << std::endl;
	}
	/// Try each free rectangle to find the best one for placement.
	for(size_t i = 0; i < freeRectangles.size(); ++i)
	{
		// If this is a perfect fit upright, choose it immediately.
		if (width == freeRectangles[i].width && height == freeRectangles[i].height && depth == freeRectangles[i].depth)
		{
			bestNode.x = freeRectangles[i].x;
			bestNode.y = freeRectangles[i].y;
            bestNode.z = freeRectangles[i].z;
			bestNode.width = width;
			bestNode.height = height;
            bestNode.depth = depth;
			bestScore = std::numeric_limits<int>::min();
			*nodeIndex = i;
			debug_assert(disjointRects.Disjoint(bestNode));
			break;
		}
		// If this is a perfect fit sideways, choose it.
		else if (height == freeRectangles[i].width && width == freeRectangles[i].height && depth == freeRectangles[i].depth)
		{
			bestNode.x = freeRectangles[i].x;
			bestNode.y = freeRectangles[i].y;
            bestNode.z = freeRectangles[i].z;
			bestNode.width = height;
			bestNode.height = width;
            bestNode.depth = depth;
			bestScore = std::numeric_limits<int>::min();
			*nodeIndex = i;
			debug_assert(disjointRects.Disjoint(bestNode));
			break;
		}
		// Does the rectangle fit upright?
		else if (width <= freeRectangles[i].width && height <= freeRectangles[i].height && depth <= freeRectangles[i].depth)
		{
			//int score = ScoreByHeuristic(width, height, depth, freeRectangles[i], rectChoice);

			//if (score < bestScore)
			//{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
                bestNode.z = freeRectangles[i].z;
				bestNode.width = width;
				bestNode.height = height;
                bestNode.depth = depth;
			//	bestScore = score;
				*nodeIndex = i;
				debug_assert(disjointRects.Disjoint(bestNode));
				break;
			//}
		}
		// Does the rectangle fit sideways?
		else if (height <= freeRectangles[i].width && width <= freeRectangles[i].height && depth <= freeRectangles[i].depth)
		{
			//int score = ScoreByHeuristic(height, width, depth, freeRectangles[i], rectChoice);

			//if (score < bestScore)
			//{
				bestNode.x = freeRectangles[i].x;
				bestNode.y = freeRectangles[i].y;
                bestNode.z = freeRectangles[i].z;
				bestNode.width = height;
				bestNode.height = width;
                bestNode.depth = depth;
			//	bestScore = score;
				*nodeIndex = i;
				debug_assert(disjointRects.Disjoint(bestNode));
				break;
			//}
		}
	}
	return bestNode;
}

void GuillotineBinPack3d::SplitFreeRectByHeuristic(const Rect3d &freeRect, const Rect3d &placedRect, GuillotineSplitHeuristic method)
{
	// Compute the lengths of the leftover area.
	const int w = freeRect.width - placedRect.width;
	const int h = freeRect.height - placedRect.height;
    const int d = freeRect.depth - placedRect.depth;

	// Placing placedRect into freeRect results in an L-shaped free area, which must be split into
	// two disjoint rectangles. This can be achieved with by splitting the L-shape using a single line.
	// We have two choices: horizontal or vertical.	

	// Use the given heuristic to decide which choice to make.

	bool splitHorizontal;
    // In Zhang defu's paper, he accept SplitShorterLeftoverAxis method
	switch(method)
	{
	case SplitShorterLeftoverAxis:
		// Split along the shorter leftover axis.
		splitHorizontal = (w <= h);
		break;
	case SplitLongerLeftoverAxis:
		// Split along the longer leftover axis.
		splitHorizontal = (w > h);
		break;
	case SplitMinimizeArea:
		// Maximize the larger area == minimize the smaller area.
		// Tries to make the single bigger rectangle.
		splitHorizontal = (placedRect.width * h > w * placedRect.height);
		break;
	case SplitMaximizeArea:
		// Maximize the smaller area == minimize the larger area.
		// Tries to make the rectangles more even-sized.
		splitHorizontal = (placedRect.width * h <= w * placedRect.height);
		break;
	case SplitShorterAxis:
		// Split along the shorter total axis.
		splitHorizontal = (freeRect.width <= freeRect.height);
		break;
	case SplitLongerAxis:
		// Split along the longer total axis.
		splitHorizontal = (freeRect.width > freeRect.height);
		break;
	default:
		splitHorizontal = true;
		assert(false);
	}

	// Perform the actual split.
	SplitFreeRectAlongAxis(freeRect, placedRect, splitHorizontal);
}

/// This function will add the two generated rectangles into the freeRectangles array. The caller is expected to
/// remove the original rectangle from the freeRectangles array after that.
void GuillotineBinPack3d::SplitFreeRectAlongAxis(const Rect3d &freeRect, const Rect3d &placedRect, bool splitHorizontal)
{
	// Form the two new rectangles.
	Rect3d bottom;
	bottom.x = freeRect.x;
	bottom.y = freeRect.y + placedRect.height;
    bottom.z = freeRect.z;
	bottom.height = freeRect.height - placedRect.height;
    bottom.depth = freeRect.depth;

	Rect3d right;
	right.x = freeRect.x + placedRect.width;
	right.y = freeRect.y;
    right.z = freeRect.z;
	right.width = freeRect.width - placedRect.width;
    right.depth = freeRect.depth;

    Rect3d up;
    up.x = freeRect.x;
    up.y = freeRect.y;
    up.z = freeRect.z + placedRect.depth;
    up.width = placedRect.width;
    up.height = placedRect.height;
    up.depth = freeRect.depth - placedRect.depth;

	if (splitHorizontal)
	{
		bottom.width = freeRect.width;
		right.height = placedRect.height;
	}
	else // Split vertically
	{
		bottom.width = placedRect.width;
		right.height = freeRect.height;
	}

	// Add the new rectangles into the free rectangle pool if they weren't degenerate.
    if (up.width > 0 && up.height > 0 && up.depth > 0)
        freeRectangles.push_back(up);
	if (bottom.width > 0 && bottom.height > 0 && bottom.depth > 0)
		freeRectangles.push_back(bottom);
	if (right.width > 0 && right.height > 0 && right.depth > 0)
		freeRectangles.push_back(right);
    
    debug_assert(disjointRects.Disjoint(up));
	debug_assert(disjointRects.Disjoint(bottom));
	debug_assert(disjointRects.Disjoint(right));
}

void GuillotineBinPack3d::MergeFreeList()
{
#ifdef _DEBUG
	DisjointRectCollection3d test;
	for(size_t i = 0; i < freeRectangles.size(); ++i)
		assert(test.Add(freeRectangles[i]) == true);
#endif

	// Do a Theta(n^2) loop to see if any pair of free rectangles could me merged into one.
	// Note that we miss any opportunities to merge three rectangles into one. (should call this function again to detect that)
	for(size_t i = 0; i < freeRectangles.size(); ++i)
		for(size_t j = i+1; j < freeRectangles.size(); ++j)
		{
			if (freeRectangles[i].width == freeRectangles[j].width && freeRectangles[i].x == freeRectangles[j].x && freeRectangles[i].z == freeRectangles[j].z && freeRectangles[i].depth == freeRectangles[j].depth)
			{
				if (freeRectangles[i].y == freeRectangles[j].y + freeRectangles[j].height)
				{
					freeRectangles[i].y -= freeRectangles[j].height;
					freeRectangles[i].height += freeRectangles[j].height;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
				else if (freeRectangles[i].y + freeRectangles[i].height == freeRectangles[j].y)
				{
					freeRectangles[i].height += freeRectangles[j].height;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
			}
			else if (freeRectangles[i].height == freeRectangles[j].height && freeRectangles[i].y == freeRectangles[j].y && freeRectangles[i].z == freeRectangles[j].z && freeRectangles[i].depth == freeRectangles[j].depth)
			{
				if (freeRectangles[i].x == freeRectangles[j].x + freeRectangles[j].width)
				{
					freeRectangles[i].x -= freeRectangles[j].width;
					freeRectangles[i].width += freeRectangles[j].width;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
				else if (freeRectangles[i].x + freeRectangles[i].width == freeRectangles[j].x)
				{
					freeRectangles[i].width += freeRectangles[j].width;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
			}
            else if(freeRectangles[i].width == freeRectangles[j].width && freeRectangles[i].height == freeRectangles[j].height && freeRectangles[i].x == freeRectangles[j].x && freeRectangles[i].y == freeRectangles[j].y){
                if (freeRectangles[i].z == freeRectangles[j].z + freeRectangles[j].depth)
				{
					freeRectangles[i].z -= freeRectangles[j].depth;
					freeRectangles[i].depth += freeRectangles[j].depth;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
				else if (freeRectangles[i].x + freeRectangles[i].depth == freeRectangles[i].x)
				{
					freeRectangles[i].depth += freeRectangles[j].depth;
					freeRectangles.erase(freeRectangles.begin() + j);
					--j;
				}
            }
		}

#ifdef _DEBUG
	test.Clear();
	for(size_t i = 0; i < freeRectangles.size(); ++i)
		assert(test.Add(freeRectangles[i]) == true);
#endif
}

}