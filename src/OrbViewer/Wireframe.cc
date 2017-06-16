//------------------------------------------------------------------------------
//  Wireframe.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Wireframe.h"
#include "Gfx/Gfx.h"
#include "shaders.h"

namespace Oryol {

//------------------------------------------------------------------------------
void
Wireframe::Setup(const GfxSetup& gfxSetup) {
    Gfx::PushResourceLabel();
    auto meshSetup = MeshSetup::Empty(MaxNumVertices, Usage::Stream);
    meshSetup.Layout = {
        { VertexAttr::Position, VertexFormat::Float3 },
        { VertexAttr::Color0, VertexFormat::Float4 }
    };
    this->drawState.Mesh[0] = Gfx::CreateResource(meshSetup);

    Id shd = Gfx::CreateResource(WireframeShader::Setup());
    auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, shd);
    pipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
    pipSetup.BlendState.ColorFormat = gfxSetup.ColorFormat;
    pipSetup.BlendState.DepthFormat = gfxSetup.DepthFormat;
    pipSetup.PrimType = PrimitiveType::Lines;
    this->drawState.Pipeline = Gfx::CreateResource(pipSetup);
    this->label = Gfx::PopResourceLabel();
}

//------------------------------------------------------------------------------
void
Wireframe::Discard() {
    Gfx::DestroyResources(this->label);
}

//------------------------------------------------------------------------------
void
Wireframe::Line(const glm::vec4& p0, const glm::vec4& p1) {
    if ((this->vertices.Size() + 2) < MaxNumVertices) {
        this->vertices.Add(this->Model * p0, this->Color);
        this->vertices.Add(this->Model * p1, this->Color);
    }
}

//------------------------------------------------------------------------------
void
Wireframe::Line(const glm::vec3& p0, const glm::vec3& p1) {
    this->Line(glm::vec4(p0, 1.0f), glm::vec4(p1, 1.0f));
}

//------------------------------------------------------------------------------
void
Wireframe::Render() {
    if (!this->vertices.Empty()) {
        Gfx::UpdateVertices(this->drawState.Mesh[0], this->vertices.begin(), this->vertices.Size()*sizeof(Vertex));
        Gfx::ApplyDrawState(this->drawState);
        WireframeShader::vsParams vsParams;
        vsParams.viewProj = this->ViewProj;
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw({ 0, this->vertices.Size() });
    }
}

} // namespace Oryol

