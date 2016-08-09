//------------------------------------------------------------------------------
//  ImGuiAdvanced.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "IMUI/IMUI.h"
#include "font.h"

using namespace Oryol;

class ImguiAdvancedApp : public App {
public:
    /// on init frame method
    virtual AppState::Code OnInit();
    /// on running frame method
    virtual AppState::Code OnRunning();
    /// on cleanup frame method
    virtual AppState::Code OnCleanup();
};
OryolMain(ImguiAdvancedApp);

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnInit() {
    auto gfxSetup = GfxSetup::Window(1024, 700, "Oryol ImGui Advanced");
    gfxSetup.HighDPI = false;
    Gfx::Setup(gfxSetup);
    Input::Setup();

    // add a custom font which has been embedded as C array
    IMUISetup uiSetup;
    uiSetup.AddFontFromMemory(dump_roboto_regular, sizeof(dump_roboto_regular), 20.0f);
    uiSetup.AddFontFromMemory(dump_font, sizeof(dump_font), 24.0f);
    IMUI::Setup(uiSetup);

    return AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnRunning() {
    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.25f, 0.75f, 0.45f, 1.0f)));
    IMUI::NewFrame();
        static bool window1 = true;
        ImGui::SetNextWindowSize(ImVec2(400,100), ImGuiSetCond_FirstUseEver);
        ImGui::PushFont(IMUI::Font(0));
        ImGui::Begin("Another Window", &window1);
        ImGui::PushFont(IMUI::Font(1));
        ImGui::Text("KC85/3 KC85/4 Z9001 Z1013");
        ImGui::PopFont();
        ImGui::PushFont(nullptr);
        ImGui::Text("Hello World");
        ImGui::PopFont();
        ImGui::End();
        ImGui::PopFont();
    ImGui::Render();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
ImguiAdvancedApp::OnCleanup() {
    IMUI::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}


