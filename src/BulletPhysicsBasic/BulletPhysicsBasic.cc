//------------------------------------------------------------------------------
//  BulletPhysicsBasic.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/Time/Clock.h"
#include "PhysicsCommon/Physics.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"

using namespace Oryol;

class BulletPhysicsBasicApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();

    void setupGraphics();
    void discardGraphics();
    void drawShadowPass();
    void drawColorPass();

    void setupPhysics();
    void discardPhysics();
    Duration updatePhysics(Duration frameTime);

    Id groundRigidBody;

    static const int ShadowMapSize = 2048;
    Id shadowMap;
    DrawState colorDrawState;
    DrawState shadowDrawState;
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 lightView;
    glm::mat4 lightProj;
    ColorShader::ColorVSParams colorVSParams;
    ShadowShader::ShadowVSParams shadowVSParams;

    int frameIndex = 0;
    static const int MaxNumBodies = 128;
    int numBodies = 0;
    StaticArray<Id, MaxNumBodies> bodies;

    TimePoint lapTimePoint;
};
OryolMain(BulletPhysicsBasicApp);

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnInit() {
    this->setupGraphics();
    this->setupPhysics();
    Dbg::Setup();
    this->lapTimePoint = Clock::Now();
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    this->frameIndex++;
    Duration frameTime = Clock::LapTime(this->lapTimePoint);
    Duration physicsTime = this->updatePhysics(frameTime);
    this->drawShadowPass();
    this->drawColorPass();
    Dbg::PrintF("\n\r"
                "  Frame time:   %.4f ms\n\r"
                "  Physics time: %.4f ms\n\r"
                "  Num Bodies:   %d\n\r",
                frameTime.AsMilliSeconds(),
                physicsTime.AsMilliSeconds(),
                this->numBodies);
    Dbg::DrawTextBuffer();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnCleanup() {
    this->discardPhysics();
    Dbg::Discard();
    this->discardGraphics();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::setupGraphics() {

    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsBasic");
    Gfx::Setup(gfxSetup);

    // setup rigid body shapes
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout.Add(VertexAttr::Position, VertexFormat::Float3);
    shapeBuilder.Layout.Add(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder.Layout.Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder
        .Plane(100, 100, 1)
        .Sphere(1, 13, 7)
        .Box(1.5f, 1.5f, 1.5f, 1);
    this->colorDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    // create color pass pipeline state
    Id shd = Gfx::CreateResource(ColorShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->colorDrawState.Pipeline = Gfx::CreateResource(ps);

    // create shadow map
    // FIXME: use RGBA8 and encode depth in pixel shader
    auto smSetup = TextureSetup::RenderTarget(ShadowMapSize, ShadowMapSize);
    smSetup.ColorFormat = PixelFormat::RGBA32F;
    smSetup.DepthFormat = PixelFormat::DEPTHSTENCIL;
    this->shadowMap = Gfx::CreateResource(smSetup);
    this->colorDrawState.FSTexture[0] = this->shadowMap;

    // create shadow pass pipeline state
    shd = Gfx::CreateResource(ShadowShader::Setup());
    ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.CullFace = Face::Front;
    ps.RasterizerState.SampleCount = 1;
    ps.BlendState.ColorFormat = smSetup.ColorFormat;
    ps.BlendState.DepthFormat = smSetup.DepthFormat;
    this->shadowDrawState.Pipeline = Gfx::CreateResource(ps);
    this->shadowDrawState.Mesh[0]  = this->colorDrawState.Mesh[0];

    // setup view and projection matrices
    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.1f, 200.0f);
    this->view = glm::lookAt(glm::vec3(0.0f, 25.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    this->lightView = glm::lookAt(glm::vec3(50.0f, 50.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    this->lightProj = glm::ortho(-75.0f, +75.0f, -75.0f, +75.0f, 1.0f, 200.0f);
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::discardGraphics() {
    Gfx::Discard();
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawShadowPass() {

    auto clearState = ClearState::ClearAll(glm::vec4(1024.0f), 1.0f, 0);
    Gfx::ApplyRenderTarget(this->shadowMap, clearState);
    Gfx::ApplyDrawState(this->shadowDrawState);
    glm::mat4 projView = this->lightProj * this->lightView;

    for (int i = 0; i < this->numBodies; i++) {
        glm::mat4 model = Physics::Transform(this->bodies[i]);
        this->shadowVSParams.MVP = projView * model;
        Gfx::ApplyUniformBlock(this->shadowVSParams);
        int primGroup = (Physics::ShapeType(this->bodies[i]) == RigidBodySetup::SphereShape) ? 1 : 2;
        Gfx::Draw(primGroup);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawColorPass() {

    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f), 1.0f, 0));

    Gfx::ApplyDrawState(this->colorDrawState);
    glm::mat4 projView = this->proj * this->view;
    glm::mat4 lightProjView = this->lightProj * this->lightView;

    // draw ground
    glm::mat4 model = Physics::Transform(this->groundRigidBody);
    this->colorVSParams.MVP = projView * model;
    this->colorVSParams.LightMVP = lightProjView * model;
    Gfx::ApplyUniformBlock(this->colorVSParams);
    Gfx::Draw(0);

    // draw rigid bodies
    // FIXME: use instancing
    for (int i = 0; i < this->numBodies; i++) {
        model = Physics::Transform(this->bodies[i]);
        this->colorVSParams.MVP = projView * model;
        this->colorVSParams.LightMVP = lightProjView * model;
        Gfx::ApplyUniformBlock(this->colorVSParams);
        int primGroup = (Physics::ShapeType(this->bodies[i]) == RigidBodySetup::SphereShape) ? 1 : 2;
        Gfx::Draw(primGroup);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::setupPhysics() {
    Physics::Setup();
    this->groundRigidBody = Physics::Create(RigidBodySetup::Plane(glm::vec4(0, 1, 0, 0), 0.25f));
    Physics::Add(this->groundRigidBody);
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::discardPhysics() {
    Physics::Destroy(this->groundRigidBody);
    for (int i = 0; i < this->numBodies; i++) {
        Physics::Destroy(this->bodies[i]);
    }
    Physics::Discard();
}

//------------------------------------------------------------------------------
Duration
BulletPhysicsBasicApp::updatePhysics(Duration frameTime) {
    TimePoint physStartTime = Clock::Now();

    // emit new rigid bodies
    if ((this->frameIndex % 10) == 0) {
        if (this->numBodies < MaxNumBodies) {
            Id newObj;
            if (this->numBodies & 1) {
                newObj = Physics::Create(RigidBodySetup::Sphere(glm::translate(glm::mat4(), glm::vec3(0, 3, 0)), 1.0f, 1.0f, 0.5f));
            }
            else {
                newObj = Physics::Create(RigidBodySetup::Box(glm::translate(glm::mat4(), glm::vec3(0, 3, 0)), glm::vec3(1.5f), 1.0f, 0.5f));
            }
            Physics::Add(newObj);
            this->bodies[this->numBodies++] = newObj;
            btRigidBody* body = Physics::Body(newObj);
            glm::vec3 ang = glm::ballRand(10.0f);
            glm::vec3 lin = (glm::ballRand(1.0f) + glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec3(5.0f, 15.0f, 5.0f);
            body->setAngularVelocity(btVector3(ang.x, ang.y, ang.z));
            body->setLinearVelocity(btVector3(lin.x, lin.y, lin.z));
            body->setDamping(0.1f, 0.1f);
        }
    }

    // step the physics world
    Physics::Update(frameTime.AsSeconds());
    return Clock::Since(physStartTime);
}
