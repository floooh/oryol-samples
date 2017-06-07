//------------------------------------------------------------------------------
//  OrbViewer/Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "IMUI/IMUI.h"
#include "HttpFS/HTTPFileSystem.h"
#include "OrbFileFormat.h"
#include "Core/Containers/InlineArray.h"
#include <glm/mat4x4.hpp>

using namespace Oryol;

class Main : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    void drawUI();

    struct Material {
        Id pipeline;
        Id colorMap;
        // vsParams
        // fsParams
    };
    struct Mesh {
        int material = 0;
        PrimitiveGroup primGroup;
    };
    struct Model {
        InlineArray<Material, 8> materials;
        InlineArray<Mesh, 8> meshes;
    };
    struct Instance {
        int model = 0;
        glm::mat4 transform;
    };

    Array<Model> models;
    Array<Instance> instances;
};
OryolMain(Main);

//------------------------------------------------------------------------------
AppState::Code
Main::OnInit() {
    IOSetup ioSetup;
    ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
    ioSetup.Assigns.Add("orb:", ORYOL_SAMPLE_URL);
    IO::Setup(ioSetup);

    auto gfxSetup = GfxSetup::WindowMSAA4(800, 512, "Orb File Viewer");
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.3f, 0.3f, 0.4f, 1.0f));
    Gfx::Setup(gfxSetup);

    Input::Setup();
    IMUI::Setup();

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnRunning() {
    Gfx::BeginPass();
    this->drawUI();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
Main::OnCleanup() {
    IMUI::Discard();
    Input::Discard();
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
