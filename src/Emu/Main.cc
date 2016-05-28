//------------------------------------------------------------------------------
//  Emu/Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/Time/Clock.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "Common/CameraHelper.h"
#include "KC85Emu.h"
#include "voxels.h"
#include "shaders.h"
#include "glm/gtc/matrix_transform.hpp"
#define STB_VOXEL_RENDER_IMPLEMENTATION
#define STBVOX_CONFIG_MODE (30)
#define STBVOX_CONFIG_PRECISION_Z (0)
#include "Common/stb_voxel_render.h"

using namespace Oryol;

class EmuApp : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    struct voxMesh {
        Id mesh;
        int numQuads = 0;
    };

    static const int MaxNumVertices = (1<<16);          // max vertices per voxel mesh
    static const int MaxNumQuads = MaxNumVertices / 4;
    static const int MaxNumIndices = MaxNumQuads * 6;

    Id createIndexMesh();
    Array<voxMesh> createVoxelMeshes(const VertexLayout& layout);
    void setupDrawState(const GfxSetup& gfxSetup, const VertexLayout& layout);
    void setupShaderParams();

    TimePoint lapTime;
    KC85Emu kc85Emu;
    glm::mat4 kcModelMatrix;
    CameraHelper camera;
    ClearState clearState = ClearState::ClearAll(glm::vec4(0.4f, 0.6f, 0.8f, 1.0f), 1.0f, 0);
    DrawState drawState;
    VoxelShader::VoxelVSParams vsParams;
    Id indexMesh;
    Array<voxMesh> voxelMeshes;
};
OryolMain(EmuApp);

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(1024, 600, "Emu");
    Gfx::Setup(gfxSetup);
    Input::Setup();
    Input::BeginCaptureText();

    // setup graphics resources
    VertexLayout layout;
    layout.Add(VertexAttr::Position, VertexFormat::UByte4N);
    layout.Add(VertexAttr::Normal, VertexFormat::UByte4N);
    this->indexMesh = this->createIndexMesh();
    this->voxelMeshes = this->createVoxelMeshes(layout);
    this->setupDrawState(gfxSetup, layout);
    this->setupShaderParams();

    // setup the camera helper
    this->camera.Setup(false);
    this->camera.Center = glm::vec3(63.0f, 25.0f, 40.0f);
    this->camera.MaxCamDist = 200.0f;
    this->camera.Distance = 80.0f;
    this->camera.Orbital = glm::vec2(glm::radians(10.0f), glm::radians(160.0f));

    // setup the KC emulator
    this->kc85Emu.Setup(gfxSetup);
    this->lapTime = Clock::Now();

    glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::translate(m, glm::vec3(-63.0f, 39.0f, 25.0f));
    m = glm::scale(m, glm::vec3(28.0f, 1.0f, 22.0f));
    this->kcModelMatrix = m;

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnRunning() {
    this->camera.Update();

    // update KC85 emu
    this->kc85Emu.Update(Clock::LapTime(this->lapTime));

    // render the voxel scene and emulator screen
    Gfx::ApplyDefaultRenderTarget(this->clearState);
    for (int i = 0; i < voxelMeshes.Size(); i++) {
        this->drawState.Mesh[1] = this->voxelMeshes[i].mesh;
        this->vsParams.ModelViewProjection = this->camera.ViewProj;
        Gfx::ApplyDrawState(this->drawState);
        Gfx::ApplyUniformBlock(this->vsParams);
        Gfx::Draw(PrimitiveGroup(0, this->voxelMeshes[i].numQuads*6));
    }
    this->kc85Emu.Render(this->camera.ViewProj * this->kcModelMatrix);
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnCleanup() {
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
EmuApp::setupDrawState(const GfxSetup& gfxSetup, const VertexLayout& layout) {
    o_assert(this->indexMesh.IsValid());
    Id shd = Gfx::CreateResource(VoxelShader::Setup());
    auto pip = PipelineSetup::FromShader(shd);
    pip.Layouts[1] = layout;    // IMPORTANT: vertices are in mesh slot #1, not #0!
    pip.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pip.DepthStencilState.DepthWriteEnabled = true;
    pip.RasterizerState.CullFaceEnabled = true;
    pip.RasterizerState.CullFace = Face::Front;
    pip.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->drawState.Pipeline = Gfx::CreateResource(pip);
    this->drawState.Mesh[0] = this->indexMesh;
}

//------------------------------------------------------------------------------
void
EmuApp::setupShaderParams() {

    this->vsParams.NormalTable[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    this->vsParams.NormalTable[1] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    this->vsParams.NormalTable[2] = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);
    this->vsParams.NormalTable[3] = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    this->vsParams.NormalTable[4] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    this->vsParams.NormalTable[5] = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

    const int numPaletteEntries = sizeof(Emu::Vox::Palette)/4;
    o_assert_dbg(numPaletteEntries < 32);
    for (int i = 0; i < numPaletteEntries; i++) {
        this->vsParams.ColorTable[i] = glm::vec4(
            Emu::Vox::Palette[i][0] / 255.0f,
            Emu::Vox::Palette[i][1] / 255.0f,
            Emu::Vox::Palette[i][2] / 255.0f,
            Emu::Vox::Palette[i][3] / 255.0f
        );
    }

    this->vsParams.LightDir = glm::normalize(glm::vec3(0.5f, 1.0f, -0.25f));
    this->vsParams.Scale = glm::vec3(1.0f);
}

//------------------------------------------------------------------------------
Id
EmuApp::createIndexMesh() {
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

    auto meshSetup = MeshSetup::FromData(Usage::InvalidUsage, Usage::Immutable);
    meshSetup.NumVertices = 0;
    meshSetup.NumIndices  = MaxNumIndices;
    meshSetup.IndicesType = IndexType::Index16;
    meshSetup.DataVertexOffset = InvalidIndex;
    meshSetup.DataIndexOffset = 0;
    return Gfx::CreateResource(meshSetup, indices, sizeof(indices));
}

//------------------------------------------------------------------------------
Array<EmuApp::voxMesh>
EmuApp::createVoxelMeshes(const VertexLayout& layout) {
    // this creates one or more Oryol meshes from the run-length-encoded
    // voxel data via stb_voxel_render

    // allocate voxel buffer and decode the runlength-encoded voxel data
    const int bufSize = Emu::Vox::X*Emu::Vox::Y*Emu::Vox::Z;
    uint8_t* colorBuf = (uint8_t*) Memory::Alloc(bufSize);
    uint8_t* lightBuf = (uint8_t*) Memory::Alloc(bufSize);
    int dstIndex = 0;
    const uint8_t* srcPtr = Emu::Vox::VoxelsRLE;
    const int rleNum = sizeof(Emu::Vox::VoxelsRLE);
    for (int i = 0; i < rleNum; i += 2) {
        const uint8_t num = srcPtr[i];
        o_assert_dbg(num > 0);
        const uint8_t c = srcPtr[i+1];
        for (uint8_t ii=0; ii<num; ii++) {
            o_assert_dbg(dstIndex < bufSize);
            colorBuf[dstIndex] = c;
            lightBuf[dstIndex] = c == 0 ? 255 : 0;
            dstIndex++;
        }
    }
    o_assert_dbg(dstIndex == bufSize);

    // setup stb mesher
    stbvox_mesh_maker stbvox;
    stbvox_init_mesh_maker(&stbvox);
    stbvox_set_default_mesh(&stbvox, 0);
    stbvox_set_buffer(&stbvox, 0, 0, colorBuf, bufSize);
    const int strideX = Emu::Vox::Z;
    const int strideY = Emu::Vox::X*Emu::Vox::Z;
    stbvox_set_input_stride(&stbvox, strideX, strideY);
    stbvox_set_input_range(&stbvox, 0, 0, 0, Emu::Vox::X, Emu::Vox::Y, Emu::Vox::Z);
    stbvox_input_description* desc = stbvox_get_input_description(&stbvox);
    desc->blocktype = colorBuf;
    desc->color = colorBuf;
    desc->lighting = lightBuf;

    // meshify into Oryol meshes in a loop
    auto meshSetup = MeshSetup::FromData();
    meshSetup.Layout = layout;
    const int vtxBufSize = MaxNumVertices * layout.ByteSize();
    void* vtxBuf = Memory::Alloc((1<<16)*8);

    Array<voxMesh> result;
    stbvox_set_buffer(&stbvox, 0, 0, vtxBuf, vtxBufSize);
    int voxres = 0;
    do {
        voxres = stbvox_make_mesh(&stbvox);
        int numQuads = stbvox_get_quad_count(&stbvox, 0);
        if (numQuads > 0) {
            voxMesh m;
            meshSetup.NumVertices = numQuads * 4;
            o_assert_dbg(meshSetup.NumVertices <= MaxNumVertices);
            m.mesh = Gfx::CreateResource(meshSetup, vtxBuf, vtxBufSize);
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
