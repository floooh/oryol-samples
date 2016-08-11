//------------------------------------------------------------------------------
//  ImGuiAdvanced.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IMUI/IMUI.h"
#include "IO/IO.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Core/Time/Clock.h"
#include "EmuCommon/KC85Emu.h"
#include "font.h"
#include "scene_shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace Oryol;

class ImguiAdvancedApp : public App {
public:
    /// on init frame method
    virtual AppState::Code OnInit();
    /// on running frame method
    virtual AppState::Code OnRunning();
    /// on cleanup frame method
    virtual AppState::Code OnCleanup();

    TimePoint lapTime;    
    struct scene_t {
        void setup(const GfxSetup& gfxSetup);
        void render();
        glm::mat4 computeMVP(const glm::vec3& pos);

        Id renderTarget;
        ClearState clearState = ClearState::ClearAll(glm::vec4(0.25f, 0.45f, 0.65f, 1.0f));
        DrawState drawState;
        SceneShader::Params params;
        glm::mat4 view;
        glm::mat4 proj;
        float angleX = 0.0f;
        float angleY = 0.0f;

        ImTextureID imguiTexId = nullptr;
    } scene;
    struct emu_t {
        void setup(const GfxSetup& gfxSetup);
        void render(Duration frameTime);

        KC85Emu kc853;
        KC85Emu kc854;
        KC85Emu kc87;
        ImTextureID kc853TexId = nullptr;
        ImTextureID kc854TexId = nullptr;
        ImTextureID kc87TexId = nullptr;

    } emu;
};
OryolMain(ImguiAdvancedApp);

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnInit() {
    auto gfxSetup = GfxSetup::Window(800, 680, "Oryol ImGui Advanced");
    Gfx::Setup(gfxSetup);
    Input::Setup();

    // add custom fonts which have been embedded as C arrays
    IMUISetup uiSetup;
    uiSetup.AddFontFromMemory(dump_roboto_regular, sizeof(dump_roboto_regular), 20.0f);
    uiSetup.AddFontFromMemory(dump_font, sizeof(dump_font), 20.0f);
    IMUI::Setup(uiSetup);

    // initialize the 3D scene and emulators
    this->scene.setup(gfxSetup);
    this->emu.setup(gfxSetup);
    this->lapTime = Clock::Now();

    return AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnRunning() {

    // update offscreen render targets
    Duration frameTime = Clock::LapTime(this->lapTime);
    this->scene.render();
    this->emu.render(frameTime);

    // render the UI, with TTF fonts and Oryol textures as ImGui images
    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.25f, 0.45f, 0.75f, 1.0f)));
    IMUI::NewFrame();
    {
        // a window with offscreen-rendered 3D scene
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Once);
        ImGui::SetNextWindowSize(ImVec2(340,300), ImGuiSetCond_FirstUseEver);
        ImGui::PushFont(IMUI::Font(0));
        ImGui::Begin("PackedNormals Demo", nullptr, ImGuiWindowFlags_NoResize);
        ImGui::Image(this->scene.imguiTexId, ImVec2(320, 256));
        ImGui::End();
        ImGui::PopFont();

        // a window with a KC85/3 emulator
        ImGui::SetNextWindowPos(ImVec2(400, 10), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340,320), ImGuiSetCond_FirstUseEver);
        ImGui::PushFont(IMUI::Font(1));
        ImGui::Begin("KC85/3 EMULATOR", nullptr, ImGuiWindowFlags_NoResize);
        ImGui::Image(this->emu.kc853TexId, ImVec2(320, 256));
        ImGui::PushFont(nullptr);
        if (ImGui::Button("On/Off")) {
            this->emu.kc853.TogglePower();
            this->emu.kc853.TogglePower();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            this->emu.kc853.Reset();
        }
        ImGui::PopFont();
        if (ImGui::IsWindowFocused()) {
            this->emu.kc853.keyboard.hasInputFocus = true;
            this->emu.kc854.keyboard.hasInputFocus = false;
            this->emu.kc87.keyboard.hasInputFocus  = false;
        }
        ImGui::End();
        ImGui::PopFont();

        // a window with a KC85/4 emulator
        ImGui::SetNextWindowPos(ImVec2(10, 340), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340,320), ImGuiSetCond_FirstUseEver);
        ImGui::PushFont(IMUI::Font(1));
        ImGui::Begin("KC85/4 EMULATOR", nullptr, ImGuiWindowFlags_NoResize);
        ImGui::Image(this->emu.kc854TexId, ImVec2(320, 256));
        ImGui::PushFont(nullptr);
        if (ImGui::Button("On/Off")) {
            this->emu.kc854.TogglePower();
            this->emu.kc854.TogglePower();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            this->emu.kc854.Reset();
        }
        ImGui::PopFont();
        if (ImGui::IsWindowFocused()) {
            this->emu.kc853.keyboard.hasInputFocus = false;
            this->emu.kc854.keyboard.hasInputFocus = true;
            this->emu.kc87.keyboard.hasInputFocus  = false;
        }
        ImGui::End();
        ImGui::PopFont();

        // a window with a KC87 emulator
        ImGui::SetNextWindowPos(ImVec2(400, 360), ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 260), ImGuiSetCond_FirstUseEver);
        ImGui::PushFont(IMUI::Font(1));
        ImGui::Begin("KC87 EMULATOR", nullptr, ImGuiWindowFlags_NoResize);
        ImGui::Image(this->emu.kc87TexId, ImVec2(320, 192));
        ImGui::PushFont(nullptr);
        if (ImGui::Button("On/Off")) {
            this->emu.kc87.TogglePower();
            this->emu.kc87.TogglePower();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            this->emu.kc87.Reset();
        }
        ImGui::PopFont();
        if (ImGui::IsWindowFocused()) {
            this->emu.kc853.keyboard.hasInputFocus = false;
            this->emu.kc854.keyboard.hasInputFocus = false;
            this->emu.kc87.keyboard.hasInputFocus  = true;
        }
        ImGui::End();
        ImGui::PopFont();
    }
    ImGui::Render();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnCleanup() {
    this->emu.kc853.Discard();
    this->emu.kc854.Discard();
    this->emu.kc87.Discard();
    IMUI::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::scene_t::setup(const GfxSetup& gfxSetup) {

    // create an offscreen render target
    auto rtSetup = TextureSetup::RenderTarget(320, 256);
    rtSetup.ColorFormat = PixelFormat::RGBA8;
    rtSetup.DepthFormat = PixelFormat::DEPTH;
    rtSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    rtSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    rtSetup.Sampler.MagFilter = TextureFilterMode::Linear;
    rtSetup.Sampler.MinFilter = TextureFilterMode::Linear;
    rtSetup.ClearHint = this->clearState;
    this->renderTarget = Gfx::CreateResource(rtSetup);

    // associate the offscreen render target with a new Imgui texture id
    this->imguiTexId = IMUI::AllocImage();
    IMUI::BindImage(this->imguiTexId, this->renderTarget);

    // shape mesh, shader and pipeline state object
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder.Box(1.0f, 1.0f, 1.0f, 4)
        .Sphere(0.75f, 36, 20)
        .Cylinder(0.5f, 1.5f, 36, 10)
        .Torus(0.3f, 0.5f, 20, 36)
        .Plane(1.5f, 1.5f, 10);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());
    Id shd = Gfx::CreateResource(SceneShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.BlendState.ColorFormat = rtSetup.ColorFormat;
    ps.BlendState.DepthFormat = rtSetup.DepthFormat;
    this->drawState.Pipeline = Gfx::CreateResource(ps);

    float fbWidth = (const float) rtSetup.Width;
    float fbHeight = (const float) rtSetup.Height;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.01f, 100.0f);
    this->view = glm::mat4();
}

//------------------------------------------------------------------------------
glm::mat4
ImguiAdvancedApp::scene_t::computeMVP(const glm::vec3& pos) {
    glm::mat4 modelTform = glm::translate(glm::mat4(), pos);
    modelTform = glm::rotate(modelTform, this->angleX, glm::vec3(1.0f, 0.0f, 0.0f));
    modelTform = glm::rotate(modelTform, this->angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    return this->proj * this->view * modelTform;
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::scene_t::render() {
    this->angleY += 0.01f;
    this->angleX += 0.02f;
    Gfx::ApplyRenderTarget(this->renderTarget, this->clearState);
    Gfx::ApplyDrawState(this->drawState);
    static const glm::vec3 positions[] = {
        glm::vec3(-1.0, 1.0f, -6.0f),
        glm::vec3(1.0f, 1.0f, -6.0f),
        glm::vec3(-2.0f, -1.0f, -6.0f),
        glm::vec3(+2.0f, -1.0f, -6.0f),
        glm::vec3(0.0f, -1.0f, -6.0f)
    };
    int primGroupIndex = 0;
    for (const auto& pos : positions) {
        this->params.ModelViewProjection = this->computeMVP(pos);
        Gfx::ApplyUniformBlock(this->params);
        Gfx::Draw(primGroupIndex++);
    }
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::emu_t::setup(const GfxSetup& gfxSetup) {
    this->kc853.Setup(gfxSetup, YAKC::device::kc85_3, YAKC::os_rom::caos_3_1);
    this->kc853.switchOnDelayFrames = 1;
    this->kc853TexId = IMUI::AllocImage();
    IMUI::BindImage(this->kc853TexId, this->kc853.draw.irmTexture320x256);

    this->kc854.Setup(gfxSetup, YAKC::device::kc85_4, YAKC::os_rom::caos_4_2);
    this->kc854.switchOnDelayFrames = 1;
    this->kc854TexId = IMUI::AllocImage();
    IMUI::BindImage(this->kc854TexId, this->kc854.draw.irmTexture320x256);

    this->kc87.Setup(gfxSetup, YAKC::device::kc87, YAKC::os_rom::kc87_os_2);
    this->kc87.switchOnDelayFrames = 1;
    this->kc87TexId = IMUI::AllocImage();
    IMUI::BindImage(this->kc87TexId, this->kc87.draw.irmTexture320x192);
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::emu_t::render(Duration frameTime) {
    this->kc853.Update(frameTime);
    this->kc854.Update(frameTime);
    this->kc87.Update(frameTime);
    this->kc853.Render(glm::mat4(), true);
    this->kc854.Render(glm::mat4(), true);
    this->kc87.Render(glm::mat4(), true);
}
