//------------------------------------------------------------------------------
//  softBody.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "softBody.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
softBody::~softBody() {
    o_assert_dbg(!this->body);
}

//------------------------------------------------------------------------------
void
softBody::setup(const SoftBodySetup& setup, const btSoftBodyWorldInfo& worldInfo) {
    o_assert_dbg(!this->body);

    btVector3 corner[4];
    for (int i = 0; i < 4; i++) {
        const auto& p = setup.Points[i];
        corner[i] = btVector3(p.x, p.y, p.z);
    }
    this->body = btSoftBodyHelpers::CreatePatch(
        (btSoftBodyWorldInfo&)worldInfo,
        corner[0], corner[1], corner[2], corner[3],
        setup.NumSegments, setup.NumSegments,
        setup.Fixed,
        false);
    this->body->m_cfg.kDP = 0.01f;
    this->body->m_cfg.piterations = 2;
}

//------------------------------------------------------------------------------
void
softBody::discard() {
    if (this->body) {
        delete this->body;
        this->body = nullptr;
    }
}

} // namespace _priv
} // namespace Oryol
