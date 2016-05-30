//------------------------------------------------------------------------------
//  Emu/Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "IO/IO.h"
#include "HTTP/HTTPFileSystem.h"
#include "Core/Time/Clock.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
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

    // interactive elements
    static const int PowerOnButton = 0;
    static const int ResetButton   = 1;
    static const int BaseDevice    = 2;
    static const int Screen        = 3;
    static const int Junost        = 4;
    static const int Keyboard      = 5;
    static const int TapeDeck      = 6;
    static const int StartJungle   = 10;

    void handleInput();
    void tooltip(const DisplayAttrs& disp, const char* str);

    ClearState clearState = ClearState::ClearAll(glm::vec4(0.4f, 0.6f, 0.8f, 1.0f), 1.0f, 0);
    TimePoint lapTime;
    KC85Emu kc85Emu;
    SceneRenderer scene;
    RayCheck rayChecker;
    glm::mat4 kcModelMatrix;
    CameraHelper camera;
};
OryolMain(KC853App);

//------------------------------------------------------------------------------
AppState::Code
KC853App::OnInit() {

    // setup Oryol modules
    IOSetup ioSetup;
    ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
    ioSetup.Assigns.Add("kcc:", ORYOL_SAMPLE_URL);
    IO::Setup(ioSetup);
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 512, "Emu");
    Gfx::Setup(gfxSetup);
    Input::Setup();
    Input::BeginCaptureText();
    Dbg::Setup();
    Dbg::SetTextScale(glm::vec2(2.0f));

    // setup the scene and ray collide checker
    this->scene.Setup(gfxSetup);
    this->rayChecker.Setup(gfxSetup);
    this->rayChecker.Add(Screen,        glm::vec3(50, 14, 38), glm::vec3(77, 35, 41));
    this->rayChecker.Add(Junost,        glm::vec3(40, 10, 42), glm::vec3(79, 39, 65));
    this->rayChecker.Add(PowerOnButton, glm::vec3(44, 1, 37), glm::vec3(49, 5, 41));
    this->rayChecker.Add(ResetButton,   glm::vec3(51, 1, 37), glm::vec3(56, 5, 41));
    this->rayChecker.Add(BaseDevice,    glm::vec3(40, 1, 39), glm::vec3(79, 8, 65));
    this->rayChecker.Add(StartJungle,   glm::vec3(14, 0, 28), glm::vec3(33, 6, 40));
    this->rayChecker.Add(Keyboard,      glm::vec3(44, 1, 18), glm::vec3(75, 2, 33));
    this->rayChecker.Add(TapeDeck,      glm::vec3(14, 0, 41), glm::vec3(33, 6, 54));

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
KC853App::OnRunning() {
    this->camera.Update();
    this->handleInput();

    // update KC85 emu
    this->kc85Emu.Update(Clock::LapTime(this->lapTime));

    // render the voxel scene and emulator screen
    Gfx::ApplyDefaultRenderTarget(this->clearState);
    this->scene.Render(this->camera.ViewProj);
    this->kc85Emu.Render(this->camera.ViewProj * this->kcModelMatrix);
//    this->rayChecker.RenderDebug(this->camera.ViewProj);
    Dbg::DrawTextBuffer();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
KC853App::OnCleanup() {
    Input::Discard();
    IO::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
KC853App::handleInput() {
    const Mouse& mouse = Input::Mouse();
    if (mouse.Attached) {
        glm::vec2 screenSpaceMousePos = mouse.Position;
        const DisplayAttrs& disp = Gfx::DisplayAttrs();
        screenSpaceMousePos.x /= float(disp.FramebufferWidth);
        screenSpaceMousePos.y /= float(disp.FramebufferHeight);
        glm::mat4 invView = glm::inverse(this->camera.View);
        int hit = this->rayChecker.Test(screenSpaceMousePos, invView, this->camera.InvProj);
        switch (hit) {
            case PowerOnButton:
                if (mouse.ButtonDown(Mouse::LMB)) {
                    this->kc85Emu.TogglePower();
                }
                if (this->kc85Emu.SwitchedOn()) {
                    this->tooltip(disp, "SWITCH KC85/3 OFF");
                }
                else {
                    this->tooltip(disp, "SWITCH KC85/3 ON");
                }
                break;
            case ResetButton:
                if (mouse.ButtonDown(Mouse::LMB)) {
                    this->kc85Emu.Reset();
                }
                this->tooltip(disp, "RESET KC85/3");
                break;
            case BaseDevice:
                this->tooltip(disp, "A KC85/3, EAST GERMAN 8-BIT COMPUTER");
                break;
            case Screen:
                // prevent Junost tooltip when mouse is over screen
                break;
            case Junost:
                this->tooltip(disp, "A YUNOST 402B, SOVIET TV");
                break;
            case Keyboard:
                this->tooltip(disp, "TYPE SOMETHING!");
                break;
            case TapeDeck:
                this->tooltip(disp, "AN 'LCR-C DATA' TAPE DECK");
                break;
            case StartJungle:
                if (mouse.ButtonDown(Mouse::LMB)) {
                    this->kc85Emu.StartGame("Jungle");
                }
                this->tooltip(disp, "PLAY JUNGLE!");
                break;
            default:
                if (!this->kc85Emu.SwitchedOn()) {
                    this->tooltip(disp, "EXPLORE!");
                }
                break;
        }
    }
}

//------------------------------------------------------------------------------
void
KC853App::tooltip(const DisplayAttrs& disp, const char* str) {
    o_assert_dbg(str);
    
    // center the text
    int center = (disp.FramebufferWidth / 16) / 2;
    int len = int(std::strlen(str));
    int posX = center - (len/2);
    if (posX < 0) {
        posX = 0;
    }
    Dbg::CursorPos(uint8_t(posX), 2);
    Dbg::Print(str);
}
