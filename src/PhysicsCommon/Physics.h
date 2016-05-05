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
#include "PhysicsCommon/rigidBody.h"

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
    static void Destroy(Id id);
    /// add object to world
    static void Add(Id id);
    /// remove object from world
    static void Remove(Id id);

    /// get world-space transform of a rigid body object
    static glm::mat4 Transform(Id id);
    /// get rigid body pointer by id
    static btRigidBody* Body(Id id);
    /// get type of rigid body object
    static RigidBodySetup::ShapeType ShapeType(Id id);

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

//------------------------------------------------------------------------------
inline btRigidBody*
Physics::Body(Id id) {
    _priv::rigidBody* body = state->rigidBodyPool.Get(id);
    if (body) {
        return body->body;
    }
    else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
inline RigidBodySetup::ShapeType
Physics::ShapeType(Id id) {
    _priv::rigidBody* body = state->rigidBodyPool.Get(id);
    if (body) {
        return body->Setup.Type;
    }
    else {
        return RigidBodySetup::None;
    }
}

} // namespace Oryol