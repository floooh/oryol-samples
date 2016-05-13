//------------------------------------------------------------------------------
//  Physics.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Physics.h"
#include "Core/Memory/Memory.h"
#include "glm/gtc/type_ptr.hpp"

namespace Oryol {

using namespace _priv;

Physics::_state* Physics::state = nullptr;

//------------------------------------------------------------------------------
void
Physics::Setup() {
    o_assert(nullptr == state);
    state = Memory::New<_state>();

    btVector3 min(-500.0f, -500.0, -500.0f);
    btVector3 max(500.0f, 500.0, 500.0f);
    state->broadphase = new btAxisSweep3(min, max, 32766);
    state->collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
    state->dispatcher = new btCollisionDispatcher(state->collisionConfiguration);
    state->solver = new btSequentialImpulseConstraintSolver();
    state->softBodySolver = new btDefaultSoftBodySolver();
    state->dynamicsWorld = new btSoftRigidDynamicsWorld(state->dispatcher,
        state->broadphase, state->solver, state->collisionConfiguration,
        state->softBodySolver);
    state->softBodyWorldInfo.m_dispatcher = state->dispatcher;
    state->softBodyWorldInfo.m_broadphase = state->broadphase;
    state->softBodyWorldInfo.m_sparsesdf.Initialize();

    state->shapePool.Setup(CollideShapeType, 128);
    state->rigidBodyPool.Setup(RigidBodyType, 1024);
    state->softBodyPool.Setup(SoftBodyType, 16);
}

//------------------------------------------------------------------------------
void
Physics::Discard() {
    o_assert(nullptr != state);

    state->softBodyPool.Discard();
    state->rigidBodyPool.Discard();
    state->shapePool.Discard();

    delete state->dynamicsWorld;
    delete state->softBodySolver;
    delete state->solver;
    delete state->dispatcher;
    delete state->collisionConfiguration;
    delete state->broadphase;
    Memory::Delete(state);
}

//------------------------------------------------------------------------------
void
Physics::Update(float frameDuration) {
    o_assert_dbg(nullptr != state);
    state->dynamicsWorld->stepSimulation(frameDuration, 10);
    state->rigidBodyPool.Update();
    state->softBodyPool.Update();
    state->shapePool.Update();
}

//------------------------------------------------------------------------------
Id
Physics::Create(const CollideShapeSetup& setup) {
    o_assert_dbg(nullptr != state);
    Id id = state->shapePool.AllocId();
    collideShape& shape = state->shapePool.Assign(id, setup, ResourceState::Valid);
    shape.setup(setup);
    return id;
}

//------------------------------------------------------------------------------
Id
Physics::Create(const RigidBodySetup& setup) {
    o_assert_dbg(nullptr != state);
    Id id = state->rigidBodyPool.AllocId();
    rigidBody& body = state->rigidBodyPool.Assign(id, setup, ResourceState::Valid);
    collideShape* shape = state->shapePool.Get(setup.Shape);
    o_assert(shape);
    body.setup(setup, shape);
    return id;
}

//------------------------------------------------------------------------------
Id
Physics::Create(const SoftBodySetup& setup) {
    o_assert_dbg(nullptr != state);
    Id id = state->softBodyPool.AllocId();
    softBody& body = state->softBodyPool.Assign(id, setup, ResourceState::Valid);
    body.setup(setup, state->softBodyWorldInfo);
    return id;
}

//------------------------------------------------------------------------------
void
Physics::Destroy(Id id) {
    o_assert_dbg(nullptr != state);
    switch (id.Type) {
        case RigidBodyType:
            {
                rigidBody* body = state->rigidBodyPool.Get(id);
                if (body) {
                    body->discard();
                    state->rigidBodyPool.Unassign(id);
                }
            }
            break;

        case SoftBodyType:
            {
                softBody* body = state->softBodyPool.Get(id);
                if (body) {
                    body->discard();
                    state->softBodyPool.Unassign(id);
                }
            }
            break;

        case CollideShapeType:
            {
                collideShape* shape = state->shapePool.Get(id);
                if (shape) {
                    shape->discard();
                    state->shapePool.Unassign(id);
                }
            }
            break;
    }
}

//------------------------------------------------------------------------------
void
Physics::Add(Id id) {
    o_assert_dbg(nullptr != state);
    if (RigidBodyType == id.Type) {
        rigidBody* body = state->rigidBodyPool.Get(id);
        if (body) {
            o_assert_dbg(body->body);
            state->dynamicsWorld->addRigidBody(body->body);
        }
    }
    else if (SoftBodyType == id.Type) {
        softBody* body = state->softBodyPool.Get(id);
        if (body) {
            o_assert_dbg(body->body);
            state->dynamicsWorld->addSoftBody(body->body);
        }
    }
}

//------------------------------------------------------------------------------
void
Physics::Remove(Id id) {
    o_assert_dbg(nullptr != state);
    if (RigidBodyType == id.Type) {
        rigidBody* body = state->rigidBodyPool.Get(id);
        if (body) {
            o_assert_dbg(body->body);
            state->dynamicsWorld->removeRigidBody(body->body);
        }
    }
    else if (SoftBodyType == id.Type) {
        softBody* body = state->softBodyPool.Get(id);
        if (body) {
            o_assert_dbg(body->body);
            state->dynamicsWorld->removeSoftBody(body->body);
        }
    }
}

//------------------------------------------------------------------------------
glm::mat4
Physics::Transform(Id id) {
    o_assert_dbg(nullptr != state);
    o_assert_dbg(RigidBodyType == id.Type);
    glm::mat4 result;
    rigidBody* body = state->rigidBodyPool.Get(id);
    if (body) {
        o_assert_dbg(body->motionState);
        btTransform tform;
        body->motionState->getWorldTransform(tform);
        btScalar m[16];
        tform.getOpenGLMatrix(m);
        result = glm::make_mat4(m);
    }
    return result;
}

} // namespace Oryol




