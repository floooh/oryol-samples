//------------------------------------------------------------------------------
//  rigidBody.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "rigidBody.h"
#include "collideShape.h"
#include "Core/Assertion.h"
#include "glm/gtc/type_ptr.hpp"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
rigidBody::~rigidBody() {
    o_assert_dbg(!this->body);
    o_assert_dbg(!this->motionState);
}

//------------------------------------------------------------------------------
void
rigidBody::setup(const RigidBodySetup& setup, collideShape* shape) {
    o_assert_dbg(!this->body);
    o_assert_dbg(!this->motionState);
    o_assert_dbg(shape);
    this->Setup = setup;

    // transform 4x4 transform to btTransform
    btTransform tform;
    tform.setFromOpenGLMatrix(glm::value_ptr(setup.Transform));
    this->motionState = new btDefaultMotionState(tform);
    btVector3 localInertia(0, 0, 0);
    if (setup.Mass > 0.0f) {
        shape->shape->calculateLocalInertia(setup.Mass, localInertia);
    }
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(setup.Mass, this->motionState, shape->shape, localInertia);
    rigidBodyCI.m_restitution = setup.Bounciness;
    this->body = new btRigidBody(rigidBodyCI);
}

//------------------------------------------------------------------------------
void
rigidBody::discard() {
    if (this->body) {
        delete this->body;
        this->body = nullptr;
    }
    if (this->motionState) {
        delete this->motionState;
        this->motionState = nullptr;
    }
}

} // namespace _priv
} // namespace Oryol
