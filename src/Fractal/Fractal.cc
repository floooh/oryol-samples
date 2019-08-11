//------------------------------------------------------------------------------
//  Fractal.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IMUI/IMUI.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "shaders.h"

using namespace Oryol;

class FractalApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();
    
private:
    enum Type : int {
        Mandelbrot = 0,
        Julia,
        NumTypes
    };

    /// reset all fractal states to their default
    void reset();
    /// zoom-out to initial rectangle
    void zoomOut();
    /// draw the ui
    void drawUI();
    /// set current bounds rectangle (either Mandelbrot or Julia context)
    void setBounds(Type type, const glm::vec4& bounds);
    /// get current bounds (either Mandelbrot or Julia context)
    glm::vec4 getBounds(Type type) const;
    /// convert mouse pos to fractal coordinate system pos
    glm::vec2 convertPos(float x, float y, const glm::vec4& bounds) const;
    /// update the fractal's area rect
    void updateFractalRect(float x0, float y0, float x1, float y1);
    /// update the Julia set position from a mouse position
    void updateJuliaPos(float x, float y);
    /// re-create offscreen render-target if window size has changed (FIXME)
    void recreateRenderTargets(const DisplayAttrs& attrs);

    ResourceLabel offscreenLabel;
    Id offscreenPass[2];
    Id offscreenRenderTarget[2];
    DrawState dispDrawState;
    DisplayShader::fsParams dispFSParams;
    int frameIndex = 0;
    bool clearFlag = true;
    bool dragStarted = false;
    ImVec2 dragStartPos;
    Type fractalType = Mandelbrot;
    struct {
        DrawState drawState;
        Mandelbrot::vsParams vsParams;
    } mandelbrot;
    struct {
        DrawState drawState;
        Julia::vsParams vsParams;
        Julia::fsParams fsParams;
    } julia;
};
OryolMain(FractalApp);

//------------------------------------------------------------------------------
AppState::Code
FractalApp::OnRunning() {

    // reset current fractal state if requested
    if (this->clearFlag) {
        this->clearFlag = false;
        Gfx::BeginPass(offscreenPass[0], PassAction::Clear(glm::vec4(0.0f)));
        Gfx::EndPass();
        Gfx::BeginPass(offscreenPass[1], PassAction::Clear(glm::vec4(0.0f)));
        Gfx::EndPass();
    }

    this->frameIndex++;
    const int index0 = this->frameIndex % 2;
    const int index1 = (this->frameIndex + 1) % 2;
    const Id& curPass = this->offscreenPass[index0];
    const Id& curTexture = this->offscreenRenderTarget[index1];

    // render next fractal iteration
    Gfx::BeginPass(curPass, PassAction::Load());
    if (Mandelbrot == this->fractalType) {
        this->mandelbrot.drawState.FSTexture[Mandelbrot::tex] = curTexture;
        Gfx::ApplyDrawState(this->mandelbrot.drawState);
        Gfx::ApplyUniformBlock(this->mandelbrot.vsParams);
    }
    else {
        this->julia.drawState.FSTexture[Julia::tex] = curTexture;
        Gfx::ApplyDrawState(this->julia.drawState);
        Gfx::ApplyUniformBlock(this->julia.vsParams);
        Gfx::ApplyUniformBlock(this->julia.fsParams);
    }
    Gfx::Draw();
    Gfx::EndPass();

    // map fractal state to display
    Gfx::BeginPass(PassAction::DontCare());
    this->dispDrawState.FSTexture[DisplayShader::tex] = this->offscreenRenderTarget[index0];
    Gfx::ApplyDrawState(this->dispDrawState);
    Gfx::ApplyUniformBlock(this->dispFSParams);
    Gfx::Draw();

    this->drawUI();
    Gfx::EndPass();
    Gfx::CommitFrame();
    
    // continue running or quit?
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
FractalApp::OnInit() {
    auto gfxSetup = GfxSetup::Window(800, 512, "Fractal Sample");
    gfxSetup.HtmlTrackElementSize = true;
    Gfx::Setup(gfxSetup);
    Gfx::Subscribe([this](const GfxEvent& event) {
        if (event.Type == GfxEvent::DisplayModified) {
            this->recreateRenderTargets(event.DisplayAttrs);
        }
    });

    Input::Setup();
    IMUI::Setup();

    // ImGui colors
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    const ImVec4 grey(1.0f, 1.0f, 1.0f, 0.5f);
    style.Colors[ImGuiCol_TitleBg] = style.Colors[ImGuiCol_TitleBgActive] = grey;
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 1.0f, 1.0f, 0.20f);
    style.Colors[ImGuiCol_Button] = grey;

    // a fullscreen quad mesh that's reused several times
    auto fsqSetup = MeshSetup::FullScreenQuad(Gfx::QueryFeature(GfxFeature::OriginTopLeft));
    Id fsq = Gfx::CreateResource(fsqSetup);
    this->mandelbrot.drawState.Mesh[0] = fsq;
    this->julia.drawState.Mesh[0] = fsq;
    fsqSetup = MeshSetup::FullScreenQuad(true);
    this->dispDrawState.Mesh[0] = Gfx::CreateResource(fsqSetup);

    // draw state for rendering the final result to screen
    Id shd = Gfx::CreateResource(DisplayShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(fsqSetup.Layout, shd);
    ps.RasterizerState.CullFaceEnabled = false;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::Always;
    this->dispDrawState.Pipeline = Gfx::CreateResource(ps);

    // setup 2 ping-poing fp32 render targets which hold the current fractal state,
    // and the texture blocks which use reference these render targets
    this->recreateRenderTargets(Gfx::DisplayAttrs());

    // setup mandelbrot state
    shd = Gfx::CreateResource(Mandelbrot::Setup());
    ps = PipelineSetup::FromLayoutAndShader(fsqSetup.Layout, shd);
    ps.RasterizerState.CullFaceEnabled = false;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::Always;
    ps.BlendState.ColorFormat = PixelFormat::RGBA32F;
    ps.BlendState.DepthFormat = PixelFormat::None;
    this->mandelbrot.drawState.Pipeline = Gfx::CreateResource(ps);

    // setup julia state
    shd = Gfx::CreateResource(Julia::Setup());
    ps = PipelineSetup::FromLayoutAndShader(fsqSetup.Layout, shd);
    ps.RasterizerState.CullFaceEnabled = false;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::Always;
    ps.BlendState.ColorFormat = PixelFormat::RGBA32F;
    ps.BlendState.DepthFormat = PixelFormat::None;
    this->julia.drawState.Pipeline = Gfx::CreateResource(ps);

    // initialize fractal states
    this->reset();

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
FractalApp::OnCleanup() {
    IMUI::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
FractalApp::drawUI() {
    const DisplayAttrs& dispAttrs = Gfx::DisplayAttrs();
    const float fbWidth = (float) dispAttrs.FramebufferWidth;
    const float fbHeight = (float) dispAttrs.FramebufferHeight;
    this->clearFlag = false;
    ImVec2 mousePos = ImGui::GetMousePos();

    IMUI::NewFrame();

    // draw the controls window
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Once);
    ImGui::Begin("Controls", nullptr, ImVec2(300, 230));
    ImGui::BulletText("mouse-drag a rectangle to zoom in");
    ImGui::BulletText("click into Mandelbrot to render\nJulia set at that point");
    ImGui::Separator();
    if (Julia == this->fractalType) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.6f));
        if (ImGui::Button("<< Back")) {
            this->clearFlag = true;
            this->fractalType = Mandelbrot;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    this->clearFlag |= ImGui::Button("Redraw");
    if (ImGui::SameLine(), ImGui::Button("Zoomout")) {
        this->clearFlag |= true;
        this->zoomOut();
    }
    if (ImGui::SameLine(), ImGui::Button("Reset")) {
        this->clearFlag |= true;
        this->reset();
    }
    ImGui::SliderFloat("Colors", &this->dispFSParams.numColors, 2.0f, 256.0f);
    ImGui::Separator();
    glm::vec2 curPos = this->convertPos(mousePos.x, mousePos.y, this->getBounds(this->fractalType));
    ImGui::Text("X: %f, Y: %f\n", curPos.x, curPos.y);
    ImGui::Separator();
    if (Mandelbrot == this->fractalType) {
        ImGui::Text("Type: Mandelbrot\n"
                    "x0: %f\ny0: %f\nx0: %f\ny1: %f",
                    this->mandelbrot.vsParams.rect.x,
                    this->mandelbrot.vsParams.rect.y,
                    this->mandelbrot.vsParams.rect.w,
                    this->mandelbrot.vsParams.rect.z);
    }
    else {
        ImGui::Text("Type: Julia\n"
                    "Pos: %f, %f\n"
                    "x0: %f\ny0: %f\nx0: %f\ny1: %f",
                    this->julia.fsParams.juliaPos.x,
                    this->julia.fsParams.juliaPos.y,
                    this->julia.vsParams.rect.x,
                    this->julia.vsParams.rect.y,
                    this->julia.vsParams.rect.w,
                    this->julia.vsParams.rect.z);
    }

    // handle dragging
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && ImGui::IsMouseClicked(0)) {
        this->dragStarted = true;
        this->dragStartPos = ImGui::GetMousePos();
    }
    if (this->dragStarted) {
        const ImVec2& mousePos = ImGui::GetMousePos();
        if (ImGui::IsMouseReleased(0)) {
            this->dragStarted = false;
            if ((this->dragStartPos.x != mousePos.x) && (this->dragStartPos.y != mousePos.y)) {
                this->updateFractalRect(this->dragStartPos.x, this->dragStartPos.y, mousePos.x, mousePos.y);
            }
            else {
                if (Mandelbrot == this->fractalType) {
                    // single click in Mandelbrot: render a Julia set from that point
                    this->updateJuliaPos(mousePos.x, mousePos.y);
                    this->fractalType = Julia;
                    this->zoomOut();
                }
            }
            this->clearFlag = true;
        }
        else {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(ImVec2(0, 0), ImVec2(fbWidth, fbHeight));
            drawList->AddRect(this->dragStartPos, ImGui::GetMousePos(), 0xFFFFFFFF);
            drawList->PopClipRect();
        }
    }

    ImGui::End();
    ImGui::Render();
}

//------------------------------------------------------------------------------
void
FractalApp::zoomOut() {
    this->setBounds(this->fractalType, glm::vec4(-2.0, -2.0, 2.0, 2.0));
}

//------------------------------------------------------------------------------
void
FractalApp::reset() {
    this->fractalType = Mandelbrot;
    this->dispFSParams.numColors = 8.0f;
    this->zoomOut();
}

//------------------------------------------------------------------------------
void
FractalApp::setBounds(Type type, const glm::vec4& bounds) {
    if (Mandelbrot == type) {
        this->mandelbrot.vsParams.rect = bounds;
    }
    else {
        this->julia.vsParams.rect = bounds;
    }
}

//------------------------------------------------------------------------------
glm::vec4
FractalApp::getBounds(Type type) const {
    if (Mandelbrot == type) {
        return this->mandelbrot.vsParams.rect;
    }
    else {
        return this->julia.vsParams.rect;
    }
}

//------------------------------------------------------------------------------
glm::vec2
FractalApp::convertPos(float x, float y, const glm::vec4& bounds) const {
    // convert mouse pos to fractal real/imaginary pos
    const DisplayAttrs& attrs = Gfx::DisplayAttrs();
    const float fbWidth = (float) attrs.FramebufferWidth;
    const float fbHeight = (float) attrs.FramebufferHeight;
    glm::vec2 rel = glm::vec2(x, y) / glm::vec2(fbWidth, fbHeight);
    return glm::vec2(bounds.x + ((bounds.z - bounds.x) * rel.x),
                     bounds.y + ((bounds.w - bounds.y) * rel.y));
}

//------------------------------------------------------------------------------
void
FractalApp::updateFractalRect(float x0, float y0, float x1, float y1) {

    if ((x0 == x1) || (y0 == y1)) return;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    glm::vec4 bounds = this->getBounds(this->fractalType);
    glm::vec2 topLeft = this->convertPos(x0, y0, bounds);
    glm::vec2 botRight = this->convertPos(x1, y1, bounds);
    this->setBounds(this->fractalType, glm::vec4(topLeft, botRight));
}

//------------------------------------------------------------------------------
void
FractalApp::updateJuliaPos(float x, float y) {
    this->julia.fsParams.juliaPos = this->convertPos(x, y, this->getBounds(this->fractalType));
}

//------------------------------------------------------------------------------
void
FractalApp::recreateRenderTargets(const DisplayAttrs& attrs) {
    // window size has changed, re-create render targets
    Log::Info("(re-)create render targets\n");

    // first destroy previous render targets
    if (ResourceLabel::Invalid != this->offscreenLabel) {
        Gfx::DestroyResources(this->offscreenLabel);
    }
    this->offscreenLabel = Gfx::PushResourceLabel();
    auto offscreenRTSetup = TextureSetup::RenderTarget2D(
        attrs.FramebufferWidth, attrs.FramebufferHeight,
        PixelFormat::RGBA32F, PixelFormat::None);
    offscreenRTSetup.Sampler.MinFilter = TextureFilterMode::Nearest;
    offscreenRTSetup.Sampler.MagFilter = TextureFilterMode::Nearest;
    for (int i = 0; i < 2; i++) {
        this->offscreenRenderTarget[i] = Gfx::CreateResource(offscreenRTSetup);
        auto passSetup = PassSetup::From(this->offscreenRenderTarget[i]);
        this->offscreenPass[i] = Gfx::CreateResource(passSetup);
    }
    this->clearFlag = true;

    Gfx::PopResourceLabel();
}
