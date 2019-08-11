//------------------------------------------------------------------------------
//  OrbViewer/Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "IMUI/IMUI.h"
#include "Anim/Anim.h"
#include "HttpFS/HTTPFileSystem.h"
#include "Common/OrbLoader.h"
#include "Common/CameraHelper.h"
#include "Common/Wireframe.h"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders.h"

using namespace Oryol;

class Main : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    struct Model {
        OrbModel orb;
        Id pipeline;
        Id animInstance;
        LambertShader::vsParams vsParams;
        glm::mat4 transform;
    };

    void drawUI();
    void drawMainWindow();
    void drawAnimControlWindow();
    void drawBoneTextureWindow();
    void loadModel(const Locator& loc);
    void drawModelDebug(const Model& model, const glm::mat4& modelMatrix);

    static const int BoneTextureWidth = 768;
    static const int BoneTextureHeight = 1;

    struct {
        bool freezeTime = false;
        float timeScale = 1.0f;
        bool meshEnabled = true;
        bool bindPoseEnabled = false;
        bool staticPoseEnabled = false;
        bool jointTrailsEnabled = false;
        bool animatedPoseEnabled = false;
        bool animWindowEnabled = false;
        bool textureWindowEnabled = false;
        AnimJob animJob;
    } ui;

    int frameIndex = 0;
    GfxSetup gfxSetup;
    Id shader;
    Id boneTexture;
    ImTextureID imguiBoneTextureId = nullptr;
    Model model;
    Wireframe wireframe;
    CameraHelper camera;
    Array<glm::mat4> dbgPose;
    static const uint32_t HistorySize = 128;
    StaticArray<Array<glm::vec3>, HistorySize> dbgHistory;
};
OryolMain(Main);

//------------------------------------------------------------------------------
AppState::Code
Main::OnInit() {
    IOSetup ioSetup;
    ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
    ioSetup.Assigns.Add("orb:", ORYOL_SAMPLE_URL);
    IO::Setup(ioSetup);

    this->gfxSetup = GfxSetup::WindowMSAA4(1024, 640, "Orb File Viewer");
    this->gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.1f, 0.1f, 0.2f, 1.0f));
    this->gfxSetup.HtmlTrackElementSize = true;
    Gfx::Setup(this->gfxSetup);
    AnimSetup animSetup;
    animSetup.SkinMatrixTableWidth = BoneTextureWidth;
    animSetup.SkinMatrixTableHeight = BoneTextureHeight;
    Anim::Setup(animSetup);
    Input::Setup();
    IMUI::Setup();
    this->camera.Setup(false);
    this->camera.Center = glm::vec3(0.0f, 1.0f, -2.5f);
    this->camera.Distance = 10.0f;
    this->camera.Orbital = glm::vec2(glm::radians(25.0f), glm::radians(-90.0f));
    this->wireframe.Setup(this->gfxSetup);

    // can setup the shader before loading any assets
    this->shader = Gfx::CreateResource(LambertShader::Setup());

    // RGBA32F texture for the animated skeleton bone info
    auto texSetup = TextureSetup::Empty2D(BoneTextureWidth, BoneTextureHeight, 1, PixelFormat::RGBA32F, Usage::Stream);
    texSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    texSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    texSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    this->boneTexture = Gfx::CreateResource(texSetup);
    this->imguiBoneTextureId = IMUI::AllocImage();
    IMUI::BindImage(this->imguiBoneTextureId, this->boneTexture);

    // load the dragon.orb file (the .txt extension is a hack
    // so that github pages compresses the file)
    this->loadModel("orb:dragon.orb.txt");

    // write something useful into the anim job triggered by UI
    this->ui.animJob.TrackIndex = 1;
    this->ui.animJob.DurationIsLoopCount = true;
    this->ui.animJob.Duration = 1.0f;
    this->ui.animJob.FadeIn = this->ui.animJob.FadeOut = 0.25f;

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnRunning() {
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        this->camera.Update();
    }
    this->wireframe.ViewProj = this->camera.ViewProj;

    // update the animation system
    if (this->model.orb.IsValid) {
        Anim::NewFrame();
        Anim::AddActiveInstance(this->model.animInstance);
        Anim::Evaluate(this->ui.freezeTime ? 0.0 : (1.0/60.0)*this->ui.timeScale);

        // upload bone info to GPU texture
        const AnimSkinMatrixInfo& boneInfo = Anim::SkinMatrixInfo();
        this->model.vsParams.skin_info = boneInfo.InstanceInfos[0].ShaderInfo;
        ImageDataAttrs imgAttrs;
        imgAttrs.NumFaces = 1;
        imgAttrs.NumMipMaps = 1;
        imgAttrs.Offsets[0][0] = 0;
        imgAttrs.Sizes[0][0] = boneInfo.SkinMatrixTableByteSize;
        Gfx::UpdateTexture(this->boneTexture, boneInfo.SkinMatrixTable, imgAttrs);
    }

    Gfx::BeginPass();
    if (this->model.orb.IsValid) {
        if (this->ui.meshEnabled) {
            DrawState drawState;
            drawState.Pipeline = this->model.pipeline;
            drawState.Mesh[0] = this->model.orb.Mesh;
            drawState.VSTexture[LambertShader::boneTex] = this->boneTexture;
            Gfx::ApplyDrawState(drawState);
            /*
            LambertShader::lightParams lightParams;
            lightParams.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            lightParams.lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.25f));
            Gfx::ApplyUniformBlock(lightParams);
            */
            this->model.vsParams.model = glm::mat4();
            this->model.vsParams.mvp = this->camera.ViewProj * this->model.vsParams.model;
            Gfx::ApplyUniformBlock(this->model.vsParams);
            for (const auto& subMesh : this->model.orb.Submeshes) {
                if (subMesh.Visible) {
                    //Gfx::ApplyUniformBlock(this->model.materials[subMesh.material].matParams);
                    Gfx::Draw(subMesh.PrimitiveGroupIndex);
                }
            }
        }
        this->drawModelDebug(this->model, glm::mat4());
    }
    this->wireframe.Render();
    this->drawUI();    
    Gfx::EndPass();
    Gfx::CommitFrame();
    if (!this->ui.freezeTime) {
        this->frameIndex++;
    }
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnCleanup() {
    this->wireframe.Discard();
    IMUI::Discard();
    Input::Discard();
    Anim::Discard();
    Gfx::Discard();
    IO::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
static bool getClipItem(void* data, int index, const char** name) {
    Main* self = (Main*)data;
    *name = Anim::Library(self->model.orb.AnimLib).Clips[index].Name.AsCStr();
    return true;
}

//------------------------------------------------------------------------------
void
Main::drawUI() {
    IMUI::NewFrame();
    if (this->model.orb.IsValid && this->model.orb.AnimLib.IsValid()) {
        this->drawMainWindow();
        if (this->ui.animWindowEnabled) {
            this->drawAnimControlWindow();
        }
        if (this->ui.textureWindowEnabled) {
            this->drawBoneTextureWindow();
        }
    }
    ImGui::Render();
}

//------------------------------------------------------------------------------
void
Main::drawMainWindow() {
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    if (ImGui::Begin("##main_window", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Checkbox("freeze time", &this->ui.freezeTime);
        ImGui::SliderFloat("time scale", &this->ui.timeScale, 0.001f, 2.0f);
        if (ImGui::Button("0.5x")) { this->ui.timeScale = 0.5f; }; ImGui::SameLine();
        if (ImGui::Button("1.0x")) { this->ui.timeScale = 1.0f; }; ImGui::SameLine();
        if (ImGui::Button("1.5x")) { this->ui.timeScale = 1.5f; }; ImGui::SameLine();
        if (ImGui::Button("2.0x")) { this->ui.timeScale = 2.0f; };
        ImGui::Checkbox("draw mesh", &this->ui.meshEnabled);
        ImGui::Checkbox("draw bind pose", &this->ui.bindPoseEnabled);
        ImGui::Checkbox("draw clip static pose", &this->ui.staticPoseEnabled);
        ImGui::Checkbox("draw animated pose", &this->ui.animatedPoseEnabled);
        ImGui::Checkbox("draw joint trails", &this->ui.jointTrailsEnabled);
        if (ImGui::Button("Anim Controls")) {
            this->ui.animWindowEnabled = !this->ui.animWindowEnabled;
        }
        if (ImGui::Button("Bone Texture")) {
            this->ui.textureWindowEnabled = !this->ui.textureWindowEnabled;
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void
Main::drawAnimControlWindow() {
    const float w = 720.0f;
    const float h = 230.0f;
    const AnimLibrary& lib = Anim::Library(this->model.orb.AnimLib);
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - h), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Once);
    if (ImGui::Begin("Anim Sequencer", &this->ui.animWindowEnabled)) {
        ImGui::BeginChild("Controls", ImVec2(210, -1), true);
        {
            const int numClips = lib.Clips.Size();
            ImGui::Combo("Clip", &this->ui.animJob.ClipIndex, getClipItem, this, numClips);
            ImGui::InputInt("Track", &this->ui.animJob.TrackIndex);
            ImGui::SliderFloat("Weight", &this->ui.animJob.MixWeight, 0.0f, 1.0f);
            ImGui::SliderFloat("FadeIn", &this->ui.animJob.FadeIn, 0.0f, 1.0f);
            ImGui::SliderFloat("FadeOut", &this->ui.animJob.FadeOut, 0.0f, 1.0f);
            ImGui::Checkbox("Duration/LoopCount", &this->ui.animJob.DurationIsLoopCount);
            ImGui::InputFloat(this->ui.animJob.DurationIsLoopCount ? "LoopCount##dur":"Duration##dur", &this->ui.animJob.Duration);
            if (ImGui::Button("Play")) {
                Anim::Play(this->model.animInstance, this->ui.animJob);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop All")) {
                Anim::StopAll(this->model.animInstance);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();

        // this draw the track sequencer visualzation
        ImGui::BeginChild("Tracks", ImVec2(-1, -1), true);
        {
            const ImVec2 clipMax = ImGui::GetWindowContentRegionMax();
            const float clipWidth = ImGui::GetWindowContentRegionWidth();
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const double t = Anim::CurrentTime();
            const float pixelsPerSecond = 40.0f;
            const float trackHeight = 16.0f;
            const float trackDist = 17.0f;
            const float playCursorX = 50.0f;
            const auto& inst = Anim::instance(this->model.animInstance);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            for (const auto& item : inst.sequencer.items) {
                float x0 = playCursorX + float((item.absStartTime - t) * pixelsPerSecond);
                float x1 = playCursorX + float((item.absEndTime - t) * pixelsPerSecond);
                float f0 = playCursorX + float((item.absFadeInTime - t) * pixelsPerSecond);
                float f1 = playCursorX + float((item.absFadeOutTime - t) * pixelsPerSecond);
                float y0 = (item.trackIndex * trackDist);
                float y1 = y0 + trackHeight;
                x0 = glm::clamp(x0, 0.0f, clipWidth);
                x1 = glm::clamp(x1, 0.0f, clipWidth);
                f0 = glm::clamp(f0, 0.0f, clipWidth);
                f1 = glm::clamp(f1, 0.0f, clipWidth);
                dl->AddRectFilled(ImVec2(p.x+f0,p.y+y0), ImVec2(p.x+f1, p.y+y1), 0xFF007700);
                if (f0 > x0) {
                    // fade-in triangle
                    dl->AddTriangleFilled(ImVec2(p.x+f0,p.y+y0), ImVec2(p.x+f0,p.y+y1), ImVec2(p.x+x0,p.y+y1), 0xFF007700);
                }
                if (f1 < x1) {
                    // fade-in triangle
                    dl->AddTriangleFilled(ImVec2(p.x+f1,p.y+y0), ImVec2(p.x+f1,p.y+y1), ImVec2(p.x+x1,p.y+y0), 0xFF007700);
                }
                dl->AddText(ImVec2(p.x+f0+5, p.y+y0+2), 0xFFFFFFFF, lib.Clips[item.clipIndex].Name.AsCStr());
            }
            dl->AddLine(ImVec2(p.x+playCursorX, p.y), ImVec2(p.x+playCursorX, p.y+clipMax.y), 0xFF00AAAA);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void
Main::drawBoneTextureWindow() {
    const float w = 800.0f;
    const float h = 64.0f;
    ImGui::SetNextWindowPos(ImVec2(5, ImGui::GetIO().DisplaySize.y - h), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Once);
    if (ImGui::Begin("Bone Texture", &this->ui.textureWindowEnabled)) {
        ImGui::Image(this->imguiBoneTextureId,
            ImVec2(float(BoneTextureWidth), float(BoneTextureHeight) * 4),
            ImVec2(0, 0), ImVec2(0.25f, 0.25f));
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void
Main::drawModelDebug(const Model& model, const glm::mat4& modelTransform) {
    const AnimSkeleton& skel = Anim::Skeleton(model.orb.Skeleton);
    const AnimLibrary& lib = Anim::Library(model.orb.AnimLib);
    const AnimClip& clip = lib.Clips[this->ui.animJob.ClipIndex];
    glm::vec3 t, s;
    glm::quat r;

    auto& wf = this->wireframe;
    wf.Model = modelTransform;

    /// the character bind pose
    if (this->ui.bindPoseEnabled) {
        wf.Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        for (int i = 0; i < skel.NumBones; i++) {
            const int parent = skel.ParentIndices[i];
            if (parent != -1) {
                wf.Line(skel.BindPose[i][3], skel.BindPose[parent][3]);
            }
        }
    }

    // draw current clip's static pose matrix
    if (this->ui.staticPoseEnabled) {
        o_assert(clip.Curves.Size() == skel.NumBones * 3);
        this->dbgPose.Clear();
        this->dbgPose.Reserve(skel.NumBones);
        wf.Color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        for (int i = 0; i < skel.NumBones; i++) {
            const AnimCurve& tCurve = clip.Curves[i*3 + 0];
            const AnimCurve& rCurve = clip.Curves[i*3 + 1];
            const AnimCurve& sCurve = clip.Curves[i*3 + 2];
            t = glm::vec3(tCurve.StaticValue[0], tCurve.StaticValue[1], tCurve.StaticValue[2]);
            r = glm::quat(rCurve.StaticValue[3], rCurve.StaticValue[0], rCurve.StaticValue[1], rCurve.StaticValue[2]);
            s = glm::vec3(sCurve.StaticValue[0], sCurve.StaticValue[1], sCurve.StaticValue[2]);
            const glm::mat4 tm = glm::translate(glm::mat4(), t);
            const glm::mat4 rm = glm::mat4_cast(r);
            const glm::mat4 sm = glm::scale(glm::mat4(), s);
            glm::mat4 m = tm * rm * sm;
            const int parent = skel.ParentIndices[i];
            if (parent != -1) {
                m = this->dbgPose[parent] * m;
                wf.Line(m[3], this->dbgPose[parent][3]);
            }
            this->dbgPose.Add(m);
        }
    }

    // draw current animation sampling/mixing result and record history
    const int histIndex = this->frameIndex % HistorySize;
    this->dbgHistory[histIndex].Clear();
    this->dbgHistory[histIndex].Reserve(skel.NumBones);
    this->dbgPose.Clear();
    wf.Color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    const Slice<float>& samples = Anim::Samples(model.animInstance);
    for (int boneIndex=0, i=0; boneIndex<skel.NumBones; boneIndex++, i+=10) {
        t = glm::vec3(samples[i+0], samples[i+1], samples[i+2]);
        r = glm::quat(samples[i+6], samples[i+3], samples[i+4], samples[i+5]);
        s = glm::vec3(samples[i+7], samples[i+8], samples[i+9]);
        const glm::mat4 tm = glm::translate(glm::mat4(), t);
        const glm::mat4 rm = glm::mat4_cast(r);
        const glm::mat4 sm = glm::scale(glm::mat4(), s);
        glm::mat4 m = tm * rm * sm;
        const int parent = skel.ParentIndices[boneIndex];
        if (parent != -1) {
            m = this->dbgPose[parent] * m;
            if (this->ui.animatedPoseEnabled) {
                wf.Line(m[3], this->dbgPose[parent][3]);
            }
        }
        this->dbgPose.Add(m);
        if (this->ui.jointTrailsEnabled) {
            this->dbgHistory[histIndex].Add(m[3]);
        }
    }

    // draw the bone history
    glm::vec3 move(0.0f, 0.0f, -0.1f);
    glm::vec4 fade(-1.0f/float(HistorySize), 0.0f, 0.0f, -1.0/float(HistorySize));
    for (uint32_t boneIndex = 0; boneIndex < (uint32_t)skel.NumBones; boneIndex++) {
        for (uint32_t i = 0; i < HistorySize-1; i++) {
            uint32_t hi0 = (histIndex - i) & (HistorySize-1);
            uint32_t hi1 = (hi0 - 1) & (HistorySize-1);
            glm::vec3 d0 = move * float(i);
            glm::vec3 d1 = move * float(i + 1);
            wf.Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) + fade * float(i);
            if (!this->dbgHistory[hi0].Empty() && !this->dbgHistory[hi1].Empty()) {
                wf.Line(this->dbgHistory[hi0][boneIndex] + d0,
                        this->dbgHistory[hi1][boneIndex] + d1);
            }
        }
    }
}

//------------------------------------------------------------------------------
void
Main::loadModel(const Locator& loc) {
    // start loading the .orb file
    IO::Load(loc.Location(), [this](IO::LoadResult res) {
        auto& orb = this->model.orb;
        if (OrbLoader::Load(res.Data, "model", orb)) {
            orb.Submeshes[0].Visible = true;

            auto pipSetup = PipelineSetup::FromLayoutAndShader(orb.MeshSetup.Layout, this->shader);
            pipSetup.DepthStencilState.DepthWriteEnabled = true;
            pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
            pipSetup.RasterizerState.CullFaceEnabled = true;
            pipSetup.RasterizerState.SampleCount = this->gfxSetup.SampleCount;
            pipSetup.BlendState.ColorFormat = this->gfxSetup.ColorFormat;
            pipSetup.BlendState.DepthFormat = this->gfxSetup.DepthFormat;
            this->model.pipeline = Gfx::CreateResource(pipSetup);
            this->model.vsParams.vtx_mag = orb.VertexMagnitude;

            if (orb.AnimLib.IsValid()) {
                auto instSetup = AnimInstanceSetup::FromLibraryAndSkeleton(orb.AnimLib, orb.Skeleton);
                this->model.animInstance = Anim::Create(instSetup);
                AnimJob job;
                job.ClipIndex = 0;
                job.TrackIndex = 0;
                Anim::Play(model.animInstance, job);
            }
        }
    },
    [](const URL& url, IOStatus::Code ioStatus) {
        // loading failed, just display an error message and carry on
        Log::Error("Failed to load file '%s' with '%s'\n", url.AsCStr(), IOStatus::ToString(ioStatus));
    });
}
