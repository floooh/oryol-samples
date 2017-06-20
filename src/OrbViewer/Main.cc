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
#include "OrbFileFormat.h"
#include "Core/Containers/InlineArray.h"
#include "OrbFile.h"
#include "Common/CameraHelper.h"
#include "Wireframe.h"
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

    struct Material {
        //LambertShader::matParams matParams;
        // FIXME: textures would go here
    };
    struct SubMesh {
        int material = 0;
        int primGroupIndex = 0;
        bool visible = false;
    };
    struct Model {
        bool isValid = false;
        Id mesh;
        Id pipeline;
        Id skeleton;
        Id animLib;
        Id animInstance;
        LambertShader::vsParams vsParams;
        InlineArray<Material, 8> materials;
        InlineArray<SubMesh, 8> subMeshes;
        glm::mat4 transform;
    };

    void drawUI();
    void drawMainWindow();
    void drawAnimControlWindow();
    void drawBoneTextureWindow();
    void loadModel(const Locator& loc);
    void drawModelDebug(const Model& model, const glm::mat4& modelMatrix);

    static const int BoneTextureWidth = 1024;
    static const int BoneTextureHeight = 128;

    struct {
        bool meshEnabled = true;
        bool bindPoseEnabled = false;
        bool staticPoseEnabled = false;
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
};
OryolMain(Main);

//------------------------------------------------------------------------------
AppState::Code
Main::OnInit() {
    IOSetup ioSetup;
    ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
//    ioSetup.Assigns.Add("orb:", ORYOL_SAMPLE_URL);
ioSetup.Assigns.Add("orb:", "http://localhost:8000/");
    IO::Setup(ioSetup);

    this->gfxSetup = GfxSetup::WindowMSAA4(1024, 640, "Orb File Viewer");
    this->gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.3f, 0.3f, 0.4f, 1.0f));
    Gfx::Setup(this->gfxSetup);
    AnimSetup animSetup;
    animSetup.SkinMatrixTableWidth = BoneTextureWidth;
    animSetup.SkinMatrixTableHeight = BoneTextureHeight;
    Anim::Setup(animSetup);
    Input::Setup();
    IMUI::Setup();
    this->camera.Setup(false);
    this->camera.Center = glm::vec3(0.0f, 1.0f, 0.0f);
    this->camera.Distance = 10.0f;
    this->camera.Orbital = glm::vec2(glm::radians(25.0f), glm::radians(0.0f));
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

    // load the dragon.orb file
    this->loadModel("orb:dragon.orb");

    // write something useful into the anim job triggered by UI
    this->ui.animJob.TrackIndex = 1;
    this->ui.animJob.DurationIsLoopCount = true;
    this->ui.animJob.Duration = 1.0f;
    this->ui.animJob.FadeIn = this->ui.animJob.FadeOut = 0.2f;

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnRunning() {
    if (!ImGui::IsMouseHoveringAnyWindow()) {
        this->camera.Update();
    }
    this->wireframe.ViewProj = this->camera.ViewProj;

    // update the animation system
    if (this->model.isValid) {
        Anim::NewFrame();
        Anim::AddActiveInstance(this->model.animInstance);
        Anim::Evaluate(1.0 / 60.0);

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

    // FIXME: change to instance rendering
    Gfx::BeginPass();
    if (this->model.isValid) {
        if (this->ui.meshEnabled) {
            DrawState drawState;
            drawState.Pipeline = this->model.pipeline;
            drawState.Mesh[0] = this->model.mesh;
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
            for (const auto& subMesh : this->model.subMeshes) {
                if (subMesh.visible) {
                    //Gfx::ApplyUniformBlock(this->model.materials[subMesh.material].matParams);
                    Gfx::Draw(subMesh.primGroupIndex);
                }
            }
        }
        this->drawModelDebug(this->model, glm::mat4());
    }
    this->wireframe.Render();
    this->drawUI();    
    Gfx::EndPass();
    Gfx::CommitFrame();
    this->frameIndex++;
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
    *name = Anim::Library(self->model.animLib).Clips[index].Name.AsCStr();
    return true;
}

//------------------------------------------------------------------------------
void
Main::drawUI() {
    IMUI::NewFrame();
    if (this->model.isValid && this->model.animLib.IsValid()) {
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
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiSetCond_Once);
    if (ImGui::Begin("##main_window", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Checkbox("draw mesh", &this->ui.meshEnabled);
        ImGui::Checkbox("draw bind pose", &this->ui.bindPoseEnabled);
        ImGui::Checkbox("draw clip static pose", &this->ui.staticPoseEnabled);
        ImGui::Checkbox("draw animated pose", &this->ui.animatedPoseEnabled);
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
    const float w = 210.0f;
    const float h = 220.0f;
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - h), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiSetCond_Once);
    if (ImGui::Begin("##anim_ctrl", &this->ui.animWindowEnabled)) {
        const int numClips = Anim::Library(this->model.animLib).Clips.Size();
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
    ImGui::End();
}

//------------------------------------------------------------------------------
void
Main::drawBoneTextureWindow() {
    const float w = 800.0f;
    const float h = 64.0f;
    ImGui::SetNextWindowPos(ImVec2(5, ImGui::GetIO().DisplaySize.y - h), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiSetCond_Once);
    if (ImGui::Begin("Bone Texture", &this->ui.textureWindowEnabled)) {
        ImGui::Image(this->imguiBoneTextureId,
            ImVec2(float(BoneTextureWidth), float(BoneTextureHeight)),
            ImVec2(0, 0), ImVec2(0.25f, 0.25f));
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void
Main::drawModelDebug(const Model& model, const glm::mat4& modelTransform) {
    const AnimSkeleton& skel = Anim::Skeleton(model.skeleton);
    const AnimLibrary& lib = Anim::Library(this->model.animLib);
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
            o_assert_dbg(tCurve.Format == AnimCurveFormat::Float3);
            const AnimCurve& rCurve = clip.Curves[i*3 + 1];
            o_assert_dbg(rCurve.Format == AnimCurveFormat::Quaternion);
            const AnimCurve& sCurve = clip.Curves[i*3 + 2];
            o_assert_dbg(sCurve.Format == AnimCurveFormat::Float3);
            t = glm::vec3(tCurve.StaticValue);
            r = glm::quat(rCurve.StaticValue.w, rCurve.StaticValue.x, rCurve.StaticValue.y, rCurve.StaticValue.z);
            s = glm::vec3(sCurve.StaticValue);
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

    // draw current animation sampling/mixing result
    if (this->ui.animatedPoseEnabled) {
        this->dbgPose.Clear();
        wf.Color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        const Slice<float>& samples = Anim::Samples(this->model.animInstance);
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
                wf.Line(m[3], this->dbgPose[parent][3]);
            }
            this->dbgPose.Add(m);
        }
    }
}

//------------------------------------------------------------------------------
void
Main::loadModel(const Locator& loc) {
    // start loading the .n3o file
    IO::Load(loc.Location(), [this](IO::LoadResult res) {
        // parse the .n3o file
        OrbFile orb;
        if (!orb.Parse(res.Data.Data(), res.Data.Size())) {
            Log::Error("Failed to parse .orb file '%s'\n", res.Url.AsCStr());
            return;
        }

        // setup new model and its associated graphics resources

        // one mesh for entire model
        auto meshSetup = orb.MakeMeshSetup();
        this->model.mesh = Gfx::CreateResource(meshSetup, res.Data.Data(), res.Data.Size());

        // one pipeline state object for all materials
        auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, this->shader);
        pipSetup.DepthStencilState.DepthWriteEnabled = true;
        pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
        pipSetup.RasterizerState.CullFaceEnabled = true;
        pipSetup.RasterizerState.SampleCount = this->gfxSetup.SampleCount;
        pipSetup.BlendState.ColorFormat = this->gfxSetup.ColorFormat;
        pipSetup.BlendState.DepthFormat = this->gfxSetup.DepthFormat;
        this->model.pipeline = Gfx::CreateResource(pipSetup);

        // submeshes link materials to mesh primitive groups
        for (int i = 0; i < orb.Meshes.Size(); i++) {
            SubMesh subMesh;
            subMesh.material = orb.Meshes[i].Material;
            subMesh.primGroupIndex = i;
            this->model.subMeshes.Add(subMesh);
        }
        this->model.subMeshes[1].visible = true;

        // materials hold shader uniform blocks and textures
        for (int i = 0; i < orb.Materials.Size(); i++) {
            Material mat;
            // FIXME: setup textures here
            //mat.matParams.diffuseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            this->model.materials.Add(mat);
        }

        // character stuff
        if (orb.HasCharacter()) {
            this->model.skeleton = Anim::Create(orb.MakeSkeletonSetup("skeleton"));
            this->model.animLib = Anim::Create(orb.MakeAnimLibSetup("animlib"));
            this->model.animInstance = Anim::Create(AnimInstanceSetup::FromLibraryAndSkeleton(this->model.animLib, this->model.skeleton));
            orb.CopyAnimKeys(this->model.animLib);
            AnimJob job;
            job.ClipIndex = 0;
            job.TrackIndex = 0;
            Anim::Play(model.animInstance, job);
        }
        model.isValid = true;
    },
    [](const URL& url, IOStatus::Code ioStatus) {
        // loading failed, just display an error message and carry on
        Log::Error("Failed to load file '%s' with '%s'\n", url.AsCStr(), IOStatus::ToString(ioStatus));
    });
}
