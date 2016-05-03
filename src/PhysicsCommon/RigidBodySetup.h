#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::RigidBodySetup
    @brief setup class for a physics rigid body
*/
#include "Core/Types.h"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

namespace Oryol {

class RigidBodySetup {
public:
    /// create as sphere
    RigidBodySetup Sphere(const glm::mat4& tform, float radius=1.0f, float mass=1.0f, float bounciness=0.0f);
    /// create as box
    RigidBodySetup Box(const glm::mat4& tform, const glm::vec3& size, float mass=1.0f, float bounciness=0.0f);

    enum ShapeType {
        None,
        SphereShape,
        BoxShape,
    } Type = None;
    glm::mat4 Transform;
    float Radius = 1.0f;
    float Mass = 1.0f;
    float Bounciness = 0.0f;
};

} // namespace Oryol
