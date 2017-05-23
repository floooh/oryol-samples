//------------------------------------------------------------------------------
//  collideShape.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "collideShape.h"
#include "Core/Assertion.h"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
collideShape::~collideShape() {
    o_assert_dbg(!this->shape);
}

//------------------------------------------------------------------------------
void
collideShape::setup(const CollideShapeSetup& setup) {
    o_assert_dbg(!this->shape);
    this->Setup = setup;
    switch (setup.Type) {
        case CollideShapeSetup::PlaneShape:
            this->shape = new btStaticPlaneShape(
                btVector3(setup.Values.x,
                          setup.Values.y,
                          setup.Values.z),
                setup.Values.w);
            break;

        case CollideShapeSetup::SphereShape:
            this->shape = new btSphereShape(setup.Values.x);
            break;

        case CollideShapeSetup::BoxShape:
            this->shape = new btBoxShape(
                btVector3(setup.Values.x * 0.5f,
                          setup.Values.y * 0.5f,
                          setup.Values.z * 0.5f));
            break;

        default:
            o_assert(false);
            break;
    }
}

//------------------------------------------------------------------------------
void
collideShape::discard() {
    if (this->shape) {
        delete this->shape;
        this->shape = nullptr;
    }
}

} // namespace _priv
} // namespace Oryol
