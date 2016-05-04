//------------------------------------------------------------------------------
//  BulletPhysicsBasic.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "PhysicsCommon/Physics.h"
#include "Gfx/Gfx.h"
#include "IMUI/IMUI.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace Oryol;

class BulletPhysicsBasicApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();

    Id ground;
    Id ball;
    DrawState drawState;
    glm::mat4 proj;
    glm::mat4 view;
    Shader::Params vsParams;
};
OryolMain(BulletPhysicsBasicApp);

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsBasic");
    Gfx::Setup(gfxSetup);
    IMUI::Setup();

    // setup graphics shapes
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout.Add(VertexAttr::Position, VertexFormat::Float3);
    shapeBuilder
        .Plane(100.0f, 100.0f, 10)
        .Sphere(1.0f, 36, 10);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());
    Id shd = Gfx::CreateResource(Shader::Setup());

    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->drawState.Pipeline = Gfx::CreateResource(ps);

    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.01f, 100.0f);
    this->view = glm::lookAt(glm::vec3(0.0f, 25.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // setup the physics world
    Physics::Setup();
    this->ground = Physics::Create(RigidBodySetup::Plane(glm::vec4(0, 1, 0, 0), 0.5f));
    this->ball = Physics::Create(RigidBodySetup::Sphere(glm::translate(glm::mat4(), glm::vec3(0, 50, 0)), 1.0f, 1.0f, 1.0f));
    Physics::Add(this->ground);
    Physics::Add(this->ball);

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    Physics::Update(1.0f/60.0f); 

    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), 1.0f, 0));
    Gfx::ApplyDrawState(this->drawState);

    // draw ground
    glm::mat4 groundTransform = Physics::Transform(this->ground);
    this->vsParams.ModelViewProjection = this->proj * this->view * groundTransform;
    this->vsParams.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(0);

    // draw ball
    glm::mat4 ballTransform = Physics::Transform(this->ball);
    this->vsParams.ModelViewProjection = this->proj * this->view * ballTransform;
    this->vsParams.Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(1);

    IMUI::NewFrame();
    if (ImGui::Begin("Physics")) {
        ImGui::Text("Ball Height: %f", ballTransform[3].y);
    }
    ImGui::End();

    ImGui::Render();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnCleanup() {

    Physics::Destroy(this->ground);
    Physics::Destroy(this->ball);
    Physics::Discard();
    IMUI::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
