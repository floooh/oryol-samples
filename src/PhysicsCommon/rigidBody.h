#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::rigidBody
    @brief physics rigid body wrapper
*/
#include "Resource/Core/resourceBase.h"
#include "PhysicsCommon/RigidBodySetup.h"
#include "btBulletDynamicsCommon.h"

namespace Oryol {
namespace _priv {

class rigidBody : resourceBase<RigidBodySetup> {
public:
    /// destructor
    ~rigidBody();
    /// setup the rigid body object
    void setup(const RigidBodySetup& setup);
    /// discard the rigid body object
    void discard();

    btCollisionShape* shape = nullptr;
    btRigidBody* body = nullptr;
    btDefaultMotionState* motionState = nullptr;
};

} // namespace _priv
} // namespace Oryol
