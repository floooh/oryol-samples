//------------------------------------------------------------------------------
//  ShapeRenderer.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "ShapeRenderer.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "glm/gtc/random.hpp"

namespace Oryol {

//------------------------------------------------------------------------------
void
ShapeRenderer::Setup(const GfxSetup& gfxSetup) {
    o_assert_dbg(this->ColorShader.IsValid());
    o_assert_dbg(this->ColorShaderInstanced.IsValid());
    o_assert_dbg(this->ShadowShader.IsValid());
    o_assert_dbg(this->ShadowShaderInstanced.IsValid());

    // create 2 instance-data buffers, one for the spheres, one for the boxes,
    // per-instance data is a 4x3 transposed model matrix, and vec4 color
    auto instSetup = MeshSetup::Empty(MaxNumInstances, Usage::Stream);
    instSetup.Layout
        .EnableInstancing()
        .Add(VertexAttr::Instance0, VertexFormat::Float4)
        .Add(VertexAttr::Instance1, VertexFormat::Float4)
        .Add(VertexAttr::Instance2, VertexFormat::Float4)
        .Add(VertexAttr::Color0, VertexFormat::Float4);
    this->SphereInstMesh = Gfx::CreateResource(instSetup);
    this->BoxInstMesh    = Gfx::CreateResource(instSetup);

    // pre-initialize the instance data with random colors
    for (int i = 0; i < MaxNumInstances; i++) {
        this->SphereInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }
    for (int i = 0; i < MaxNumInstances; i++) {
        this->BoxInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }

    // setup a mesh with a sphere and box submesh, which is used by all draw states
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder
        .Plane(100, 100, 1)
        .Sphere(this->SphereRadius, 15, 11)
        .Box(this->BoxSize, this->BoxSize, this->BoxSize, 1);
    Id shapeMesh = Gfx::CreateResource(shapeBuilder.Build());
    this->ColorDrawState.Mesh[0] = shapeMesh;
    this->ColorInstancedDrawState.Mesh[0] = shapeMesh;
    this->ShadowDrawState.Mesh[0] = shapeMesh;
    this->ShadowInstancedDrawState.Mesh[0] = shapeMesh;

    // create color pass pipeline states (one for non-instanced, one for instanced rendering)
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, this->ColorShader);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->ColorDrawState.Pipeline = Gfx::CreateResource(ps);
    ps.Shader = this->ColorShaderInstanced;
    ps.Layouts[1] = instSetup.Layout;
    this->ColorInstancedDrawState.Pipeline = Gfx::CreateResource(ps);

    // create shadow map, use RGBA8 format and encode/decode depth in pixel shader
    auto smSetup = TextureSetup::RenderTarget(this->ShadowMapSize, this->ShadowMapSize);
    smSetup.ColorFormat = PixelFormat::RGBA8;
    smSetup.DepthFormat = PixelFormat::DEPTH;
    this->ShadowMap = Gfx::CreateResource(smSetup);
    this->ColorDrawState.FSTexture[0] = this->ShadowMap;

    // create shadow pass pipeline states (one for non-instanced, one for instanced rendering)
    ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, this->ShadowShader);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.CullFace = Face::Front;
    ps.RasterizerState.SampleCount = 1;
    ps.BlendState.ColorFormat = smSetup.ColorFormat;
    ps.BlendState.DepthFormat = smSetup.DepthFormat;
    this->ShadowDrawState.Pipeline = Gfx::CreateResource(ps);
    ps.Shader = this->ShadowShaderInstanced;
    ps.Layouts[1] = instSetup.Layout;
    this->ShadowInstancedDrawState.Pipeline = Gfx::CreateResource(ps);
}

//------------------------------------------------------------------------------
void
ShapeRenderer::BeginTransforms() {
    this->NumBoxes = 0;
    this->NumSpheres = 0;
}

//------------------------------------------------------------------------------
void
ShapeRenderer::EndTransforms() {
    if (this->NumSpheres > 0) {
        Gfx::UpdateVertices(this->SphereInstMesh, this->SphereInstData, sizeof(instData) * this->NumSpheres);
    }
    if (this->NumBoxes > 0) {
        Gfx::UpdateVertices(this->BoxInstMesh, this->BoxInstData, sizeof(instData) * this->NumBoxes);
    }
}

} // namespace Oryol
