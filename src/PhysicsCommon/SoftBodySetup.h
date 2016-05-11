#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::SoftBodySetup
    @brief setup class for Bullet softbodies
*/
#include "Core/Types.h"
#include "Core/Containers/StaticArray.h"
#include "glm/vec3.hpp"

namespace Oryol {

class SoftBodySetup {
public:
    /// create as patch
    static SoftBodySetup Patch() {
        SoftBodySetup setup;
        setup.Points[0] = glm::vec3(-10.0f, 5.0f, -10.0f);
        setup.Points[1] = glm::vec3(+10.0f, 5.0f, -10.0f);
        setup.Points[2] = glm::vec3(-10.0f, 5.0f, +10.0f);
        setup.Points[3] = glm::vec3(+10.0f, 5.0f, +10.0f);
        return setup;
    };

    enum Type {
        RopeType,
        PatchType,

        InvalidType,
    };

    Type SoftBodyType = InvalidType;
    StaticArray<glm::vec3, 4> Points;       // rope or patch corners
    int NumSegments = 10;                   // patch x/y segments
    int Fixed = 0xF;                        // set bit for each fixed corner
};

} // namespace Oryol