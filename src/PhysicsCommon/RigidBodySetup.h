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
    /// create as sphere
    static RigidBodySetup Sphere(const glm::mat4& tform, float radius=1.0f, float mass=1.0f, float bounciness=0.0f) {
        RigidBodySetup setup;
        setup.Type = SphereShape;
        setup.Transform = tform;
        setup.Radius = radius;
        setup.Mass = mass;
        setup.Bounciness = bounciness;
        return setup;
    };
    /// create as box
    static RigidBodySetup Box(const glm::mat4& tform, const glm::vec3& size, float mass=1.0f, float bounciness=0.0f) {
        RigidBodySetup setup;
        setup.Type = BoxShape;
        setup.Transform = tform;
        setup.Size = size;
        setup.Mass = mass;
        setup.Bounciness = bounciness;
        return setup;
    };

    /// create as plane
    static RigidBodySetup Plane(const glm::vec4& plane, float bounciness=0.0f) {
        RigidBodySetup setup;
        setup.Type = PlaneShape;
        setup.PlaneValues = plane;
        setup.Mass = 0.0f;
        setup.Bounciness = bounciness;
        return setup;
    };

    enum ShapeType {
        None,
        PlaneShape,
        SphereShape,
        BoxShape,
    } Type = None;
    glm::mat4 Transform;
    glm::vec4 PlaneValues;
    glm::vec3 Size;
    float Radius = 1.0f;
    float Mass = 1.0f;
    float Bounciness = 0.0f;
};

} // namespace Oryol
