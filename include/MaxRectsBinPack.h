/** @file MaxRectsBinPack.h
	@author Jukka Jyl�nki

	@brief Implements different bin packer algorithms that use the MAXRECTS data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <vector>

#include "Rect3d.h"
#include <iostream>
#define DEBUG_BIN_PACK

namespace rbp {

/** MaxRectsBinPack implements the MAXRECTS data structure and different bin packing algorithms that 
	use this structure. */
class MaxRectsBinPack
{
public:
	/// Instantiates a bin of size (0,0). Call Init to create a new bin.
	MaxRectsBinPack();

	/// Instantiates a bin of the given size.
	/// @param allowFlip Specifies whether the packing algorithm is allowed to rotate the input rectangles by 90 degrees to consider a better placement.
	MaxRectsBinPack(int width, int height, int depth, bool allowFlip = true);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int width, int height, int depth, bool allowFlip = true);

	/// Specifies the different heuristic rules that can be used when deciding where to place a new rectangle.
	enum FreeRectChoiceHeuristic
	{
		RectBestShortSideFit, ///< -BSSF: Positions the rectangle against the short side of a free rectangle into which it fits the best.
		RectBestLongSideFit, ///< -BLSF: Positions the rectangle against the long side of a free rectangle into which it fits the best.
		RectBestAreaFit, ///< -BAF: Positions the rectangle into the smallest free rect into which it fits.
		RectBottomLeftRule, ///< -BL: Does the Tetris placement.
		RectContactPointRule ///< -CP: Choosest the placement where the rectangle touches other rects as much as possible.
	};

	/// Inserts the given list of rectangles in an offline/batch mode, possibly rotated.
	/// @param rects The list of rectangles to insert. This vector will be destroyed in the process.
	/// @param dst [out] This list will contain the packed rectangles. The indices will not correspond to that of rects.
	/// @param method The rectangle placement rule to use when packing.
	//void Insert(std::vector<RectSize> &rects, std::vector<Rect> &dst, FreeRectChoiceHeuristic method);

	/// Inserts a single rectangle into the bin, possibly rotated.
	Rect3d Insert(int width, int height, int depth, FreeRectChoiceHeuristic method);

	/// Computes the ratio of used surface area to the total bin area.
	float Occupancy() const;

private:
	int binWidth;
	int binHeight;
	int binDepth;
	int supportTh = 0.8f;

	bool binAllowFlip;

	std::vector<Rect3d> usedRectangles;
	std::vector<FreeRect3d> freeRectangles;

	
	/// Computes the placement score for the -CP variant.
	int ContactPointScoreNode(int x, int y, int z, int width, int height, int depth) const;

	Rect3d FindPositionForNewNodeBottomLeft(int width, int height, int depth, int &bestY, int &bestX, int& bestZ) const;
	// Rect FindPositionForNewNodeBestShortSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit) const;
	// Rect FindPositionForNewNodeBestLongSideFit(int width, int height, int &bestShortSideFit, int &bestLongSideFit) const;
	// Rect FindPositionForNewNodeBestAreaFit(int width, int height, int &bestAreaFit, int &bestShortSideFit) const;
	// Rect FindPositionForNewNodeContactPoint(int width, int height, int &contactScore) const;

	/// @return True if the free node was split.
	bool SplitFreeNode(FreeRect3d freeNode, const Rect3d &usedNode);

    // sort free rectangles in deepest-bottom-left order, that is y-z-x (or x-z-y in some case)
	void sortFreeSpace();
    
	//check if place node is blocked by used rect
	bool isBlocked(const Rect3d& usedRect, const Rect3d& newNode) const;

	/// Goes through the free rectangle list and removes any redundant entries.
	void PruneFreeList();

	//for debug purpose
	inline void printFreeRect(const std::string& indicator, const FreeRect3d& r) const{	
	#ifdef DEBUG_BIN_PACK
		std::cout << indicator << std::endl;
		std::cout << "x:" << r.x << " y:" << r.y << " z:" << r.z << " size:" << r.width << "X" << r.height << "X" << r.depth
		<< "  support:" << "x " << r.supportx0 << "~" << r.supportx1 << " y " << r.supporty0 << "~" << r.supporty1 << std::endl;	
	#endif
	};

	//for debug purpose
	inline void printRect(const std::string& indicator, const Rect3d& r) const{	
	#ifdef DEBUG_BIN_PACK
	    std::cout << indicator << std::endl;
		std::cout << "x:" << r.x << " y:" << r.y << " z:" << r.z << " size:" << r.width << "X" << r.height << "X" << r.depth << std::endl;	
	#endif
	}
};

}
