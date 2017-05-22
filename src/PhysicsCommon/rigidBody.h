#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::rigidBody
    @brief physics rigid body wrapper
*/
#include "Resource/ResourceBase.h"
#include "PhysicsCommon/RigidBodySetup.h"
#include "btBulletDynamicsCommon.h"

namespace Oryol {
namespace _priv {

class collideShape;

class rigidBody : public ResourceBase<RigidBodySetup> {
public:
    /// destructor
    ~rigidBody();
    /// setup the rigid body object
    void setup(const RigidBodySetup& setup, collideShape* shape);
    /// discard the rigid body object
    void discard();

    btRigidBody* body = nullptr;
    btDefaultMotionState* motionState = nullptr;
};

} // namespace _priv
} // namespace Oryol
