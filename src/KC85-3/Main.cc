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
#include "SceneRenderer.h"
#include "RayCheck.h"
#include "glm/gtc/matrix_transform.hpp"

using namespace Oryol;

class KC853App : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    ClearState clearState = ClearState::ClearAll(glm::vec4(0.4f, 0.6f, 0.8f, 1.0f), 1.0f, 0);
    TimePoint lapTime;
    KC85Emu kc85Emu;
    SceneRenderer scene;
    RayCheck rayChecker;
    glm::mat4 kcModelMatrix;
    CameraHelper camera;
    glm::mat4 invProj;
};
OryolMain(KC853App);

//------------------------------------------------------------------------------
AppState::Code
KC853App::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 512, "Emu");
    Gfx::Setup(gfxSetup);
    Input::Setup();
    Input::BeginCaptureText();
    this->scene.Setup(gfxSetup);
    this->rayChecker.Setup(gfxSetup);

    // setup the camera helper
    this->camera.Setup(false);
    this->camera.Center = glm::vec3(63.0f, 25.0f, 40.0f);
    this->camera.MaxCamDist = 200.0f;
    this->camera.Distance = 80.0f;
    this->camera.Orbital = glm::vec2(glm::radians(10.0f), glm::radians(160.0f));
    this->invProj = glm::inverse(this->camera.Proj);

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
KC853App::OnRunning() {
    this->camera.Update();

    if (Input::Mouse().Attached) {
        glm::vec2 screenSpaceMousePos = Input::Mouse().Position;
        const DisplayAttrs& disp = Gfx::DisplayAttrs();
        screenSpaceMousePos.x /= float(disp.FramebufferWidth);
        screenSpaceMousePos.y /= float(disp.FramebufferHeight);
        glm::mat4 invView = glm::inverse(this->camera.View);
        int hit = this->rayChecker.Test(screenSpaceMousePos, invView, this->invProj);
    }

    // update KC85 emu
    this->kc85Emu.Update(Clock::LapTime(this->lapTime));

    // render the voxel scene and emulator screen
    Gfx::ApplyDefaultRenderTarget(this->clearState);
    this->scene.Render(this->camera.ViewProj);
    this->kc85Emu.Render(this->camera.ViewProj * this->kcModelMatrix);
    this->rayChecker.RenderDebug(this->camera.ViewProj);
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
KC853App::OnCleanup() {
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

