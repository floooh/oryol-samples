//------------------------------------------------------------------------------
//  rigidBody.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "rigidBody.h"
#include "Core/Assertion.h"
#include "glm/gtc/type_ptr.hpp"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
rigidBody::~rigidBody() {
    o_assert_dbg(!this->shape);
    o_assert_dbg(!this->body);
    o_assert_dbg(!this->motionState);
}

//------------------------------------------------------------------------------
void
rigidBody::setup(const RigidBodySetup& setup) {
    o_assert_dbg(!this->shape);
    o_assert_dbg(!this->body);
    o_assert_dbg(!this->motionState);

    switch (setup.Type) {
        case RigidBodySetup::PlaneShape:
            this->shape = new btStaticPlaneShape(
                btVector3(setup.PlaneValues.x,
                          setup.PlaneValues.y,
                          setup.PlaneValues.z),
                setup.PlaneValues.w);
            break;

        case RigidBodySetup::SphereShape:
            this->shape = new btSphereShape(setup.Radius);
            break;

        case RigidBodySetup::BoxShape:
            this->shape = new btBoxShape(
                btVector3(setup.Size.x * 0.5f,
                          setup.Size.y * 0.5f,
                          setup.Size.z * 0.5f));
            break;

        default:
            o_assert(false);
            break;
    }

    // transform 4x4 transform to btTransform
    btTransform tform;
    tform.setFromOpenGLMatrix(glm::value_ptr(setup.Transform));
    this->motionState = new btDefaultMotionState(tform);
    btVector3 localInertia(0, 0, 0);
    if (setup.Mass > 0.0f) {
        this->shape->calculateLocalInertia(setup.Mass, localInertia);
    }
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(setup.Mass, this->motionState, this->shape, localInertia);
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
    if (this->shape) {
        delete this->shape;
        this->shape = nullptr;
    }
}

} // namespace _priv
} // namespace Oryol
