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
    virtual AppState::Code OnInit();
    virtual AppState::Code OnRunning();
    virtual AppState::Code OnCleanup();
    void setupImguiStyle();

    TimePoint lapTime;

    // the 3D scene, render a bunch of shapes to an offscreen render target
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

    // state for the 3 emulators
    struct emu_t {
        void setup(const GfxSetup& gfxSetup);
        void render(Duration frameTime);
        enum {
            KC85_3 = 0,
            KC85_4 = 1,
            KC87 = 2,
            NumEmus,
        };
        KC85Emu kc[NumEmus];
        ImTextureID imgId[NumEmus];
    } emu;
};
OryolMain(ImguiAdvancedApp);

// initial emulator window position, size and title
struct { float x, y, w, h; const char* title; } winAttrs[ImguiAdvancedApp::emu_t::NumEmus] = {
    { 400.0f,  10.0f, 360.0f, 340.0f, "KC85/3 EMULATOR" },
    {  10.0f, 340.0f, 360.0f, 340.0f, "KC85/4 EMULATOR" },
    { 400.0f, 360.0f, 360.0f, 280.0f, "KC87 EMULATOR" },
};

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnInit() {
    auto gfxSetup = GfxSetup::Window(800, 680, "Oryol ImGui Advanced");
    Gfx::Setup(gfxSetup);
    Input::Setup();

    // add custom fonts which have been embedded as C arrays
    IMUISetup uiSetup;
    uiSetup.AddFontFromMemory(dump_roboto_regular, sizeof(dump_roboto_regular), 18.0f);
    uiSetup.AddFontFromMemory(dump_font, sizeof(dump_font), 18.0f);
    IMUI::Setup(uiSetup);
    // tweak y offset of the retro-future-font
    IMUI::Font(1)->DisplayOffset.y += 1;
    this->setupImguiStyle();

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

        // emulator windows
        for (int i = 0; i < emu_t::NumEmus; i++) {
            const auto& attrs = winAttrs[i];
            ImGui::SetNextWindowPos(ImVec2(attrs.x, attrs.y), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(attrs.w, attrs.h), ImGuiSetCond_FirstUseEver);
            ImGui::PushFont(IMUI::Font(1));
            ImGui::Begin(attrs.title, nullptr);
            ImVec2 imgSize = ImGui::GetContentRegionAvail();
            imgSize.y -= ImGui::GetTextLineHeight();
            ImGui::Image(this->emu.imgId[i], imgSize);
            ImGui::PushFont(nullptr);
            if (ImGui::Button("On/Off")) {
                this->emu.kc[i].TogglePower();
                this->emu.kc[i].TogglePower();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                this->emu.kc[i].Reset();
            }
            ImGui::PopFont();
            if (ImGui::IsWindowFocused()) {
                for (int j = 0; j < emu_t::NumEmus; j++) {
                    this->emu.kc[j].keyboard.hasInputFocus = (i == j);
                }
            }
            ImGui::End();
            ImGui::PopFont();
        }
    }
    ImGui::Render();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnCleanup() {
    for (auto& kc : this->emu.kc) {
        kc.Discard();
    }
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
    for (int i = 0; i < NumEmus; i++) {
        auto& kc = this->kc[i];
        YAKC::system sys;
        YAKC::os_rom os;
        Id* texId;
        switch (i) {
            case KC85_3:
                sys=YAKC::system::kc85_3; os=YAKC::os_rom::caos_3_1;
                texId = &kc.draw.texture;
                break;
            case KC85_4:
                sys=YAKC::system::kc85_4; os=YAKC::os_rom::caos_4_2;
                texId = &kc.draw.texture;
                break;
            default:
                sys=YAKC::system::kc87; os=YAKC::os_rom::kc87_os_2;
                texId = &kc.draw.texture;
                break;
        }
        kc.Setup(gfxSetup, sys, os);
        kc.switchOnDelayFrames = 1;
        this->imgId[i] = IMUI::AllocImage();
        IMUI::BindImage(this->imgId[i], *texId);
    }
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::emu_t::render(Duration frameTime) {
    for (int i = 0; i < NumEmus; i++) {
        this->kc[i].Update(frameTime);
        this->kc[i].Render(glm::mat4(), true);
    }
}

//------------------------------------------------------------------------------
void
ImguiAdvancedApp::setupImguiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Alpha = 1.0f;
    style.WindowFillAlphaDefault = 0.5f;
    style.WindowTitleAlign = ImGuiAlign_Center;
    style.TouchExtraPadding = ImVec2(5.0f, 5.0f);
    style.AntiAliasedLines = true;
    style.AntiAliasedShapes = true;
    style.Colors[ImGuiCol_ResizeGrip]    = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
}

