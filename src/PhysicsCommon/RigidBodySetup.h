#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::RigidBodySetup
    @brief setup class for a physics rigid body
*/
#include "Core/Types.h"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Oryol {

class RigidBodySetup {
public:
    /// create from collide shape
    static RigidBodySetup FromShape(Id shape, const glm::mat4& tform=glm::mat4(1.0f), float mass=1.0f, float bounciness=0.0f) {
        RigidBodySetup setup;
        setup.Shape = shape;
        setup.Transform = tform;
        setup.Mass = mass;
        setup.Bounciness = bounciness;
        return setup;
    };
    Id Shape;
    glm::mat4 Transform = glm::mat4(1.0f);
    float Mass = 1.0f;
    float Bounciness = 0.0f;
};

} // namespace Oryol
