//------------------------------------------------------------------------------
//  Dragons.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/Time/Clock.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "Anim/Anim.h"
#include "IMUI/IMUI.h"
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
    void initInstances();
    void updateNumInstances();
    void drawUI();

    static const int PrimGroupIndex = 0;
    static const int MaxNumInstances = 1024;
    static const int BoneTextureWidth = 1024;
    static const int BoneTextureHeight = MaxNumInstances/4 + 1;

    GfxDesc gfxDesc;
    CameraHelper camera;
    Id shader;
    OrbModel orbModel;
    DrawState drawState;
    DragonShader::vsParams vsParams;
    int numWantedInstances = 1;
    int numActiveInstances = 0;
    StaticArray<Id, MaxNumInstances> instances;

    // xxxx,yyyy,zzzz is transposed model matrix
    // boneInfo is position in bone texture
    VertexLayout instanceBufferLayout;
    struct InstanceVertex {
        float xxxx[4];
        float yyyy[4];
        float zzzz[4];
        float boneInfo[4];
    };
    InstanceVertex InstanceData[MaxNumInstances];

    double evalDuration = 0.0;
    double updateBoneTexDuration = 0.0;
    double updateInstBufDuration = 0.0;
    double drawDuration = 0.0;
    double frameDuration = 0.0;
    TimePoint frameLapTime;
    bool uiTextureWindowEnabled = false;
    int uiTexScale = 2;
    ImTextureID imguiBoneTextureId = nullptr;
};
OryolMain(Dragons);

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnInit() {
    IO::Setup(IODesc()
        .Assign("orb:", ORYOL_SAMPLE_URL)
        .FileSystem("http", HTTPFileSystem::Creator()));

    this->gfxDesc = GfxDesc()
        .Width(1024).Height(640)
        .SampleCount(4)
        .Title("Dragons")
        .HtmlTrackElementSize(true);
    Gfx::Setup(this->gfxDesc);
    AnimSetup animSetup;
    animSetup.MaxNumInstances = MaxNumInstances;
    animSetup.MaxNumActiveInstances = MaxNumInstances;
    animSetup.SkinMatrixTableWidth = BoneTextureWidth;
    animSetup.SkinMatrixTableHeight = BoneTextureHeight;
    Anim::Setup(animSetup);
    Input::Setup();
    IMUI::Setup();

    this->camera.Setup(false);
    this->camera.Center = glm::vec3(0.0f, 2.0f, -2.5f);
    this->camera.Distance = 20.0f;
    this->camera.Orbital = glm::vec2(glm::radians(45.0f), 0.0f);

    // setup the shader now, pipeline setup happens when model file has loaded
    this->shader = Gfx::CreateShader(DragonShader::Desc());

    // RGBA32F texture for the animated skeleton bones
    Id boneTexture = Gfx::CreateTexture(TextureDesc()
        .Type(TextureType::Texture2D)
        .Width(BoneTextureWidth)
        .Height(BoneTextureHeight)
        .NumMipMaps(1)
        .Format(PixelFormat::RGBA32F)
        .Usage(Usage::Stream)
        .MinFilter(TextureFilterMode::Nearest)
        .MagFilter(TextureFilterMode::Nearest)
        .WrapU(TextureWrapMode::ClampToEdge)
        .WrapV(TextureWrapMode::ClampToEdge));
    this->drawState.VSTexture[DragonShader::boneTex] = boneTexture;
    this->imguiBoneTextureId = IMUI::AllocImage();
    IMUI::BindImage(this->imguiBoneTextureId, boneTexture);

    // vertex-layout and -buffer for per-instance info
    this->instanceBufferLayout = VertexLayout()
        .EnableInstancing()
        .Add({
            { "instance0", VertexFormat::Float4 }, // transposes model matrix xxxx
            { "instance1", VertexFormat::Float4 }, // transposes model matrix yyyy
            { "instance2", VertexFormat::Float4 }, // transposes model matrix zzzz
            { "instance3", VertexFormat::Float4 }  // bone texture location
        });
    this->drawState.VertexBuffers[1] = Gfx::CreateBuffer(BufferDesc()
        .Size(MaxNumInstances * sizeof(InstanceVertex))
        .Usage(Usage::Stream));

    // load the dragon.orb file and add the first model instance,
    // the .txt extension is a hack so that github pages compresses the file
    this->loadModel("orb:dragon.orb.txt");

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnRunning() {
    if (!ImGui::GetIO().WantCaptureMouse) {
        this->camera.HandleInput();
    }
    this->camera.Center.z = -(this->numActiveInstances / 16) * 2.0f;
    this->camera.UpdateTransforms();
    if (this->orbModel.IsValid) {
        // add or remove instances
        if (this->numWantedInstances != this->numActiveInstances) {
            this->updateNumInstances();
        }

        // update the animation system
        Anim::NewFrame();
        for (int i = 0; i < this->numActiveInstances; i++) {
            Anim::AddActiveInstance(this->instances[i]);
        }
        TimePoint evalStart = Clock::Now();
        Anim::Evaluate(1.0/60.0);
        this->evalDuration = Clock::Since(evalStart).AsMilliSeconds();

        // upload animated skeleton bone info to GPU texture
        TimePoint updTexStart = Clock::Now();
        const AnimSkinMatrixInfo& boneInfo = Anim::SkinMatrixInfo();
        ImageContent imgContent;
        imgContent.Size[0][0] = boneInfo.SkinMatrixTableByteSize;
        imgContent.Pointer[0][0] = boneInfo.SkinMatrixTable;
        Gfx::UpdateTexture(this->drawState.VSTexture[DragonShader::boneTex], imgContent);
        this->updateBoneTexDuration = Clock::Since(updTexStart).AsMilliSeconds();

        // update the instance vertex buffer with bone texture locations
        TimePoint updInstStart = Clock::Now();
        int instIndex;
        for (instIndex = 0; instIndex < this->numActiveInstances; instIndex++) {
            const auto& shdInfo = boneInfo.InstanceInfos[instIndex].ShaderInfo;
            auto& vtx = this->InstanceData[instIndex];
            for (int i = 0; i < 4; i++) {
                vtx.boneInfo[i] = shdInfo[i];
            }
        }
        if (instIndex > 0) {
            Gfx::UpdateBuffer(this->drawState.VertexBuffers[1], this->InstanceData, sizeof(InstanceVertex)*instIndex);
        }
        this->updateInstBufDuration = Clock::Since(updInstStart).AsMilliSeconds();
    }

    // all dragons rendered in a single draw call via hardware instancing
    TimePoint drawStart = Clock::Now();
    Gfx::BeginPass(PassAction().Clear(0.2f, 0.3f, 0.5f, 1.0f));
    IMUI::NewFrame();
    this->drawUI();
    if (this->orbModel.IsValid) {
        Gfx::ApplyDrawState(this->drawState);
        this->vsParams.view_proj = this->camera.ViewProj;
        Gfx::ApplyUniformBlock(this->vsParams);
        Gfx::Draw(this->orbModel.Submeshes[0].PrimGroup, this->numActiveInstances);
    }
    ImGui::Render();
    Gfx::EndPass();
    Gfx::CommitFrame();
    this->drawDuration = Clock::Since(drawStart).AsMilliSeconds();
    this->frameDuration = Clock::LapTime(this->frameLapTime).AsMilliSeconds();

    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
Dragons::OnCleanup() {
    IMUI::Discard();
    Input::Discard();
    Anim::Discard();
    Gfx::Discard();
    IO::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
Dragons::loadModel(const Locator& loc) {
    // start loading the .orb file
    IO::Load(loc.Location(), [this](IO::LoadResult res) {
        if (OrbLoader::Load(res.Data, "model", this->orbModel)) {
            this->drawState.VertexBuffers[0] = this->orbModel.VertexBuffer;
            this->drawState.IndexBuffer = this->orbModel.IndexBuffer;
            this->vsParams.vtx_mag = this->orbModel.VertexMagnitude;
            this->drawState.Pipeline = Gfx::CreatePipeline(PipelineDesc()
                .Shader(this->shader)
                .Layout(0, this->orbModel.Layout)
                .Layout(1, this->instanceBufferLayout)
                .IndexType(this->orbModel.IndexType)
                .DepthWriteEnabled(true)
                .DepthCmpFunc(CompareFunc::LessEqual)
                .CullFaceEnabled(true)
                .SampleCount(this->gfxDesc.SampleCount())
                .ColorFormat(this->gfxDesc.ColorFormat())
                .DepthFormat(this->gfxDesc.DepthFormat()));
            this->initInstances();
        }
    },
    [](const URL& url, IOStatus::Code ioStatus) {
        // loading failed, just display an error message and carry on
        Log::Error("Failed to load file '%s' with '%s'\n", url.AsCStr(), IOStatus::ToString(ioStatus));
    });
}

//------------------------------------------------------------------------------
void
Dragons::initInstances() {
    // this pre-creates the max number of instances (these are not
    // evaluated or rendered until they are added per-frame
    // as active instances to the animation system
    o_assert_dbg(this->orbModel.IsValid);

    // setup the instance positions in the instance data vertex buffer,
    // we arange them in a triangle, pointing to the viewer
    int x = 0, y = 0;
    const float dist = 5.0f;
    for (int instIndex = 0; instIndex < MaxNumInstances; instIndex++) {
        glm::vec3 pos((x*dist) - (y*dist)*0.5f, 0.0f, -y*dist);
        glm::mat4 m = glm::translate(glm::mat4(), pos);
        for (int i = 0; i < 4; i++) {
            this->InstanceData[instIndex].xxxx[i] = m[i][0];
            this->InstanceData[instIndex].yyyy[i] = m[i][1];
            this->InstanceData[instIndex].zzzz[i] = m[i][2];
        }
        if (++x > y) {
            // next row
            x = 0;
            y++;
        }
    }
    // pre-allocate instances
    auto instSetup = AnimInstanceSetup::FromLibraryAndSkeleton(this->orbModel.AnimLib, this->orbModel.Skeleton);
    for (int instIndex = 0; instIndex < MaxNumInstances; instIndex++) {
        this->instances[instIndex] = Anim::Create(instSetup);
    }
}

//------------------------------------------------------------------------------
void
Dragons::updateNumInstances() {
    if (this->numWantedInstances < this->numActiveInstances) {
        // removal is easy...
        this->numActiveInstances = this->numWantedInstances;
    }
    else if (this->numWantedInstances > this->numActiveInstances) {
        const int numClips = 7;
        const int clips[numClips] = { 0, 1, 2, 3, 4, 5 };
        AnimJob job;
        job.TrackIndex = 0;
        for (int i = this->numActiveInstances; i < this->numWantedInstances; i++) {
            int r = rand();
            job.ClipIndex = clips[r % numClips];
            Anim::Play(this->instances[i], job);
        }
        this->numActiveInstances = this->numWantedInstances;
    }
}

//------------------------------------------------------------------------------
void
Dragons::drawUI() {
    if (this->orbModel.IsValid) {
        ImGui::SetNextWindowPos(ImVec2(10, 40), ImGuiSetCond_Once);
        if (ImGui::Begin("Dragons", nullptr)) {
            const int numBonesPerInst = Anim::Skeleton(this->orbModel.Skeleton).NumBones;
            const int numTrisPerInst = this->orbModel.Submeshes[0].PrimGroup.NumElements / 3;
            ImGui::SliderInt("num instances", &this->numWantedInstances, 1, MaxNumInstances);
            ImGui::Text("num bones:     %d\n",  numBonesPerInst * this->numActiveInstances);
            ImGui::Text("num triangles: %d\n", numTrisPerInst * this->numActiveInstances);
            ImGui::Text("anim system:  %.3f ms", this->evalDuration);
            ImGui::Text("texture upd:  %.3f ms", this->updateBoneTexDuration);
            ImGui::Text("instance upd: %.3f ms", this->updateInstBufDuration);
            ImGui::Text("render:       %.3f ms", this->drawDuration);
            ImGui::Text("frame time:   %.3f ms", this->frameDuration);
            if (ImGui::Button("Bone Texture")) {
                this->uiTextureWindowEnabled = !this->uiTextureWindowEnabled;
            }
        }
        ImGui::End();
        if (this->uiTextureWindowEnabled) {
            ImGui::SetNextWindowPos(ImVec2(10, 220), ImGuiSetCond_Once);
            ImGui::SetNextWindowSize(ImVec2(900, 400), ImGuiSetCond_Once);
            if (ImGui::Begin("Bone Texture", &this->uiTextureWindowEnabled)) {
                ImGui::InputInt("##scale", &this->uiTexScale);
                ImGui::SameLine();
                if (ImGui::Button("1x")) this->uiTexScale = 1;
                ImGui::SameLine();
                if (ImGui::Button("2x")) this->uiTexScale = 2;
                ImGui::SameLine();
                if (ImGui::Button("4x")) this->uiTexScale = 3;
                ImGui::BeginChild("##frame", ImVec2(0,0), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::Image(this->imguiBoneTextureId,
                    ImVec2(float(BoneTextureWidth*this->uiTexScale), float(BoneTextureHeight*this->uiTexScale)),
                    ImVec2(0, 0), ImVec2(1.0f, 1.0f));
                ImGui::End();
            }
            ImGui::End();
        }
    }
    else {
        const ImVec2 windowPos = ImVec2(ImGui::GetIO().DisplaySize.x/2, ImGui::GetIO().DisplaySize.y/2);
        const ImVec2 windowPivot = ImVec2(0.5f, 0.5f);
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
        ImGui::SetNextWindowBgAlpha(0.3f);
        if (ImGui::Begin("#loading", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
            ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|
            ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav))
        {
            ImGui::Text("Loading...");
            ImGui::End();
        }
    }
}


