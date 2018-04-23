//------------------------------------------------------------------------------
//  SceneRenderer.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "SceneRenderer.h"
#include "voxels.h"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE (30)
#define STBVOX_CONFIG_PRECISION_Z (0)
#ifdef _MSC_VER
#pragma warning(disable:4267) // size_t to int conversion
#pragma warning(disable:4244) // __in64 to int conversion
#endif
#include "Common/stb_voxel_render.h"

namespace Oryol {

//------------------------------------------------------------------------------
void
SceneRenderer::Setup(const GfxDesc& gfxDesc) {

    // setup graphics resources
    VertexLayout layout = {
        { "position", VertexFormat::UByte4N },
        { "normal", VertexFormat::UByte4N }
    };
    this->indexBuffer = this->createIndexBuffer();
    this->voxelBuffers = this->createVoxelBuffers(layout);
    this->setupDrawState(gfxDesc, layout);
    this->setupShaderParams();
}

//------------------------------------------------------------------------------
void
SceneRenderer::Discard() {
    // nothing to do here
}

//------------------------------------------------------------------------------
void
SceneRenderer::Render(const glm::mat4& viewProj) {
    for (int i = 0; i < voxelBuffers.Size(); i++) {
        this->drawState.VertexBuffers[0] = this->voxelBuffers[i].buffer;
        this->vsParams.mvp = viewProj;
        Gfx::ApplyDrawState(this->drawState);
        Gfx::ApplyUniformBlock(this->vsParams);
        Gfx::Draw(0, this->voxelBuffers[i].numQuads*6);
    }
}

//------------------------------------------------------------------------------
void
SceneRenderer::setupDrawState(const GfxDesc& gfxDesc, const VertexLayout& layout) {
    o_assert(this->indexBuffer.IsValid());
    Id shd = Gfx::CreateShader(VoxelShader::Desc());
    this->drawState.Pipeline = Gfx::CreatePipeline(PipelineDesc()
        .Shader(shd)
        .Layout(0, layout)
        .IndexType(IndexType::UInt16)
        .DepthCmpFunc(CompareFunc::LessEqual)
        .DepthWriteEnabled(true)
        .CullFaceEnabled(true)
        .CullFace(Face::Front)
        .ColorFormat(gfxDesc.ColorFormat())
        .DepthFormat(gfxDesc.DepthFormat())
        .SampleCount(gfxDesc.SampleCount()));
    this->drawState.IndexBuffer = this->indexBuffer;
}

//------------------------------------------------------------------------------
void
SceneRenderer::setupShaderParams() {

    this->vsParams.normal_table[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    this->vsParams.normal_table[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    this->vsParams.normal_table[2] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);
    this->vsParams.normal_table[3] = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    this->vsParams.normal_table[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    this->vsParams.normal_table[5] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

    const int numPaletteEntries = sizeof(Emu::Vox::Palette)/4;
    o_assert_dbg(numPaletteEntries < 32);
    for (int i = 0; i < numPaletteEntries; i++) {
        this->vsParams.color_table[i] = glm::vec4(
            Emu::Vox::Palette[i][0] / 255.0f,
            Emu::Vox::Palette[i][1] / 255.0f,
            Emu::Vox::Palette[i][2] / 255.0f,
            Emu::Vox::Palette[i][3] / 255.0f
        );
    }

    this->vsParams.light_dir = glm::normalize(glm::vec3(0.5f, 1.0f, -0.25f));
    this->vsParams.light_intensity = 1.0f;
    this->vsParams.scale = glm::vec3(1.0f);
}

//------------------------------------------------------------------------------
Id
SceneRenderer::createIndexBuffer() {
    // creates one big index mesh with the max number of indices to
    // render one of the voxel meshes
    uint16_t indices[MaxNumIndices];
    for (int quadIndex = 0; quadIndex < MaxNumQuads; quadIndex++) {
        int baseVertexIndex = quadIndex * 4;
        int ii = quadIndex * 6;
        indices[ii]   = baseVertexIndex + 0;
        indices[ii+1] = baseVertexIndex + 1;
        indices[ii+2] = baseVertexIndex + 2;
        indices[ii+3] = baseVertexIndex + 0;
        indices[ii+4] = baseVertexIndex + 2;
        indices[ii+5] = baseVertexIndex + 3;
    }

    return Gfx::CreateBuffer(BufferDesc()
        .Type(BufferType::IndexBuffer)
        .Size(sizeof(indices))
        .Content(indices));
}

//------------------------------------------------------------------------------
Array<SceneRenderer::voxMesh>
SceneRenderer::createVoxelBuffers(const VertexLayout& layout) {
    // this creates one or more Oryol meshes from the run-length-encoded
    // voxel data via stb_voxel_render

    // allocate voxel buffer and decode the runlength-encoded voxel data
    const int strideX = Emu::Vox::Z+2;
    const int strideY = (Emu::Vox::X+2)*strideX;
    const int bufSize = (Emu::Vox::X+2) * (Emu::Vox::Y+2) * (Emu::Vox::Z+2);
    uint8_t* colorBuf = (uint8_t*) Memory::Alloc(bufSize);
    Memory::Clear(colorBuf, bufSize);
    uint8_t* lightBuf = (uint8_t*) Memory::Alloc(bufSize);
    Memory::Clear(lightBuf, bufSize);
    const uint8_t* srcPtr = Emu::Vox::VoxelsRLE;
    const int rleNum = sizeof(Emu::Vox::VoxelsRLE);
    int x = 0, y = 0, z = 0;
    for (int i = 0; i < rleNum; i += 2) {
        const uint8_t num = srcPtr[i];
        o_assert_dbg(num > 0);
        const uint8_t c = srcPtr[i+1];
        for (uint8_t ii=0; ii<num; ii++) {
            const int dstIndex = (y+1)*strideY + (x+1)*strideX + (z+1);
            o_assert_dbg(dstIndex < bufSize);
            colorBuf[dstIndex] = c;
            lightBuf[dstIndex] = c == 0 ? 255 : 0;
            z++;
            if (z >= Emu::Vox::Z) {
                z = 0;
                x++;
                if (x >= Emu::Vox::X) {
                    x = 0;
                    y++;
                    o_assert_dbg(y <= Emu::Vox::Y);
                }
            }
        }
    }

    // setup stb mesher
    stbvox_mesh_maker stbvox;
    stbvox_init_mesh_maker(&stbvox);
    stbvox_set_default_mesh(&stbvox, 0);
    stbvox_set_input_stride(&stbvox, strideX, strideY);
    stbvox_set_input_range(&stbvox, 1, 1, 1, Emu::Vox::X, Emu::Vox::Y, Emu::Vox::Z);
    stbvox_input_description* desc = stbvox_get_input_description(&stbvox);
    desc->blocktype = colorBuf;
    desc->color = colorBuf;
    desc->lighting = lightBuf;

    // meshify into Oryol meshes in a loop
    const int vtxBufSize = MaxNumVertices * layout.ByteSize();
    void* vtxBuf = Memory::Alloc(vtxBufSize);

    Array<voxMesh> result;
    stbvox_set_buffer(&stbvox, 0, 0, vtxBuf, vtxBufSize);
    int voxres = 0;
    do {
        voxres = stbvox_make_mesh(&stbvox);
        int numQuads = stbvox_get_quad_count(&stbvox, 0);
        if (numQuads > 0) {
            voxMesh m;
            m.buffer = Gfx::CreateBuffer(BufferDesc()
                .Type(BufferType::VertexBuffer)
                .Size(numQuads * 4 * layout.ByteSize())
                .Content(vtxBuf));
            m.numQuads = numQuads;
            result.Add(m);
        }
        stbvox_reset_buffers(&stbvox);
        stbvox_set_buffer(&stbvox, 0, 0, vtxBuf, vtxBufSize);
    }
    while (0 == voxres);
    Memory::Free(vtxBuf);
    Memory::Free(colorBuf);
    Memory::Free(lightBuf);

    return result;
}

} // namespace Oryol

