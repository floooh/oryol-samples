//------------------------------------------------------------------------------
//  rigidBody.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "rigidBody.h"
#include "Core/Assertion.h"

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
    o_error("FIXME\n");
}

//------------------------------------------------------------------------------
void
rigidBody::discard() {
    o_error("FIXME\n");
}

} // namespace _priv
} // namespace Oryol
