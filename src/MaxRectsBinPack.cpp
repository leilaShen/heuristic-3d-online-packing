/** @file MaxRectsBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the MAXRECTS data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include <algorithm>
#include <utility>
#include <iostream>
#include <limits>

#include <cassert>
#include <cstring>
#include <cmath>

#include "../include/MaxRectsBinPack.h"

namespace rbp {

using namespace std;

MaxRectsBinPack::MaxRectsBinPack()
:binWidth(0),
binHeight(0),
binDepth(0)
{
}

MaxRectsBinPack::MaxRectsBinPack(int width, int height, int depth, bool allowFlip)
{
	Init(width, height, depth, allowFlip);
}

void MaxRectsBinPack::Init(int width, int height, int depth, bool allowFlip)
{
	binAllowFlip = allowFlip;
	binWidth = width;
	binHeight = height;
    binDepth = depth;	

	FreeRect3d n;
	n.x = 0;
	n.y = 0;
	n.z = 0;
	n.width = width;
	n.height = height;
	n.depth = depth;
	n.supportx0 = 0;
	n.supportx1 = width;
	n.supporty0 = 0;
	n.supporty1 = height;

	usedRectangles.clear();
	freeRectangles.clear();
	freeRectangles.push_back(n);
}

Rect3d MaxRectsBinPack::Insert(int width, int height, int depth, FreeRectChoiceHeuristic method)
{
	Rect3d newNode;
	// Unused in this function. We don't need to know the score after finding the position.
	int score1 = std::numeric_limits<int>::max();
	int score2 = std::numeric_limits<int>::max();
	int score3 = std::numeric_limits<int>::max();
	this->sortFreeSpace();
	switch(method)
	{
		//case RectBestShortSideFit: newNode = FindPositionForNewNodeBestShortSideFit(width, height, score1, score2); break;
		case RectBottomLeftRule: newNode = FindPositionForNewNodeBottomLeft(width, height, depth, score1, score2, score3); break;
		//case RectContactPointRule: newNode = FindPositionForNewNodeContactPoint(width, height, score1); break;
		//case RectBestLongSideFit: newNode = FindPositionForNewNodeBestLongSideFit(width, height, score2, score1); break;
		//case RectBestAreaFit: newNode = FindPositionForNewNodeBestAreaFit(width, height, score1, score2); break;
	}
		
	if (newNode.height == 0)
		return newNode;

	size_t numRectanglesToProcess = freeRectangles.size();
	for(size_t i = 0; i < numRectanglesToProcess; ++i)
	{
		if (SplitFreeNode(freeRectangles[i], newNode))
		{
			freeRectangles.erase(freeRectangles.begin() + i);
			--i;
			--numRectanglesToProcess;
		}
	}

	PruneFreeList();

	usedRectangles.push_back(newNode);
	return newNode;
}



/// Computes the ratio of used surface area.
float MaxRectsBinPack::Occupancy() const
{
	unsigned long usedSurfaceArea = 0;
	for(size_t i = 0; i < usedRectangles.size(); ++i)
		usedSurfaceArea += usedRectangles[i].width * usedRectangles[i].height;

	return (float)usedSurfaceArea / (binWidth * binHeight);
}

void MaxRectsBinPack::sortFreeSpace(){
	std::sort(freeRectangles.begin(), freeRectangles.end(), [](FreeRect3d& r1, FreeRect3d& r2)->bool{
		if(r1.y < r2.y){
			return true;
		}
		if(r1.y == r2.y && r1.z < r2.z){
			return true;
		}
		if(r1.y == r2.y && r1.z == r2.z && r1.x < r2.x){
			return true;
		}
		return false;
	});
}

bool MaxRectsBinPack::isBlocked(const Rect3d& usedRect, const Rect3d& newNode) const{
	//check if intersect in XOY plane
	if (newNode.x < usedRect.x + usedRect.width && usedRect.x < newNode.x + newNode.width && newNode.y < usedRect.y + usedRect.height && usedRect.y < newNode.y + newNode.height){
		return newNode.z < usedRect.z + usedRect.depth;
	}
	return false;
}

Rect3d MaxRectsBinPack::FindPositionForNewNodeBottomLeft(int width, int height, int depth, int &bestY, int &bestX, int& bestZ) const
{
	Rect3d bestNode;
	memset(&bestNode, 0, sizeof(Rect3d));

	bestY = std::numeric_limits<int>::max();
	bestX = std::numeric_limits<int>::max();
	bestZ = std::numeric_limits<int>::max();	

    bool blocked = false;
	for(size_t i = 0; i < freeRectangles.size(); ++i)
	{	
		int supportWidth = freeRectangles[i].supportx1 - freeRectangles[i].supportx0;		
		int supportHeight = freeRectangles[i].supporty1 - freeRectangles[i].supporty0;
		printFreeRect(std::string("free space:")+std::to_string(i), freeRectangles[i]);
		// Try to place the rectangle in upright (non-flipped) orientation.
		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height && freeRectangles[i].depth >= depth && supportHeight >= height * supportTh && supportWidth >= width*supportTh)
		{
			bestNode.x = freeRectangles[i].supportx0;
			bestNode.y = freeRectangles[i].supporty0;
			bestNode.z = freeRectangles[i].z;
			bestNode.width = width;
			bestNode.height = height;
			bestNode.depth = depth;
			bestY = bestNode.y + height;
			bestX = bestNode.x;
			bestZ = bestNode.z;			
			for(size_t j = 0; j < usedRectangles.size(); ++j){
				if(isBlocked(usedRectangles[j], bestNode)){
					blocked = true;
					break;
				}
			}
			if(blocked == false){
				return bestNode;
			}
		}
		if (binAllowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width && freeRectangles[i].depth >= depth && supportHeight >= width * supportTh && supportWidth >= height*supportTh)
		{	
			bestNode.x = freeRectangles[i].supportx0;
			bestNode.y = freeRectangles[i].supporty0;
			bestNode.z = freeRectangles[i].z;
			bestNode.width = height;
			bestNode.height = width;
			bestNode.depth = depth;
			bestY = bestNode.y + width;
			bestX = bestNode.x;
			bestZ = bestNode.z;
			blocked = false;
			for(size_t j = 0; j < usedRectangles.size(); ++j){
				if(isBlocked(usedRectangles[j], bestNode)){
					blocked = true;
					break;
				}
			}
			if(blocked == false){
				return bestNode;
			}
		}
	}
	memset(&bestNode, 0, sizeof(Rect3d));
	return bestNode;
}

// Rect MaxRectsBinPack::FindPositionForNewNodeBestShortSideFit(int width, int height, 
// 	int &bestShortSideFit, int &bestLongSideFit) const
// {
// 	Rect bestNode;
// 	memset(&bestNode, 0, sizeof(Rect));

// 	bestShortSideFit = std::numeric_limits<int>::max();
// 	bestLongSideFit = std::numeric_limits<int>::max();

// 	for(size_t i = 0; i < freeRectangles.size(); ++i)
// 	{
// 		// Try to place the rectangle in upright (non-flipped) orientation.
// 		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
// 		{
// 			int leftoverHoriz = abs(freeRectangles[i].width - width);
// 			int leftoverVert = abs(freeRectangles[i].height - height);
// 			int shortSideFit = min(leftoverHoriz, leftoverVert);
// 			int longSideFit = max(leftoverHoriz, leftoverVert);

// 			if (shortSideFit < bestShortSideFit || (shortSideFit == bestShortSideFit && longSideFit < bestLongSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = width;
// 				bestNode.height = height;
// 				bestShortSideFit = shortSideFit;
// 				bestLongSideFit = longSideFit;
// 			}
// 		}

// 		if (binAllowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
// 		{
// 			int flippedLeftoverHoriz = abs(freeRectangles[i].width - height);
// 			int flippedLeftoverVert = abs(freeRectangles[i].height - width);
// 			int flippedShortSideFit = min(flippedLeftoverHoriz, flippedLeftoverVert);
// 			int flippedLongSideFit = max(flippedLeftoverHoriz, flippedLeftoverVert);

// 			if (flippedShortSideFit < bestShortSideFit || (flippedShortSideFit == bestShortSideFit && flippedLongSideFit < bestLongSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = height;
// 				bestNode.height = width;
// 				bestShortSideFit = flippedShortSideFit;
// 				bestLongSideFit = flippedLongSideFit;
// 			}
// 		}
// 	}
// 	return bestNode;
// }

// Rect MaxRectsBinPack::FindPositionForNewNodeBestLongSideFit(int width, int height, 
// 	int &bestShortSideFit, int &bestLongSideFit) const
// {
// 	Rect bestNode;
// 	memset(&bestNode, 0, sizeof(Rect));

// 	bestShortSideFit = std::numeric_limits<int>::max();
// 	bestLongSideFit = std::numeric_limits<int>::max();

// 	for(size_t i = 0; i < freeRectangles.size(); ++i)
// 	{
// 		// Try to place the rectangle in upright (non-flipped) orientation.
// 		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
// 		{
// 			int leftoverHoriz = abs(freeRectangles[i].width - width);
// 			int leftoverVert = abs(freeRectangles[i].height - height);
// 			int shortSideFit = min(leftoverHoriz, leftoverVert);
// 			int longSideFit = max(leftoverHoriz, leftoverVert);

// 			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = width;
// 				bestNode.height = height;
// 				bestShortSideFit = shortSideFit;
// 				bestLongSideFit = longSideFit;
// 			}
// 		}

// 		if (binAllowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
// 		{
// 			int leftoverHoriz = abs(freeRectangles[i].width - height);
// 			int leftoverVert = abs(freeRectangles[i].height - width);
// 			int shortSideFit = min(leftoverHoriz, leftoverVert);
// 			int longSideFit = max(leftoverHoriz, leftoverVert);

// 			if (longSideFit < bestLongSideFit || (longSideFit == bestLongSideFit && shortSideFit < bestShortSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = height;
// 				bestNode.height = width;
// 				bestShortSideFit = shortSideFit;
// 				bestLongSideFit = longSideFit;
// 			}
// 		}
// 	}
// 	return bestNode;
// }

// Rect MaxRectsBinPack::FindPositionForNewNodeBestAreaFit(int width, int height, 
// 	int &bestAreaFit, int &bestShortSideFit) const
// {
// 	Rect bestNode;
// 	memset(&bestNode, 0, sizeof(Rect));

// 	bestAreaFit = std::numeric_limits<int>::max();
// 	bestShortSideFit = std::numeric_limits<int>::max();

// 	for(size_t i = 0; i < freeRectangles.size(); ++i)
// 	{
// 		int areaFit = freeRectangles[i].width * freeRectangles[i].height - width * height;

// 		// Try to place the rectangle in upright (non-flipped) orientation.
// 		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
// 		{
// 			int leftoverHoriz = abs(freeRectangles[i].width - width);
// 			int leftoverVert = abs(freeRectangles[i].height - height);
// 			int shortSideFit = min(leftoverHoriz, leftoverVert);

// 			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = width;
// 				bestNode.height = height;
// 				bestShortSideFit = shortSideFit;
// 				bestAreaFit = areaFit;
// 			}
// 		}

// 		if (binAllowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
// 		{
// 			int leftoverHoriz = abs(freeRectangles[i].width - height);
// 			int leftoverVert = abs(freeRectangles[i].height - width);
// 			int shortSideFit = min(leftoverHoriz, leftoverVert);

// 			if (areaFit < bestAreaFit || (areaFit == bestAreaFit && shortSideFit < bestShortSideFit))
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = height;
// 				bestNode.height = width;
// 				bestShortSideFit = shortSideFit;
// 				bestAreaFit = areaFit;
// 			}
// 		}
// 	}
// 	return bestNode;
// }

/// Returns 0 if the two intervals i1 and i2 are disjoint, or the length of their overlap otherwise.
int CommonIntervalLength(int i1start, int i1end, int i2start, int i2end)
{
	if (i1end < i2start || i2end < i1start)
		return 0;
	return min(i1end, i2end) - max(i1start, i2start);
}

// int MaxRectsBinPack::ContactPointScoreNode(int x, int y, int width, int height) const
// {
// 	int score = 0;

// 	if (x == 0 || x + width == binWidth)
// 		score += height;
// 	if (y == 0 || y + height == binHeight)
// 		score += width;

// 	for(size_t i = 0; i < usedRectangles.size(); ++i)
// 	{
// 		if (usedRectangles[i].x == x + width || usedRectangles[i].x + usedRectangles[i].width == x)
// 			score += CommonIntervalLength(usedRectangles[i].y, usedRectangles[i].y + usedRectangles[i].height, y, y + height);
// 		if (usedRectangles[i].y == y + height || usedRectangles[i].y + usedRectangles[i].height == y)
// 			score += CommonIntervalLength(usedRectangles[i].x, usedRectangles[i].x + usedRectangles[i].width, x, x + width);
// 	}
// 	return score;
// }

// Rect MaxRectsBinPack::FindPositionForNewNodeContactPoint(int width, int height, int &bestContactScore) const
// {
// 	Rect bestNode;
// 	memset(&bestNode, 0, sizeof(Rect));

// 	bestContactScore = -1;

// 	for(size_t i = 0; i < freeRectangles.size(); ++i)
// 	{
// 		// Try to place the rectangle in upright (non-flipped) orientation.
// 		if (freeRectangles[i].width >= width && freeRectangles[i].height >= height)
// 		{
// 			int score = ContactPointScoreNode(freeRectangles[i].x, freeRectangles[i].y, width, height);
// 			if (score > bestContactScore)
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = width;
// 				bestNode.height = height;
// 				bestContactScore = score;
// 			}
// 		}
// 		if (binAllowFlip && freeRectangles[i].width >= height && freeRectangles[i].height >= width)
// 		{
// 			int score = ContactPointScoreNode(freeRectangles[i].x, freeRectangles[i].y, height, width);
// 			if (score > bestContactScore)
// 			{
// 				bestNode.x = freeRectangles[i].x;
// 				bestNode.y = freeRectangles[i].y;
// 				bestNode.width = height;
// 				bestNode.height = width;
// 				bestContactScore = score;
// 			}
// 		}
// 	}
// 	return bestNode;
// }

bool MaxRectsBinPack::SplitFreeNode(FreeRect3d freeNode, const Rect3d& usedNode)
{	
	printFreeRect("freeNode:",freeNode);	
	printRect("usedNode:", usedNode);
	// Test with SAT if the rectangles even intersect.
	if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
		usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y || 
		usedNode.z >= freeNode.z + freeNode.depth || usedNode.z + usedNode.depth <= freeNode.z)
		return false;

#ifdef DEBUG_BIN_PACK
	std::cout << "enter into space cutting...." << std::endl;
#endif	
	
	// New node at the top side of the used node. cut space along xoz plane
	if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height)
	{
		FreeRect3d newNode = freeNode;
		newNode.height = usedNode.y - newNode.y;
		newNode.supporty1 = min(freeNode.supporty1, usedNode.y);
		
		printFreeRect("cut space along xoz....................",newNode);

		freeRectangles.push_back(newNode);
	}
    
	// New node at the bottom side of the used node. cut space along xoz plane
	if (usedNode.y + usedNode.height < freeNode.y + freeNode.height)
	{	
		FreeRect3d newNode = freeNode;	
		newNode.y = usedNode.y + usedNode.height;
		newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);		
	    newNode.supporty0 = max(freeNode.supporty0, newNode.y);

		printFreeRect("cut space along xoz................", newNode);        
		
		freeRectangles.push_back(newNode);
		
	}
    
	// New node at the left side of the used node. cut space along zoy plane
	if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width)
	{
		FreeRect3d newNode = freeNode;
		newNode.width = usedNode.x - newNode.x;
		newNode.supportx1 = min(freeNode.supportx1, usedNode.x);
    
 		printFreeRect("cut space along yoz...............", newNode);

		freeRectangles.push_back(newNode);
	}    
	
	// New node at the right side of the used node. cut space along zoy plane
	if (usedNode.x + usedNode.width < freeNode.x + freeNode.width)
	{
		FreeRect3d newNode = freeNode;
		newNode.x = usedNode.x + usedNode.width;
		newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
		newNode.supportx0 = max(freeNode.supportx0, newNode.x);
        
		printFreeRect("cut space along yoz............", newNode);

		freeRectangles.push_back(newNode);
	}
    
	// New node at bottom of the used node. cut space along xoy plane
	if(usedNode.z > freeNode.z && usedNode.z < freeNode.z + freeNode.depth){
		FreeRect3d newNode = freeNode;
		newNode.depth = usedNode.z - newNode.z;
		
		printFreeRect("cut space along xoy...........", newNode);

		freeRectangles.push_back(newNode);
	}

	// New node at top of the used node. cut space along xoy plane
	if(usedNode.z + usedNode.depth < freeNode.z + freeNode.depth){
		FreeRect3d newNode = freeNode;
		newNode.z = usedNode.z + usedNode.depth;
		newNode.depth = freeNode.z + freeNode.depth - newNode.z;
		newNode.supportx0 = usedNode.x;
		newNode.supportx1 = usedNode.x + usedNode.width;
		newNode.supporty0 = usedNode.y;
		newNode.supporty1 = usedNode.y + usedNode.height;				
		printFreeRect("cut space along xoy.................",newNode);
		freeRectangles.push_back(newNode);
	}	
	return true;
}

void MaxRectsBinPack::PruneFreeList()
{
	/* 
	///  Would be nice to do something like this, to avoid a Theta(n^2) loop through each pair.
	///  But unfortunately it doesn't quite cut it, since we also want to detect containment. 
	///  Perhaps there's another way to do this faster than Theta(n^2).

	if (freeRectangles.size() > 0)
		clb::sort::QuickSort(&freeRectangles[0], freeRectangles.size(), NodeSortCmp);

	for(size_t i = 0; i < freeRectangles.size()-1; ++i)
		if (freeRectangles[i].x == freeRectangles[i+1].x &&
		    freeRectangles[i].y == freeRectangles[i+1].y &&
		    freeRectangles[i].width == freeRectangles[i+1].width &&
		    freeRectangles[i].height == freeRectangles[i+1].height)
		{
			freeRectangles.erase(freeRectangles.begin() + i);
			--i;
		}
	*/

	/// Go through each pair and remove any rectangle that is redundant.
	for(size_t i = 0; i < freeRectangles.size(); ++i)
		for(size_t j = i+1; j < freeRectangles.size(); ++j)
		{
			if (IsContainedInFree3d(freeRectangles[i], freeRectangles[j]))
			{
				freeRectangles.erase(freeRectangles.begin()+i);
				--i;
				break;
			}
			if (IsContainedInFree3d(freeRectangles[j], freeRectangles[i]))
			{
				freeRectangles.erase(freeRectangles.begin()+j);
				--j;
			}
		}
}

}
