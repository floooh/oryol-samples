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
ShapeRenderer::Setup(const GfxDesc& gfxDesc) {
    o_assert_dbg(this->ColorShader.IsValid());
    o_assert_dbg(this->ColorShaderInstanced.IsValid());
    o_assert_dbg(this->ShadowShader.IsValid());
    o_assert_dbg(this->ShadowShaderInstanced.IsValid());

    // create 2 instance-data buffers, one for the spheres, one for the boxes,
    // per-instance data is a 4x3 transposed model matrix, and vec4 color
    VertexLayout instLayout = VertexLayout()
        .EnableInstancing()
        .Add({
            { "instance0", VertexFormat::Float4 },
            { "instance1", VertexFormat::Float4 },
            { "instance2", VertexFormat::Float4 },
            { "color0",    VertexFormat::Float4 }
        });
    auto instBufDesc = BufferDesc()
        .Type(BufferType::VertexBuffer)
        .Size(MaxNumInstances * instLayout.ByteSize())
        .Usage(Usage::Stream);
    this->SphereInstBuffer = Gfx::CreateBuffer(instBufDesc);
    this->BoxInstBuffer = Gfx::CreateBuffer(instBufDesc);

    // pre-initialize the instance data with random colors
    for (int i = 0; i < MaxNumInstances; i++) {
        this->SphereInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }
    for (int i = 0; i < MaxNumInstances; i++) {
        this->BoxInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }

    // setup a mesh with a sphere and box submesh, which is used by all draw states
    auto shape = ShapeBuilder()
        .Positions("position", VertexFormat::Float3)
        .Normals("normal", VertexFormat::Byte4N)
        .Plane(100, 100, 1)
        .Sphere(this->SphereRadius, 15, 11)
        .Box(this->BoxSize, this->BoxSize, this->BoxSize, 1)
        .Build();
    this->planePrimGroup = shape.PrimitiveGroups[0];
    this->spherePrimGroup = shape.PrimitiveGroups[1];
    this->boxPrimGroup = shape.PrimitiveGroups[2];
    Id shapeVertexBuffer = Gfx::CreateBuffer(shape.VertexBufferDesc);
    Id shapeIndexBuffer = Gfx::CreateBuffer(shape.IndexBufferDesc);
    this->ColorDrawState.VertexBuffers[0] = shapeVertexBuffer;
    this->ColorInstancedDrawState.VertexBuffers[0] = shapeVertexBuffer;
    this->ShadowDrawState.VertexBuffers[0] = shapeVertexBuffer;
    this->ShadowInstancedDrawState.VertexBuffers[0] = shapeVertexBuffer;
    this->ColorDrawState.IndexBuffer = shapeIndexBuffer;
    this->ColorInstancedDrawState.IndexBuffer = shapeIndexBuffer;
    this->ShadowDrawState.IndexBuffer = shapeIndexBuffer;
    this->ShadowInstancedDrawState.IndexBuffer = shapeIndexBuffer;

    // create color pass pipeline states (one for non-instanced, one for instanced rendering)
    auto pipDesc = PipelineDesc(shape.PipelineDesc)
        .DepthWriteEnabled(true)
        .DepthCmpFunc(CompareFunc::LessEqual)
        .CullFaceEnabled(true)
        .ColorFormat(gfxDesc.ColorFormat())
        .DepthFormat(gfxDesc.DepthFormat())
        .SampleCount(gfxDesc.SampleCount());
    this->ColorDrawState.Pipeline = Gfx::CreatePipeline(pipDesc.Shader(this->ColorShader));
    this->ColorInstancedDrawState.Pipeline = Gfx::CreatePipeline(pipDesc
        .Shader(this->ColorShaderInstanced)
        .Layout(1, instLayout));

    // create shadow map, use RGBA8 format and encode/decode depth in pixel shader
    const int smSize = this->ShadowMapSize;
    auto smDesc = TextureDesc()
        .RenderTarget(true)
        .Type(TextureType::Texture2D)
        .Width(smSize)
        .Height(smSize);
    this->ShadowMap = Gfx::CreateTexture(smDesc.Format(PixelFormat::RGBA8));
    this->ColorDrawState.FSTexture[0] = this->ShadowMap;
    this->ColorInstancedDrawState.FSTexture[0] = this->ShadowMap;
    Id smDepthBuffer = Gfx::CreateTexture(smDesc.Format(PixelFormat::DEPTH));
    this->ShadowPassAction = PassAction().Clear(glm::vec4(1.0), 1.0f, 0);
    this->ShadowPass = Gfx::CreatePass(PassDesc()
        .ColorAttachment(0, this->ShadowMap)
        .DepthStencilAttachment(smDepthBuffer));

    // create shadow pass pipeline states (one for non-instanced, one for instanced rendering)
    pipDesc = PipelineDesc(shape.PipelineDesc)
        .DepthWriteEnabled(true)
        .DepthCmpFunc(CompareFunc::LessEqual)
        .CullFaceEnabled(true)
        .SampleCount(1)
        .ColorFormat(PixelFormat::RGBA8)
        .DepthFormat(PixelFormat::DEPTH);
    this->ShadowDrawState.Pipeline = Gfx::CreatePipeline(pipDesc.Shader(this->ShadowShader));
    this->ShadowInstancedDrawState.Pipeline = Gfx::CreatePipeline(pipDesc
        .Shader(this->ShadowShaderInstanced)
        .Layout(1, instLayout));
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
        Gfx::UpdateBuffer(this->SphereInstBuffer, this->SphereInstData, sizeof(instData) * this->NumSpheres);
    }
    if (this->NumBoxes > 0) {
        Gfx::UpdateBuffer(this->BoxInstBuffer, this->BoxInstData, sizeof(instData) * this->NumBoxes);
    }
}

} // namespace Oryol
