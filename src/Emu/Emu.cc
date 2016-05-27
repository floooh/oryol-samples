//------------------------------------------------------------------------------
//  Emu.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"

using namespace Oryol;

class EmuApp : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    ClearState clearState = ClearState::ClearAll(glm::vec4(0.2f, 0.5f, 0.8f, 1.0f), 1.0f, 0);
};
OryolMain(EmuApp);

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnInit() {
    auto gfxSetup = GfxSetup::Window(800, 600, "Emu");
    Gfx::Setup(gfxSetup);

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnRunning() {
    Gfx::ApplyDefaultRenderTarget(this->clearState);
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
EmuApp::OnCleanup() {
    Gfx::Discard();
    return App::OnCleanup();
}