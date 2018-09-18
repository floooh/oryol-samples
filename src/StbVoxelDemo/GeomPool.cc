//------------------------------------------------------------------------------
//  GeomPool.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "GeomPool.h"
#include "Config.h"
#include "Gfx/Gfx.h"
#include "glm/geometric.hpp"
#include "glm/gtc/random.hpp"

using namespace Oryol;

//------------------------------------------------------------------------------
void
GeomPool::Setup(const GfxDesc& gfxDesc) {

    // setup a static mesh with only indices which is shared by all geom meshes
    uint16_t indices[Config::GeomMaxNumIndices];
    for (int quadIndex = 0; quadIndex < Config::GeomMaxNumQuads; quadIndex++) {
        int baseVertexIndex = quadIndex * 4;
        int ii = quadIndex * 6;
        indices[ii]   = baseVertexIndex + 0;
        indices[ii+1] = baseVertexIndex + 1;
        indices[ii+2] = baseVertexIndex + 2;
        indices[ii+3] = baseVertexIndex + 0;
        indices[ii+4] = baseVertexIndex + 2;
        indices[ii+5] = baseVertexIndex + 3;
    }
    this->IndexBuffer = Gfx::CreateBuffer(BufferDesc()
        .Type(BufferType::IndexBuffer)
        .Size(sizeof(indices))
        .Content(indices));

    // setup shader params template
    Shader::vsParams vsParams;
    vsParams.normal_table[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    vsParams.normal_table[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    vsParams.normal_table[2] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);
    vsParams.normal_table[3] = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    vsParams.normal_table[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    vsParams.normal_table[5] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
    for (int i = 0; i < int(sizeof(vsParams.color_table)/sizeof(glm::vec4)); i++) {
        vsParams.color_table[i] = glm::linearRand(glm::vec4(0.25f), glm::vec4(1.0f));
    }

    // setup shader and drawstate
    Id shd = Gfx::CreateShader(Shader::Desc());
    VertexLayout layout = {
        { "position", VertexFormat::UByte4N },
        { "normal", VertexFormat::UByte4N }
    };
    this->Pipeline = Gfx::CreatePipeline(PipelineDesc()
        .Shader(shd)
        .IndexType(IndexType::UInt16)
        .Layout(0, layout)
        .DepthCmpFunc(CompareFunc::LessEqual)
        .DepthWriteEnabled(true)
        .CullFaceEnabled(true)
        .CullFace(Face::Front)
        .ColorFormat(gfxDesc.ColorFormat())
        .DepthFormat(gfxDesc.DepthFormat())
        .SampleCount(gfxDesc.SampleCount()));

    // setup items
    auto vbufDesc = BufferDesc()
        .Type(BufferType::VertexBuffer)
        .Usage(Usage::Dynamic)
        .Size(Config::GeomMaxNumVertices * layout.ByteSize());
    for (auto& geom : this->Geoms) {
        geom.VSParams = vsParams;
        geom.NumQuads = 0;
        geom.VertexBuffer = Gfx::CreateBuffer(vbufDesc);
    }
    this->freeGeoms.Reserve(NumGeoms);
    this->FreeAll();
}

//------------------------------------------------------------------------------
void
GeomPool::Discard() {
    this->IndexBuffer.Invalidate();
    this->Pipeline.Invalidate();
    this->freeGeoms.Clear();
}

//------------------------------------------------------------------------------
void
GeomPool::FreeAll() {
    this->freeGeoms.Clear();
    for (int i = 0; i < NumGeoms; i++) {
        this->freeGeoms.Add(i);
    }
}
