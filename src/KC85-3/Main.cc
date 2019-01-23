//------------------------------------------------------------------------------
//  Emu/Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "IO/IO.h"
#include "HttpFS/HTTPFileSystem.h"
#include "Core/Time/Clock.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Input/Input.h"
#include "Common/CameraHelper.h"
#include "SceneRenderer.h"
#include "RayCheck.h"
#include "Emu.h"
#include "glm/gtc/matrix_transform.hpp"
#include <cstring>

using namespace Oryol;

class KC853App : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    // interactive elements
    enum {
        PowerOnButton,
        ResetButton,
        BaseDevice,
        Screen,
        Junost,
        Keyboard,
        TapeDeck,
        Jungle,
        Digger,
        Pengo,
        Boulderdash,
        Cave
    };

    void handleInput();
    void tooltip(const DisplayAttrs& disp, const char* str);

    TimePoint lapTime;
    SceneRenderer scene;
    RayCheck rayChecker;
    glm::mat4 kcModelMatrix;
    CameraHelper camera;
    Emu emu;
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
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.4f, 0.6f, 0.8f, 1.0f));
    gfxSetup.HtmlTrackElementSize = true;
    Gfx::Setup(gfxSetup);
    Input::Setup();
    Dbg::Setup();
    Dbg::TextScale(2.0f, 2.0f);

    // setup the scene and ray collide checker
    this->scene.Setup(gfxSetup);
    this->rayChecker.Setup(gfxSetup);
    this->rayChecker.Add(Screen,        glm::vec3(51, 15, 39), glm::vec3(78, 36, 42));
    this->rayChecker.Add(Junost,        glm::vec3(41, 11, 43), glm::vec3(80, 40, 66));
    this->rayChecker.Add(PowerOnButton, glm::vec3(45, 2, 38), glm::vec3(50, 6, 42));
    this->rayChecker.Add(ResetButton,   glm::vec3(51, 2, 38), glm::vec3(57, 6, 42));
    this->rayChecker.Add(BaseDevice,    glm::vec3(41, 2, 40), glm::vec3(80, 9, 66));
    this->rayChecker.Add(Keyboard,      glm::vec3(45, 2, 19), glm::vec3(76, 3, 34));
    this->rayChecker.Add(Jungle,        glm::vec3(27, 2, 18), glm::vec3(35, 3, 23));
    this->rayChecker.Add(Digger,        glm::vec3(20, 8, 47), glm::vec3(28, 9, 52));
    this->rayChecker.Add(Pengo,         glm::vec3(16, 2, 14), glm::vec3(24, 3, 19));
    this->rayChecker.Add(Boulderdash,   glm::vec3(22, 2, 5),  glm::vec3(30, 3, 10));
    this->rayChecker.Add(Cave,          glm::vec3(15, 1, 29), glm::vec3(34, 7, 41));
    this->rayChecker.Add(TapeDeck,      glm::vec3(15, 1, 42), glm::vec3(34, 7, 55));

    // setup the camera helper
    this->camera.Setup(false);
    this->camera.Center = glm::vec3(63.0f, 25.0f, 40.0f);
    this->camera.MaxCamDist = 200.0f;
    this->camera.Distance = 80.0f;
    this->camera.Orbital = glm::vec2(glm::radians(10.0f), glm::radians(160.0f));

    // setup the KC emulator
    this->emu.Setup(gfxSetup);
    this->lapTime = Clock::Now();

    glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    m = glm::translate(m, glm::vec3(-64.0f, 40.0f, 26.0f));
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
    this->emu.Tick(Clock::LapTime(this->lapTime));

    // render the voxel scene and emulator screen
    Gfx::BeginPass();
    this->scene.Render(this->camera.ViewProj);
    this->emu.Render(this->camera.ViewProj * this->kcModelMatrix);
//    this->rayChecker.RenderDebug(this->camera.ViewProj);
    Dbg::DrawTextBuffer();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
KC853App::OnCleanup() {
    this->emu.Discard();
    Input::Discard();
    IO::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
KC853App::handleInput() {
    if (Input::MouseAttached()) {
        glm::vec2 screenSpaceMousePos = Input::MousePosition();
        const bool lmb = Input::MouseButtonDown(MouseButton::Left);
        const DisplayAttrs& disp = Gfx::DisplayAttrs();
        screenSpaceMousePos.x /= float(disp.FramebufferWidth);
        screenSpaceMousePos.y /= float(disp.FramebufferHeight);
        glm::mat4 invView = glm::inverse(this->camera.View);
        int hit = this->rayChecker.Test(screenSpaceMousePos, invView, this->camera.InvProj);
        switch (hit) {
            case PowerOnButton:
                if (lmb) {
                    this->emu.TogglePower();
                }
                if (this->emu.SwitchedOn()) {
                    this->tooltip(disp, "SWITCH KC85/3 OFF");
                }
                else {
                    this->tooltip(disp, "SWITCH KC85/3 ON");
                }
                break;
            case ResetButton:
                if (lmb) {
                    this->emu.Reset();
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
            case Jungle:
                if (lmb) {
                    this->emu.StartGame("kcc:jungle.kcc");
                }
                this->tooltip(disp, "PLAY JUNGLE!");
                break;
            case Digger:
                if (lmb) {
                    this->emu.StartGame("kcc:digger3.tap");
                }
                this->tooltip(disp, "PLAY DIGGER!");
                break;
            case Pengo:
                if (lmb) {
                    this->emu.StartGame("kcc:pengo.kcc");
                }
                this->tooltip(disp, "PLAY PENGO!");
                break;
            case Boulderdash:
                if (lmb) {
                    this->emu.StartGame("kcc:boulder3.tap");
                }
                this->tooltip(disp, "PLAY BOULDERDASH!");
                break;
            case Cave:
                if (lmb) {
                    this->emu.StartGame("kcc:cave.kcc");
                }
                this->tooltip(disp, "PLAY CAVE!");
                break;
            default:
                if (!this->emu.SwitchedOn()) {
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
