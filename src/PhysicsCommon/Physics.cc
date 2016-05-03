//------------------------------------------------------------------------------
//  Physics.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Physics.h"
#include "Core/Memory/Memory.h"

namespace Oryol {

Physics::_state* Physics::state = nullptr;

//------------------------------------------------------------------------------
void
Physics::Setup() {
    o_assert(nullptr == state);
    state = Memory::New<_state>();

    state->broadphase = new btDbvtBroadphase();
    state->collisionConfiguration = new btDefaultCollisionConfiguration();
    state->dispatcher = new btCollisionDispatcher(state->collisionConfiguration);
    state->solver = new btSequentialImpulseConstraintSolver();
    state->dynamicsWorld = new btDiscreteDynamicsWorld(state->dispatcher,
        state->broadphase, state->solver, state->collisionConfiguration);

    state->rigidBodyPool.Setup(0, 1024);
}

//------------------------------------------------------------------------------
void
Physics::Discard() {
    o_assert(nullptr != state);

    state->rigidBodyPool.Discard();

    delete state->dynamicsWorld;
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
}

} // namespace Oryol




