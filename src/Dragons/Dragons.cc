//------------------------------------------------------------------------------
//  Dragons.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "Anim/Anim.h"
#include "HttpFS/HTTPFileSystem.h"
#include "Core/Containers/InlineArray.h"
#include "Common/CameraHelper.h"
#include "Common/OrbLoader.h"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

using namespace Oryol;

class Dragons : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    void loadModel(const Locator& loc);
    void addInstance(const glm::vec3& pos, int animClipIndex);

    static const int MaxNumInstances = 128;
    static const int BoneTextureWidth = 1024;
    static const int BoneTextureHeight = 128;

    GfxSetup gfxSetup;
    CameraHelper camera;
    Id shader;
    OrbModel orbModel;
    DrawState drawState;
    Shader::vsParams vsParams;
    struct Instance {
        Id animInstance;
        glm::mat4 modelMatrix;
    };
    InlineArray<Instance, MaxNumInstances> instances;

    // xxxx,yyyy,zzzz is transposed model matrix
    // boneInfo is position in bone texture
    VertexLayout instanceMeshLayout;
    struct InstanceVertex {
        float xxxx[4];
        float yyyy[4];
        float zzzz[4];
        float boneInfo[4];
    };
    InstanceVertex InstanceData[MaxNumInstances];
};
OryolMain(Dragons);

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnInit() {
    IOSetup ioSetup;
    ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
//    ioSetup.Assigns.Add("orb:", ORYOL_SAMPLE_URL);
ioSetup.Assigns.Add("orb:", "http://localhost:8000/");
    IO::Setup(ioSetup);

    this->gfxSetup = GfxSetup::WindowMSAA4(1024, 640, "Dragons");
    this->gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.2f, 0.3f, 0.5f, 1.0f));
    Gfx::Setup(this->gfxSetup);
    AnimSetup animSetup;
    animSetup.MaxNumActiveInstances = MaxNumInstances;
    animSetup.SkinMatrixTableWidth = BoneTextureWidth;
    animSetup.SkinMatrixTableHeight = BoneTextureHeight;
    Anim::Setup(animSetup);
    Input::Setup();

    this->camera.Setup(false);
    this->camera.Center = glm::vec3(0.0f, 0.0f, -2.5f);
    this->camera.Distance = 10.0f;
    this->camera.Orbital = glm::vec2(glm::radians(25.0f), 0.0f);

    // can setup the shader before loading any assets
    this->shader = Gfx::CreateResource(Shader::Setup());

    // RGBA32F texture for the animated skeleton bone info
    auto texSetup = TextureSetup::Empty2D(BoneTextureWidth, BoneTextureHeight, 1, PixelFormat::RGBA32F, Usage::Stream);
    texSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    texSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    this->drawState.VSTexture[Shader::boneTex] = Gfx::CreateResource(texSetup);

    // vertex buffer for per-instance info
    auto meshSetup = MeshSetup::Empty(MaxNumInstances, Usage::Stream);
    meshSetup.Layout = {
        { VertexAttr::Instance0, VertexFormat::Float4 },
        { VertexAttr::Instance1, VertexFormat::Float4 },
        { VertexAttr::Instance2, VertexFormat::Float4 },
        { VertexAttr::Instance3, VertexFormat::Float4 }
    };
    meshSetup.Layout.EnableInstancing();
    this->instanceMeshLayout = meshSetup.Layout;
    this->drawState.Mesh[1] = Gfx::CreateResource(meshSetup);

    // load the dragon.orb file and add the first model instance
    this->loadModel("orb:dragon.orb");

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnRunning() {
    this->camera.Update();

    // update the animation system
    if (this->orbModel.IsValid) {
        Anim::NewFrame();
        for (const auto& inst : this->instances) {
            Anim::AddActiveInstance(inst.animInstance);
        }
        Anim::Evaluate(1.0/60.0);

        // upload bone info to GPU texture
        const AnimSkinMatrixInfo& boneInfo = Anim::SkinMatrixInfo();
        ImageDataAttrs imgAttrs;
        imgAttrs.NumFaces = 1;
        imgAttrs.NumMipMaps = 1;
        imgAttrs.Offsets[0][0] = 0;
        imgAttrs.Sizes[0][0] = boneInfo.SkinMatrixTableByteSize;
        Gfx::UpdateTexture(this->drawState.VSTexture[Shader::boneTex], boneInfo.SkinMatrixTable, imgAttrs);

        // update the instance vertex buffer
        int instIndex = 0;
        for (const auto& inst : this->instances) {
            const glm::mat4& m = inst.modelMatrix;
            const auto& shdInfo = boneInfo.InstanceInfos[instIndex].ShaderInfo;
            auto& vtx = this->InstanceData[instIndex];
            for (int i = 0; i < 4; i++) {
                vtx.xxxx[i] = m[i][0];
                vtx.yyyy[i] = m[i][1];
                vtx.zzzz[i] = m[i][2];
                vtx.boneInfo[i] = shdInfo[i];
            }
            instIndex++;
        }
        Gfx::UpdateVertices(this->drawState.Mesh[1], this->InstanceData, sizeof(InstanceVertex)*instIndex);
    }

    Gfx::BeginPass();
    if (this->orbModel.IsValid) {
        const AnimSkinMatrixInfo& boneInfo = Anim::SkinMatrixInfo();
        Gfx::ApplyDrawState(this->drawState);
        this->vsParams.view_proj = this->camera.ViewProj;
        Gfx::ApplyUniformBlock(this->vsParams);
        Gfx::Draw(1, this->instances.Size());
    }
    Gfx::EndPass();
    Gfx::CommitFrame();

    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnCleanup() {
    Input::Discard();
    Anim::Discard();
    Gfx::Discard();
    IO::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
Dragons::loadModel(const Locator& loc) {
    // start loading the .n3o file
    IO::Load(loc.Location(), [this](IO::LoadResult res) {
        if (OrbLoader::Load(res.Data, "model", this->orbModel)) {
            this->drawState.Mesh[0] = this->orbModel.Mesh;

            auto pipSetup = PipelineSetup::FromShader(this->shader);
            pipSetup.Layouts[0] = this->orbModel.Layout;
            pipSetup.Layouts[1] = this->instanceMeshLayout;
            pipSetup.DepthStencilState.DepthWriteEnabled = true;
            pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
            pipSetup.RasterizerState.CullFaceEnabled = true;
            pipSetup.RasterizerState.SampleCount = this->gfxSetup.SampleCount;
            pipSetup.BlendState.ColorFormat = this->gfxSetup.ColorFormat;
            pipSetup.BlendState.DepthFormat = this->gfxSetup.DepthFormat;
            this->drawState.Pipeline = Gfx::CreateResource(pipSetup);

            this->addInstance(glm::vec3(2.5f, 0.0f, 0.0f), 0);
            this->addInstance(glm::vec3(-5.0f, 0.0f, 0.0f), 6);
        }
    },
    [](const URL& url, IOStatus::Code ioStatus) {
        // loading failed, just display an error message and carry on
        Log::Error("Failed to load file '%s' with '%s'\n", url.AsCStr(), IOStatus::ToString(ioStatus));
    });
}

//------------------------------------------------------------------------------
void
Dragons::addInstance(const glm::vec3& pos, int clipIndex) {
    if (!this->orbModel.IsValid) {
        return;
    }
    auto& inst = this->instances.Add();
    inst.modelMatrix = glm::translate(glm::mat4(), pos);
    inst.animInstance = Anim::Create(AnimInstanceSetup::FromLibraryAndSkeleton(
        this->orbModel.AnimLib,
        this->orbModel.Skeleton));
    AnimJob job;
    job.ClipIndex = clipIndex;
    job.TrackIndex = 0;
    Anim::Play(inst.animInstance, job);
}
