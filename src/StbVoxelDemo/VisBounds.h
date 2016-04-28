#pragma once
//------------------------------------------------------------------------------
/**
    @class VisBounds
    @brief a 2D bounding area in the VisTree
*/
#include "Core/Types.h"
#include "glm/exponential.hpp"

struct VisBounds {
    /// default constructor
    VisBounds() : x0(0), x1(0), y0(0), y1(0) { };
    /// constructor
    VisBounds(int x0_, int x1_, int y0_, int y1_) : x0(x0_), x1(x1_), y0(y0_), y1(y1_) { };

    int x0, x1, y0, y1;
};