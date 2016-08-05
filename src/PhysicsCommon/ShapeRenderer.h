#pragma
//------------------------------------------------------------------------------
/**
    @class Oryol::ShapeRenderer
    @brief simple shape rendering helper for physics samples
*/
#include "Gfx/Gfx.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace Oryol {

class ShapeRenderer {
public:
    /// the shaders must have been created before Setup() is called!
    Id ColorShader;
    Id ColorShaderInstanced;
    Id ShadowShader;
    Id ShadowShaderInstanced;
    float SphereRadius = 1.0f;
    float BoxSize = 1.0f;
    int ShadowMapSize = 2048;

    /// setup the shape renderer
    void Setup(const GfxSetup& gfxSetup);
    /// discard the shape renderer
    void Discard();
    /// draw the shadow pass (no ground)
    template<class VSPARAMS> void DrawShadowPass(const VSPARAMS& vsParams);
    /// draw the ground
    template<class VSPARAMS, class FSPARAMS> void DrawGround(const VSPARAMS& vsParams, const FSPARAMS& fsParams);
    /// draw the dynamic shapes
    template<class VSPARAMS, class FSPARAMS> void DrawShapes(const VSPARAMS& vsParams, const FSPARAMS& fsParams);
    /// begin updating shape transforms
    void BeginTransforms();
    /// update next box transform
    void UpdateBoxTransform(const glm::mat4& m);
    /// update next sphere transform
    void UpdateSphereTransform(const glm::mat4& m);
    /// finish updating shape transforms
    void EndTransforms();

    static const int MaxNumInstances = 1024;
    Id ShadowMap;
    DrawState ColorDrawState;
    DrawState ShadowDrawState;
    DrawState ColorInstancedDrawState;
    DrawState ShadowInstancedDrawState;
    int NumSpheres = 0;
    int NumBoxes = 0;
    Id SphereInstMesh;
    Id BoxInstMesh;
    struct instData {
        glm::vec4 xxxx;
        glm::vec4 yyyy;
        glm::vec4 zzzz;
        glm::vec4 color;
    };
    instData SphereInstData[MaxNumInstances];
    instData BoxInstData[MaxNumInstances];

};

//------------------------------------------------------------------------------
template<class VSPARAMS> inline void
ShapeRenderer::DrawShadowPass(const VSPARAMS& vsParams) {
    Gfx::ApplyRenderTarget(this->ShadowMap, ClearState::ClearAll(glm::vec4(1.0f), 1.0f, 0));
    if (this->NumSpheres > 0) {
        this->ShadowInstancedDrawState.Mesh[1] = this->SphereInstMesh;
        Gfx::ApplyDrawState(this->ShadowInstancedDrawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw(1, this->NumSpheres);
    }
    if (this->NumBoxes > 0) {
        this->ShadowInstancedDrawState.Mesh[1] = this->BoxInstMesh;
        Gfx::ApplyDrawState(this->ShadowInstancedDrawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw(2, this->NumBoxes);
    }
}

//------------------------------------------------------------------------------
template<class VSPARAMS, class FSPARAMS> void
ShapeRenderer::DrawGround(const VSPARAMS& vsParams, const FSPARAMS& fsParams) {
    Gfx::ApplyDrawState(this->ColorDrawState);
    Gfx::ApplyUniformBlock(vsParams);
    Gfx::ApplyUniformBlock(fsParams);
    Gfx::Draw();
}

//------------------------------------------------------------------------------
template<class VSPARAMS, class FSPARAMS> void
ShapeRenderer::DrawShapes(const VSPARAMS& vsParams, const FSPARAMS& fsParams) {
    if (this->NumSpheres > 0) {
        this->ColorInstancedDrawState.Mesh[1] = this->SphereInstMesh;
        Gfx::ApplyDrawState(this->ColorInstancedDrawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::ApplyUniformBlock(fsParams);
        Gfx::Draw(1, this->NumSpheres);
    }
    if (this->NumBoxes > 0) {
        this->ColorInstancedDrawState.Mesh[1] = this->BoxInstMesh;
        Gfx::ApplyDrawState(this->ColorInstancedDrawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::ApplyUniformBlock(fsParams);
        Gfx::Draw(2, this->NumBoxes);
    }
}

//------------------------------------------------------------------------------
inline void
ShapeRenderer::UpdateBoxTransform(const glm::mat4& m) {
    if (this->NumBoxes < MaxNumInstances) {
        instData& item = this->BoxInstData[this->NumBoxes++];
        item.xxxx = glm::row(m, 0);
        item.yyyy = glm::row(m, 1);
        item.zzzz = glm::row(m, 2);
    }
}

//------------------------------------------------------------------------------
inline void
ShapeRenderer::UpdateSphereTransform(const glm::mat4& m) {
    if (this->NumSpheres < MaxNumInstances) {
        instData& item = this->SphereInstData[this->NumSpheres++];
        item.xxxx = glm::row(m, 0);
        item.yyyy = glm::row(m, 1);
        item.zzzz = glm::row(m, 2);
    }
}

} // namespace Oryol
