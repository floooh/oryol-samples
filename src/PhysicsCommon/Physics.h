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
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "Resource/ResourcePool.h"
#include "PhysicsCommon/CollideShapeSetup.h"
#include "PhysicsCommon/collideShape.h"
#include "PhysicsCommon/RigidBodySetup.h"
#include "PhysicsCommon/rigidBody.h"
#include "PhysicsCommon/SoftBodySetup.h"
#include "PhysicsCommon/softBody.h"

namespace Oryol {

class Physics {
public:
    /// setup the physics world
    static void Setup();
    /// discard the physics world
    static void Discard();
    /// update the physics world
    static void Update(float frameDuration);

    /// create a collision shape object
    static Id Create(const CollideShapeSetup& setup);
    /// create a rigid body object
    static Id Create(const RigidBodySetup& setup);
    /// create a soft body object
    static Id Create(const SoftBodySetup& setup);
    /// destroy object
    static void Destroy(Id id);
    /// add object to world
    static void Add(Id id);
    /// remove object from world
    static void Remove(Id id);

    /// get world-space transform of a rigid body object
    static glm::mat4 Transform(Id id);
    /// get collide shape pointer by id
    static btCollisionShape* Shape(Id id);
    /// get rigid body pointer by id
    static btRigidBody* RigidBody(Id id);
    /// get soft body pointer by id
    static btSoftBody* SoftBody(Id id);
    /// get a rigid body's collide shape type
    static CollideShapeSetup::ShapeType RigidBodyShapeType(Id rigidBodyId);

    /// get pointer to Bullet dynamics world
    static btDynamicsWorld* World() {
        o_assert_dbg(state && state->dynamicsWorld);
        return state->dynamicsWorld;
    };

private:
    static const Id::TypeT RigidBodyType = 1;
    static const Id::TypeT SoftBodyType = 2;
    static const Id::TypeT CollideShapeType = 3;
    struct _state {
        btBroadphaseInterface* broadphase = nullptr;
        btSoftBodyRigidBodyCollisionConfiguration* collisionConfiguration = nullptr;
        btCollisionDispatcher* dispatcher = nullptr;
        btSequentialImpulseConstraintSolver* solver = nullptr;
        btDefaultSoftBodySolver* softBodySolver = nullptr;
        btSoftRigidDynamicsWorld* dynamicsWorld = nullptr;
        btSoftBodyWorldInfo softBodyWorldInfo;
        ResourcePool<_priv::collideShape> shapePool;
        ResourcePool<_priv::rigidBody> rigidBodyPool;
        ResourcePool<_priv::softBody> softBodyPool;
    };
    static _state* state;
};

//------------------------------------------------------------------------------
inline btRigidBody*
Physics::RigidBody(Id id) {
    o_assert_dbg(RigidBodyType == id.Type);
    _priv::rigidBody* body = state->rigidBodyPool.Get(id);
    if (body) {
        return body->body;
    }
    else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
inline btSoftBody*
Physics::SoftBody(Id id) {
    o_assert_dbg(SoftBodyType == id.Type);
    _priv::softBody* body = state->softBodyPool.Get(id);
    if (body) {
        return body->body;
    }
    else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
inline CollideShapeSetup::ShapeType
Physics::RigidBodyShapeType(Id id) {
    o_assert_dbg(RigidBodyType == id.Type);
    _priv::rigidBody* body = state->rigidBodyPool.Get(id);
    if (body) {
        _priv::collideShape* shape = state->shapePool.Get(body->Setup.Shape);
        if (shape) {
            return shape->Setup.Type;
        }
    }
    return CollideShapeSetup::None;
}

} // namespace Oryol
