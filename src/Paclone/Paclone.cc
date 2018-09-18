//------------------------------------------------------------------------------
//  Paclone.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "Dbg/Dbg.h"
#include "Sound/Sound.h"
#include "Assets/Gfx/FullscreenQuadBuilder.h"
#include "canvas.h"
#include "game.h"
#include "shaders.h"
#include "glm/common.hpp"

using namespace Oryol;
using namespace Paclone;

#if ORYOL_RASPBERRYPI || ORYOL_IOS || ORYOL_ANDROID
#define USE_CRTEFFECT (0)
#else
#define USE_CRTEFFECT (1)
#endif

class PacloneApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();

private:
    Direction getInput();
    void applyViewPort();

    Id canvasPass;
    DrawState canvasDrawState;

    canvas spriteCanvas;
    game gameState;
    sound sounds;
    int tick = 0;
    int viewPortX = 0;
    int viewPortY = 0;
    int viewPortW = 0;
    int viewPortH = 0;
    Direction input = NoDirection;
    
    const int canvasWidth = Width * 8;
    const int canvasHeight = Height * 8;
    const int dispWidth = canvasWidth * 2;
    const int dispHeight = canvasHeight * 2;
};
OryolMain(PacloneApp);

//------------------------------------------------------------------------------
AppState::Code
PacloneApp::OnInit() {
    
    this->tick = 0;
    Gfx::Setup(GfxDesc()
        .Width(dispWidth).Height(dispHeight)
        .Title("Oryol Pacman Clone Sample")
        .HtmlTrackElementSize(true));
    Input::Setup();
    Sound::Setup(SoundSetup());
    Dbg::Setup(DbgDesc().DepthFormat(PixelFormat::None));
    
    // setup a offscreen render target, copy-shader and texture block
    this->canvasDrawState.FSTexture[0] = Gfx::CreateTexture(TextureDesc()
        .RenderTarget(true)
        .Type(TextureType::Texture2D)
        .Width(canvasWidth)
        .Height(canvasHeight)
        .MinFilter(TextureFilterMode::Linear)
        .MagFilter(TextureFilterMode::Linear)
        .WrapU(TextureWrapMode::ClampToEdge)
        .WrapV(TextureWrapMode::ClampToEdge));
    this->canvasPass = Gfx::CreatePass(PassDesc()
        .ColorAttachment(0, this->canvasDrawState.FSTexture[0]));

    auto fsq = FullscreenQuadBuilder()
        .FlipV(Gfx::QueryFeature(GfxFeature::OriginTopLeft))
        .Build();
    this->canvasDrawState.VertexBuffers[0] = Gfx::CreateBuffer(fsq.VertexBufferDesc);
    #if USE_CRTEFFECT
    Id shd = Gfx::CreateShader(CRTShader::Desc());
    #else
    Id shd = Gfx::CreateShader(NoCRTShader::Desc());
    #endif
    this->canvasDrawState.Pipeline = Gfx::CreatePipeline(PipelineDesc(fsq.PipelineDesc).Shader(shd));

    // setup canvas and game state
    this->spriteCanvas.Setup(PixelFormat::RGBA8, Width, Height, 8, 8, NumSprites);
    this->sounds.CreateSoundEffects();
    this->gameState.Init(&this->spriteCanvas, &this->sounds);

    return App::OnInit();
}

//------------------------------------------------------------------------------
void
PacloneApp::applyViewPort() {
    float aspect = float(Width) / float(Height);
    const int fbWidth = Gfx::Width();
    const int fbHeight = Gfx::Height();
    this->viewPortY = 0;
    this->viewPortH = fbHeight;
    this->viewPortW = (const int) (fbHeight * aspect);
    this->viewPortX = (fbWidth - viewPortW) / 2;
    Gfx::ApplyViewPort(this->viewPortX, this->viewPortY, this->viewPortW, this->viewPortH);
}

//------------------------------------------------------------------------------
AppState::Code
PacloneApp::OnRunning() {

    Direction input = this->getInput();
    this->gameState.Update(this->tick, &this->spriteCanvas, &this->sounds, input);

    // render into offscreen render target
    Gfx::BeginPass(this->canvasPass);
    this->spriteCanvas.Render();
    Dbg::DrawTextBuffer(canvasWidth, canvasHeight);
    Gfx::EndPass();
    
    // copy offscreen render target into backbuffer
    Gfx::BeginPass();
    this->applyViewPort();
    Gfx::ApplyDrawState(this->canvasDrawState);
    Gfx::Draw(0, 4);
    Gfx::EndPass();
    Gfx::CommitFrame();
    this->tick++;

    // continue running or quit?
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
PacloneApp::OnCleanup() {
    this->gameState.Cleanup();
    this->spriteCanvas.Discard();
    Dbg::Discard();
    Sound::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
Direction
PacloneApp::getInput() {
    if (Input::KeyboardAttached()) {
        if (Input::KeyPressed(Key::Left))       this->input = Left;
        else if (Input::KeyPressed(Key::Right)) this->input = Right;
        else if (Input::KeyPressed(Key::Up))    this->input = Up;
        else if (Input::KeyPressed(Key::Down))  this->input = Down;
    }
    if (Input::GamepadAttached(0)) {
        const float deadZone = 0.3f;
        float hori = Input::GamepadAxisValue(0, GamepadAxis::LeftStickHori) +
                     Input::GamepadAxisValue(0, GamepadAxis::RightStickHori);
        float vert = Input::GamepadAxisValue(0, GamepadAxis::LeftStickVert) +
                     Input::GamepadAxisValue(0, GamepadAxis::RightStickVert);
        if (hori < -deadZone) this->input = Left;
        else if (hori > deadZone) this->input = Right;
        else if (vert < -deadZone) this->input = Up;
        else if (vert > deadZone) this->input = Down;
        if (Input::GamepadButtonPressed(0, GamepadButton::DPadLeft)) this->input = Left;
        else if (Input::GamepadButtonPressed(0, GamepadButton::DPadRight)) this->input = Right;
        else if (Input::GamepadButtonPressed(0, GamepadButton::DPadUp)) this->input = Up;
        else if (Input::GamepadButtonPressed(0, GamepadButton::DPadDown)) this->input = Down;
    }
    if (Input::TouchpadAttached() || Input::MouseAttached()) {
        if (Input::TouchTapped() || Input::TouchPanning() || Input::MouseButtonPressed(MouseButton::Left)) {
            glm::vec2 pos;
            if (Input::TouchTapped() || Input::TouchPanning()) {
                pos = Input::TouchPosition(0);
            }
            else {
                pos = Input::MousePosition();
            }
            const Int2 pacmanPos = this->gameState.PacmanPos();
            const float relX = float(pacmanPos.x) / float(this->spriteCanvas.CanvasWidth());
            const float relY = float(pacmanPos.y) / float(this->spriteCanvas.CanvasHeight());
            const float px = this->viewPortX + this->viewPortW * relX;
            const float py = this->viewPortY + this->viewPortH * relY;
            const float dx = pos.x - px;
            const float dy = pos.y - py;
            if (glm::abs(dx) > glm::abs(dy)) {
                // it's a horizontal movement
                this->input = dx < 0.0f ? Left : Right;
            }
            else {
                // it's a vertical movement
                this->input = dy < 0.0f ? Up : Down;
            }
        }
    }
    return this->input;
}
