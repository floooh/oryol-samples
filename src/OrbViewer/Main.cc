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
        Id mesh;
        Id pipeline;
        Id skeleton;
        InlineArray<Material, 8> materials;
        InlineArray<SubMesh, 8> subMeshes;
    };
    struct Instance {
        int model = 0;
        glm::mat4 transform;
    };

    void drawUI();
    void loadModel(const Locator& loc);
    void drawModelDebug(const Model& model, const glm::mat4& modelMatrix);

    GfxSetup gfxSetup;
    Id shader;
    Array<Model> models;
    Array<Instance> instances;
    Wireframe wireframe;
    CameraHelper camera;
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

    this->gfxSetup = GfxSetup::WindowMSAA4(800, 512, "Orb File Viewer");
    this->gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.3f, 0.3f, 0.4f, 1.0f));
    Gfx::Setup(this->gfxSetup);
    Anim::Setup(AnimSetup());
    Input::Setup();
    IMUI::Setup();
    this->camera.Setup(false);
    this->wireframe.Setup(this->gfxSetup);

    // can setup the shader before loading any assets
    this->shader = Gfx::CreateResource(LambertShader::Setup());

    // load the dragon.orb file
    this->loadModel("orb:dragon.orb");

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnRunning() {
    this->camera.Update();
    this->wireframe.ViewProj = this->camera.ViewProj;
    Gfx::BeginPass();
    this->drawUI();

    // FIXME: change to instance rendering
    if (!this->models.Empty()) {
        const auto& model = this->models[0];
        DrawState drawState;
        drawState.Pipeline = model.pipeline;
        drawState.Mesh[0] = model.mesh;
        Gfx::ApplyDrawState(drawState);
        /*
        LambertShader::lightParams lightParams;
        lightParams.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        lightParams.lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.25f));
        Gfx::ApplyUniformBlock(lightParams);
        */
        LambertShader::vsParams vsParams;
        vsParams.model = glm::mat4();
        vsParams.mvp = this->camera.ViewProj * vsParams.model;
        Gfx::ApplyUniformBlock(vsParams);
        for (const auto& subMesh : model.subMeshes) {
            if (subMesh.visible) {
                //Gfx::ApplyUniformBlock(model.materials[subMesh.material].matParams);
                Gfx::Draw(subMesh.primGroupIndex);
            }
        }
        this->drawModelDebug(model, glm::mat4());
    }
    this->wireframe.Render();
    Gfx::EndPass();
    Gfx::CommitFrame();
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
void
Main::drawUI() {
    IMUI::NewFrame();
    // FIXME
    ImGui::Render();
}

//------------------------------------------------------------------------------
void
Main::drawModelDebug(const Model& model, const glm::mat4& modelTransform) {
    auto& wf = this->wireframe;
    wf.Model = modelTransform;

    // a rectangle at the bottom
    wf.Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    wf.Line(glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(+1.0f, 0.0f, -1.0f));
    wf.Line(glm::vec3(+1.0f, 0.0f, -1.0f), glm::vec3(+1.0f, 0.0f, +1.0f));
    wf.Line(glm::vec3(+1.0f, 0.0f, +1.0f), glm::vec3(-1.0f, 0.0f, +1.0f));
    wf.Line(glm::vec3(-1.0f, 0.0f, +1.0f), glm::vec3(-1.0f, 0.0f, -1.0f));

    /// the character bind pose
    wf.Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    const AnimSkeleton& skel = Anim::Skeleton(model.skeleton);
    for (int i = 0; i < skel.NumBones; i++) {
        const int parent = skel.ParentIndices[i];
        if (parent != -1) {
            wf.Line(skel.BindPose[i][3], skel.BindPose[parent][3]);
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
        this->models.Add();
        auto& model = this->models.Back();

        // one mesh for entire model
        auto meshSetup = orb.MakeMeshSetup();
        model.mesh = Gfx::CreateResource(meshSetup, res.Data.Data(), res.Data.Size());

        // one pipeline state object for all materials
        auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, this->shader);
        pipSetup.DepthStencilState.DepthWriteEnabled = true;
        pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
        pipSetup.RasterizerState.CullFaceEnabled = true;
        pipSetup.RasterizerState.SampleCount = this->gfxSetup.SampleCount;
        pipSetup.BlendState.ColorFormat = this->gfxSetup.ColorFormat;
        pipSetup.BlendState.DepthFormat = this->gfxSetup.DepthFormat;
        model.pipeline = Gfx::CreateResource(pipSetup);

        // submeshes link materials to mesh primitive groups
        for (int i = 0; i < orb.Meshes.Size(); i++) {
            SubMesh subMesh;
            subMesh.material = orb.Meshes[i].Material;
            subMesh.primGroupIndex = i;
            model.subMeshes.Add(subMesh);
        }
        model.subMeshes[1].visible = true;

        // materials hold shader uniform blocks and textures
        for (int i = 0; i < orb.Materials.Size(); i++) {
            Material mat;
            // FIXME: setup textures here
            //mat.matParams.diffuseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            model.materials.Add(mat);
        }

        // character stuff
        if (orb.HasCharacter()) {
            model.skeleton = Anim::Create(orb.MakeSkeletonSetup("skeleton"));
        }
    },
    [](const URL& url, IOStatus::Code ioStatus) {
        // loading failed, just display an error message and carry on
        Log::Error("Failed to load file '%s' with '%s'\n", url.AsCStr(), IOStatus::ToString(ioStatus));
    });
}
