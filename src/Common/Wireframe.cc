//------------------------------------------------------------------------------
//  Wireframe.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Wireframe.h"
#include "Gfx/Gfx.h"
#include "wireframe_shaders.h"

namespace Oryol {

//------------------------------------------------------------------------------
void
Wireframe::Setup(const GfxDesc& gfxDesc) {
    Gfx::PushResourceLabel();
    this->drawState.VertexBuffers[0] = Gfx::CreateBuffer(BufferDesc()
        .Type(BufferType::VertexBuffer)
        .Size(MaxNumVertices * sizeof(Vertex))
        .Usage(Usage::Stream));

    Id shd = Gfx::CreateShader(WireframeShader::Desc());
    this->drawState.Pipeline = Gfx::CreatePipeline(PipelineDesc()
        .Shader(shd)
        .PrimitiveType(PrimitiveType::Lines)
        .Layout(0, {
            { "position", VertexFormat::Float3 },
            { "color0", VertexFormat::Float4 }
        })
        .BlendEnabled(true)
        .BlendSrcFactorRGB(BlendFactor::SrcAlpha)
        .BlendDstFactorRGB(BlendFactor::OneMinusSrcAlpha)
        .BlendSrcFactorAlpha(BlendFactor::Zero)
        .BlendDstFactorAlpha(BlendFactor::One)
        .ColorFormat(gfxDesc.ColorFormat())
        .DepthFormat(gfxDesc.DepthFormat())
        .SampleCount(gfxDesc.SampleCount()));
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
Wireframe::Rect(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    this->Line(p0, p1); this->Line(p1, p2); this->Line(p2, p3); this->Line(p3, p0);
}

//------------------------------------------------------------------------------
void
Wireframe::Render() {
    if (!this->vertices.Empty()) {
        Gfx::UpdateBuffer(this->drawState.VertexBuffers[0], 
            this->vertices.begin(),
            this->vertices.Size()*sizeof(Vertex));
        Gfx::ApplyDrawState(this->drawState);
        WireframeShader::vsParams vsParams;
        vsParams.viewProj = this->ViewProj;
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw(0, this->vertices.Size());
        this->vertices.Reset();
    }
}

} // namespace Oryol

