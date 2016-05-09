#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::CollideShapeSetup
    @brief setup object for a Bullet collide shape
*/
#include "Core/Types.h"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace Oryol {

class CollideShapeSetup {
public:
    /// create as sphere
    static CollideShapeSetup Sphere(float radius) {
        CollideShapeSetup setup;
        setup.Type = SphereShape;
        setup.Values.x = radius;
        return setup;
    };
    /// create as box
    static CollideShapeSetup Box(const glm::vec3& size) {
        CollideShapeSetup setup;
        setup.Type = BoxShape;
        setup.Values = glm::vec4(size, 0.0f);
        return setup;
    };
    /// create as plane
    static CollideShapeSetup Plane(const glm::vec4& plane) {
        CollideShapeSetup setup;
        setup.Type = PlaneShape;
        setup.Values = plane;
        return setup;
    };

    enum ShapeType {
        None,
        PlaneShape,
        SphereShape,
        BoxShape,
    } Type = None;
    glm::vec4 Values;
};

} // namespace Oryol
