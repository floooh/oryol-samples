#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics
    @brief helper class for managing a Bullet physics world
*/
#include "Core/Types.h"
#include "Resource/Id.h"
#include "Core/Time/Duration.h"
#include "Core/Assertion.h"
#include "btBulletDynamicsCommon.h"
#include "Resource/Core/ResourcePool.h"
#include "PhysicsCommon/RigidBodySetup.h"
#include "PhysicsCommon/RigidBody.h"

namespace Oryol {

class Physics {
public:
    /// setup the physics world
    static void Setup();
    /// discard the physics world
    static void Discard();
    /// update the physics world
    static void Update(float frameDuration);

    /// create a physics object
    static Id Create(const RigidBodySetup& setup);
    /// destroy object
    static void Destroy(Id obj);
    /// add object to world
    static void Add(Id obj);
    /// remove object from world
    static void Remove(Id obj);

    /// get world-space transform of a rigid body object
    static glm::mat4 Transform(Id obj);

    /// get pointer to Bullet dynamics world
    static btDynamicsWorld* World() {
        o_assert_dbg(state && state->dynamicsWorld);
        return state->dynamicsWorld;
    };

private:
    struct _state {
        btBroadphaseInterface* broadphase = nullptr;
        btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
        btCollisionDispatcher* dispatcher = nullptr;
        btSequentialImpulseConstraintSolver* solver = nullptr;
        btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
        ResourcePool<_priv::rigidBody, RigidBodySetup> rigidBodyPool;
    };
    static _state* state;
};

} // namespace Oryol