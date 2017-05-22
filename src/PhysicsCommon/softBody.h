#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::softBody
    @brief wrapper for Bullet soft body
*/
#include "Resource/ResourceBase.h"
#include "PhysicsCommon/SoftBodySetup.h"
#include "BulletSoftBody/btSoftBody.h"

namespace Oryol {
namespace _priv {

class softBody : public ResourceBase<SoftBodySetup> {
public:
    /// destructor
    ~softBody();
    /// setup the soft body object
    void setup(const SoftBodySetup& setup, const btSoftBodyWorldInfo& worldInfo);
    /// discard the soft body body
    void discard();

    btSoftBody* body = nullptr;
};

} // namespace _priv
} // namespace Oryol

