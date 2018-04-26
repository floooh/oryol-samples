//-----------------------------------------------------------------------------
//  MeshViewer.cc
//-----------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/String/StringBuilder.h"
#include "Gfx/Gfx.h"
#include "IO/IO.h"
#include "IMUI/IMUI.h"
#include "Input/Input.h"
#include "HttpFS/HTTPFileSystem.h"
#include "Common/OmshParser.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/polar_coordinates.hpp"
#include "shaders.h"

using namespace Oryol;

class MeshViewerApp : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

private:
    void handleInput();
    void updateCamera();
    void updateLight();
    void drawUI();
    void createMaterials();
    void loadMesh(const char* path);
    void applyVariables(int materialIndex);

    int frameCount = 0;
    ResourceLabel curMeshLabel;
    Id vertexBuffer;
    Id indexBuffer;
    Array<PrimitiveGroup> primGroups;
    VertexLayout layout;
    IndexType::Code indexType = IndexType::None;
    glm::vec3 eyePos;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 model;
    glm::mat4 modelViewProj;

    int curMeshIndex = 0;
    static const int numMeshes = 3;
    static const char* meshNames[numMeshes];
    static const char* meshPaths[numMeshes];
    static const int numShaders = 3;
    static const char* shaderNames[numShaders];
    enum {
        Normals = 0,
        Lambert,
        Phong
    };
    Id shaders[numShaders];
    ResourceLabel curMaterialLabel;
    int numMaterials = 0;
    static constexpr int MaxNumMaterials = 16;
    struct Material {
        int shaderIndex = Phong;
        Id pipeline;
        glm::vec4 diffuse = glm::vec4(0.0f, 0.24f, 0.64f, 1.0f);
        glm::vec4 specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        float specPower = 32.0f;
    } materials[MaxNumMaterials];
    bool gammaCorrect = true;

    struct CameraSetting {
        float dist = 8.0f;
        glm::vec2 orbital = glm::vec2(glm::radians(25.0f), 0.0f);
        float height = 1.0f;
        glm::vec2 startOrbital;
        float startDistance = 0.0f;
    } camera;
    bool camAutoOrbit = true;
    struct CameraSetting cameraSettings[numMeshes];

    glm::vec2 lightOrbital = glm::vec2(glm::radians(25.0f), 0.0f);
    glm::vec3 lightDir;
    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float lightIntensity = 1.0f;
    bool lightAutoOrbit = false;
    bool dragging = false;

    StringBuilder strBuilder;

    const float minCamDist = 0.5f;
    const float maxCamDist = 20.0f;
    const float minLatitude = -85.0f;
    const float maxLatitude = 85.0f;
    const float minCamHeight = -2.0f;
    const float maxCamHeight = 5.0f;
};
OryolMain(MeshViewerApp);

const char* MeshViewerApp::meshNames[numMeshes] = {
    "Tiger",
    "Blitz",
    "Teapot"
};
const char* MeshViewerApp::meshPaths[numMeshes] = {
    "msh:tiger.omsh.txt",
    "msh:opelblitz.omsh.txt",
    "msh:teapot.omsh.txt"
};

const char* MeshViewerApp::shaderNames[numShaders] = {
    "Normals",
    "Lambert",
    "Phong"
};

//-----------------------------------------------------------------------------
AppState::Code
MeshViewerApp::OnInit() {

    IO::Setup(IODesc()
        .FileSystem("http", HTTPFileSystem::Creator())
        .Assign("msh:", ORYOL_SAMPLE_URL));
    Gfx::Setup(GfxDesc()
        .Width(800)
        .Height(512)
        .SampleCount(4)
        .HighDPI(true)
        .Title("Oryol Mesh Viewer"));
    Input::Setup();
    Input::SetPointerLockHandler([this] (const InputEvent& event) -> PointerLockMode::Code {
        if (event.Button == MouseButton::Left) {
            if (event.Type == InputEvent::MouseButtonDown) {
                if (!ImGui::IsMouseHoveringAnyWindow()) {
                    this->dragging = true;
                    return PointerLockMode::Enable;
                }
            }
            else if (event.Type == InputEvent::MouseButtonUp) {
                if (this->dragging) {
                    this->dragging = false;
                    return PointerLockMode::Disable;
                }
            }
        }
        return PointerLockMode::DontCare;
    });

    // setup IMUI ui system
    IMUI::Setup();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.TouchExtraPadding.x = 5.0f;
    style.TouchExtraPadding.y = 5.0f;
    ImVec4 defaultBlue(0.0f, 0.5f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_TitleBg] = defaultBlue;
    style.Colors[ImGuiCol_TitleBgCollapsed] = defaultBlue;
    style.Colors[ImGuiCol_SliderGrab] = defaultBlue;
    style.Colors[ImGuiCol_SliderGrabActive] = defaultBlue;
    style.Colors[ImGuiCol_Button] = defaultBlue;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.5f, 1.0f, 0.3f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0f, 0.5f, 1.0f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.5f, 1.0f, 0.3f);
    style.Colors[ImGuiCol_Header] = defaultBlue;
    style.Colors[ImGuiCol_HeaderHovered] = defaultBlue;
    style.Colors[ImGuiCol_HeaderActive] = defaultBlue;

    this->shaders[Normals] = Gfx::CreateShader(NormalsShader::Desc());
    this->shaders[Lambert] = Gfx::CreateShader(LambertShader::Desc());
    this->shaders[Phong]   = Gfx::CreateShader(PhongShader::Desc());
    this->loadMesh(this->meshPaths[this->curMeshIndex]);

    // setup projection and view matrices
    const float fbWidth = (const float) Gfx::DisplayAttrs().Width;
    const float fbHeight = (const float) Gfx::DisplayAttrs().Height;
    this->proj = glm::perspectiveFov(glm::radians(60.0f), fbWidth, fbHeight, 0.01f, 100.0f);

    // non-standard camera settings when switching objects to teapot:
    this->cameraSettings[2].dist = 0.8f;
    this->cameraSettings[2].height = 0.0f;

    return App::OnInit();
}

//-----------------------------------------------------------------------------
AppState::Code
MeshViewerApp::OnRunning() {

    this->frameCount++;
    this->handleInput();
    this->updateCamera();
    this->updateLight();

    Gfx::BeginPass(PassAction().Clear(0.5f, 0.5f, 0.5f, 1.0f));
    this->drawUI();
    DrawState drawState;
    drawState.VertexBuffers[0] = this->vertexBuffer;
    drawState.IndexBuffer = this->indexBuffer;
    for (int i = 0; i < this->numMaterials; i++) {
        drawState.Pipeline = this->materials[i].pipeline;
        Gfx::ApplyDrawState(drawState);
        this->applyVariables(i);
        Gfx::Draw(this->primGroups[i]);
    }
    ImGui::Render();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//-----------------------------------------------------------------------------
AppState::Code
MeshViewerApp::OnCleanup() {
    IMUI::Discard();
    Input::Discard();
    Gfx::Discard();
    IO::Discard();
    return App::OnCleanup();
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::handleInput() {

    // rotate camera with mouse if not UI-dragging
    if (Input::TouchpadAttached()) {
        if (!ImGui::IsMouseHoveringAnyWindow()) {
            if (Input::TouchPanningStarted()) {
                this->camera.startOrbital = this->camera.orbital;
            }
            if (Input::TouchPanning()) {
                glm::vec2 diff = (Input::TouchPosition(0) - Input::TouchStartPosition(0)) * 0.01f;
                this->camera.orbital.y = this->camera.startOrbital.y - diff.x;
                this->camera.orbital.x = glm::clamp(this->camera.startOrbital.x + diff.y, glm::radians(minLatitude), glm::radians(maxLatitude));
            }
            if (Input::TouchPinchingStarted()) {
                this->camera.startDistance = this->camera.dist;
            }
            if (Input::TouchPinching()) {
                float startDist = glm::length(glm::vec2(Input::TouchStartPosition(1) - Input::TouchStartPosition(0)));
                float curDist   = glm::length(glm::vec2(Input::TouchPosition(1) - Input::TouchPosition(0)));
                this->camera.dist = glm::clamp(this->camera.startDistance - (curDist - startDist) * 0.01f, minCamDist, maxCamDist);
            }
        }
    }
    if (Input::MouseAttached()) {
        if (this->dragging) {
            if (Input::MouseButtonPressed(MouseButton::Left)) {
                this->camera.orbital.y -= Input::MouseMovement().x * 0.01f;
                this->camera.orbital.x = glm::clamp(
                    this->camera.orbital.x + Input::MouseMovement().y * 0.01f,
                    glm::radians(minLatitude),
                    glm::radians(maxLatitude));
            }
        }
        this->camera.dist = glm::clamp(this->camera.dist + Input::MouseScroll().y * 0.1f, minCamDist, maxCamDist);
    }
}

//------------------------------------------------------------------------------
void
MeshViewerApp::updateCamera() {
    if (this->camAutoOrbit) {
        this->camera.orbital.y += 0.01f;
        if (this->camera.orbital.y > glm::radians(360.0f)) {
            this->camera.orbital.y = 0.0f;
        }
    }
    this->eyePos = glm::euclidean(this->camera.orbital) * this->camera.dist;
    glm::vec3 poi  = glm::vec3(0.0f, this->camera.height, 0.0f);
    this->view = glm::lookAt(this->eyePos + poi, poi, glm::vec3(0.0f, 1.0f, 0.0f));
    this->modelViewProj = this->proj * this->view;
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::updateLight() {
    if (this->lightAutoOrbit) {
        this->lightOrbital.y += 0.01f;
        if (this->lightOrbital.y > glm::radians(360.0f)) {
            this->lightOrbital.y = 0.0f;
        }
    }
    this->lightDir = glm::euclidean(this->lightOrbital);
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::drawUI() {
    const char* state;
    switch (Gfx::QueryResourceState(this->vertexBuffer)) {
        case ResourceState::Valid: state = "Loaded"; break;
        case ResourceState::Failed: state = "Load Failed"; break;
        case ResourceState::Alloc: state = "Loading..."; break;
        default: state = "Invalid"; break;
    }
    IMUI::NewFrame();
    ImGui::Begin("Mesh Viewer", nullptr, ImVec2(240, 300), 0.25f, 0);
    ImGui::PushItemWidth(130.0f);
    if (ImGui::Combo("##mesh", (int*) &this->curMeshIndex, this->meshNames, numMeshes)) {
        this->camera = this->cameraSettings[this->curMeshIndex];
        this->loadMesh(this->meshPaths[this->curMeshIndex]);
    }
    ImGui::Text("state: %s\n", state);
    if (this->primGroups.Size() > 0) {
        ImGui::Text("primitive groups:\n");
        for (int i = 0; i < this->primGroups.Size(); i++) {
            ImGui::Text("%d: %d triangles\n", i, this->primGroups[i].NumElements / 3);
        }
    }
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat("Dist##cam", &this->camera.dist, minCamDist, maxCamDist);
        ImGui::SliderFloat("Height##cam", &this->camera.height, minCamHeight, maxCamHeight);
        ImGui::SliderAngle("Long##cam", &this->camera.orbital.y, 0.0f, 360.0f);
        ImGui::SliderAngle("Lat##cam", &this->camera.orbital.x, minLatitude, maxLatitude);
        ImGui::Checkbox("Auto Orbit##cam", &this->camAutoOrbit);
        if (ImGui::Button("Reset##cam")) {
            this->camera.dist = 8.0f;
            this->camera.height = 1.0f;
            this->camera.orbital = glm::vec2(0.0f, 0.0f);
            this->camAutoOrbit = false;
        }
    }
    if (ImGui::CollapsingHeader("Light")) {
        ImGui::SliderAngle("Long##light", &this->lightOrbital.y, 0.0f, 360.0f);
        ImGui::SliderAngle("Lat##light", &this->lightOrbital.x, minLatitude, maxLatitude);
        ImGui::ColorEdit3("Color##light", &this->lightColor.x);
        ImGui::SliderFloat("Intensity##light", &this->lightIntensity, 0.0f, 5.0f);
        ImGui::Checkbox("Auto Orbit##light", &this->lightAutoOrbit);
        ImGui::Checkbox("Gamma Correct##light", &this->gammaCorrect);
        if (ImGui::Button("Reset##light")) {
            this->lightOrbital = glm::vec2(glm::radians(25.0f), 0.0f);
            this->lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->lightIntensity = 1.0f;
            this->lightAutoOrbit = false;
        }
    }
    for (int i = 0; i < this->numMaterials; i++) {
        this->strBuilder.Format(32, "Material %d", i);
        if (ImGui::CollapsingHeader(this->strBuilder.AsCStr())) {
            this->strBuilder.Format(32, "shader##mat%d", i);
            if (ImGui::Combo(strBuilder.AsCStr(), &this->materials[i].shaderIndex, this->shaderNames, numShaders)) {
                this->createMaterials();
            }
            if ((Lambert == this->materials[i].shaderIndex) || (Phong == this->materials[i].shaderIndex)) {
                this->strBuilder.Format(32, "diffuse##%d", i);
                ImGui::ColorEdit3(this->strBuilder.AsCStr(), &this->materials[i].diffuse.x);
            }
            if (Phong == this->materials[i].shaderIndex) {
                this->strBuilder.Format(32, "specular##%d", i);
                ImGui::ColorEdit3(this->strBuilder.AsCStr(), &this->materials[i].specular.x);
                this->strBuilder.Format(32, "power##%d", i);
                ImGui::SliderFloat(this->strBuilder.AsCStr(), &this->materials[i].specPower, 1.0f, 512.0f);
            }
        }
    }
    ImGui::PopItemWidth();
    ImGui::End();
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::createMaterials() {
    if (this->curMaterialLabel.IsValid()) {
        Gfx::DestroyResources(this->curMaterialLabel);
    }

    this->curMaterialLabel = Gfx::PushResourceLabel();
    for (int i = 0; i < this->numMaterials; i++) {
        this->materials[i].pipeline = Gfx::CreatePipeline(PipelineDesc()
            .Shader(this->shaders[this->materials[i].shaderIndex])
            .Layout(0, this->layout)
            .IndexType(this->indexType)
            .DepthWriteEnabled(true)
            .DepthCmpFunc(CompareFunc::LessEqual)
            .CullFaceEnabled(true)
            .SampleCount(4));
    }
    Gfx::PopResourceLabel();
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::loadMesh(const char* path) {

    // unload current mesh
    if (this->curMeshLabel.IsValid()) {
        Gfx::DestroyResources(this->curMeshLabel);
        this->vertexBuffer.Invalidate();
        this->indexBuffer.Invalidate();
        this->primGroups.Clear();
    }

    // load new mesh, use 'onloaded' callback to capture the mesh setup
    // object of the loaded mesh
    this->numMaterials = 0;
    this->curMeshLabel = Gfx::PushResourceLabel();
    this->vertexBuffer = Gfx::AllocBuffer(Locator::NonShared());
    this->indexBuffer = Gfx::AllocBuffer(Locator::NonShared());
    Gfx::PopResourceLabel();
    IO::Load(path, [this](IO::LoadResult res) {
        OmshParser::Result omsh = OmshParser::Parse(res.Data.Data(), res.Data.Size());
        if (omsh.Valid) {
            Gfx::InitBuffer(this->vertexBuffer, omsh.VertexBufferDesc);
            Gfx::InitBuffer(this->indexBuffer, omsh.IndexBufferDesc);
            this->layout = omsh.Layout;
            this->indexType = omsh.IndexType;
            this->primGroups = omsh.PrimitiveGroups;
            this->numMaterials = omsh.PrimitiveGroups.Size();
            this->createMaterials();
        }
    });
}

//-----------------------------------------------------------------------------
void
MeshViewerApp::applyVariables(int matIndex) {
    switch (this->materials[matIndex].shaderIndex) {
        case Normals:
            // Normals shader
            {
                NormalsShader::vsParams vsParams;
                vsParams.mvp = this->modelViewProj;
                Gfx::ApplyUniformBlock(vsParams);
            }
            break;
        case Lambert:
            // Lambert shader
            {
                LambertShader::vsParams vsParams;
                vsParams.mvp = this->modelViewProj;
                vsParams.model = this->model;
                Gfx::ApplyUniformBlock(vsParams);

                LambertShader::fsParams fsParams;
                fsParams.lightColor = this->lightColor * this->lightIntensity;
                fsParams.lightDir = this->lightDir;
                fsParams.matDiffuse = this->materials[matIndex].diffuse;
                fsParams.gammaCorrect = this->gammaCorrect ? 1.0f : 0.0f;
                Gfx::ApplyUniformBlock(fsParams);
            }
            break;
        case Phong:
            // Phong shader
            {
                PhongShader::vsParams vsParams;
                vsParams.mvp = this->modelViewProj;
                vsParams.model = this->model;
                Gfx::ApplyUniformBlock(vsParams);

                PhongShader::fsParams fsParams;
                fsParams.eyePos = this->eyePos;
                fsParams.lightColor = this->lightColor * this->lightIntensity;
                fsParams.lightDir = this->lightDir;
                fsParams.matDiffuse = this->materials[matIndex].diffuse;
                fsParams.matSpecular = this->materials[matIndex].specular;
                fsParams.matSpecularPower = this->materials[matIndex].specPower;
                fsParams.gammaCorrect = this->gammaCorrect ? 1.0f : 0.0f;
                Gfx::ApplyUniformBlock(fsParams);
            }
            break;
            
        default:
            o_error("Unknown shader index, FIXME!");
            break;
    }
}
