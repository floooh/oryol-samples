#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::collideShape
    @brief Bullet collide shape wrapper
*/
#include "Resource/ResourceBase.h"
#include "PhysicsCommon/CollideShapeSetup.h"
#include "btBulletDynamicsCommon.h"

namespace Oryol {
namespace _priv {

class collideShape : public ResourceBase {
public:
    /// destructor
    ~collideShape();
    /// setup the collide shape
    void setup(const CollideShapeSetup& setup);
    /// discard the collide shape
    void discard();

    CollideShapeSetup Setup;
    btCollisionShape* shape = nullptr;
};

} // namespace _priv
} // namespace Oryol

