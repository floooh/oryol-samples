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
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/random.hpp"

using namespace Oryol;

static const float SphereRadius = 1.0f;
static const float BoxSize = 1.5f;

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
    glm::mat4 lightProjView;
    ColorShader::ColorVSParams colorVSParams;
    ColorShader::ColorFSParams colorFSParams;
    ShadowShader::ShadowVSParams shadowVSParams;

    struct body {
        Id id;
        glm::vec3 diffColor;
    };
    int frameIndex = 0;
    static const int MaxNumBodies = 512;
    int numBodies = 0;
    StaticArray<body, MaxNumBodies> bodies;

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
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::Normal, VertexFormat::Byte4N)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder
        .Plane(100, 100, 1)
        .Sphere(SphereRadius, 15, 11)
        .Box(BoxSize, BoxSize, BoxSize, 1);
    this->colorDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    // create color pass pipeline state
    Id shd = Gfx::CreateResource(ColorShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->colorDrawState.Pipeline = Gfx::CreateResource(ps);

    // create shadow map, use RGBA8 format and encode/decode depth in pixel shader
    auto smSetup = TextureSetup::RenderTarget(ShadowMapSize, ShadowMapSize);
    smSetup.ColorFormat = PixelFormat::RGBA8;
    smSetup.DepthFormat = PixelFormat::DEPTH;
    this->shadowMap = Gfx::CreateResource(smSetup);
    this->colorDrawState.FSTexture[0] = this->shadowMap;
    this->colorFSParams.ShadowMapSize = glm::vec2(float(ShadowMapSize));

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
    const glm::vec3 eyePos(0.0f, 25.0f, -50.0f);
    this->view = glm::lookAt(eyePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightView = glm::lookAt(glm::vec3(25.0f, 50.0f, -25.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-75.0f, +75.0f, -75.0f, +75.0f, 1.0f, 400.0f);
    this->lightProjView = lightProj * lightView;
    this->colorFSParams.LightDir = glm::vec3(glm::row(lightView, 2));
    this->colorFSParams.EyePos = eyePos;
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::discardGraphics() {
    Gfx::Discard();
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawShadowPass() {

    Gfx::ApplyRenderTarget(this->shadowMap, ClearState::ClearAll(glm::vec4(0.0f), 1.0f, 0));
    Gfx::ApplyDrawState(this->shadowDrawState);

    for (int i = 0; i < this->numBodies; i++) {
        glm::mat4 model = Physics::Transform(this->bodies[i].id);
        this->shadowVSParams.MVP = this->lightProjView * model;
        Gfx::ApplyUniformBlock(this->shadowVSParams);
        int primGroup = (Physics::ShapeType(this->bodies[i].id) == RigidBodySetup::SphereShape) ? 1 : 2;
        Gfx::Draw(primGroup);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawColorPass() {
    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f), 1.0f, 0));

    Gfx::ApplyDrawState(this->colorDrawState);
    glm::mat4 projView = this->proj * this->view;

    // draw ground
    glm::mat4 model = Physics::Transform(this->groundRigidBody);
    this->colorVSParams.Model = model;
    this->colorVSParams.MVP = projView * model;
    this->colorVSParams.LightMVP = lightProjView * model;
    this->colorVSParams.DiffColor = glm::vec3(0.5, 0.5, 0.5);
    Gfx::ApplyUniformBlock(this->colorVSParams);
    Gfx::ApplyUniformBlock(this->colorFSParams);
    Gfx::Draw(0);

    // draw rigid bodies
    // FIXME: use instancing
    for (int i = 0; i < this->numBodies; i++) {
        model = Physics::Transform(this->bodies[i].id);
        this->colorVSParams.Model = model;
        this->colorVSParams.MVP = projView * model;
        this->colorVSParams.LightMVP = this->lightProjView * model;
        this->colorVSParams.DiffColor = this->bodies[i].diffColor;
        Gfx::ApplyUniformBlock(this->colorVSParams);
        int primGroup = (Physics::ShapeType(this->bodies[i].id) == RigidBodySetup::SphereShape) ? 1 : 2;
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
        Physics::Destroy(this->bodies[i].id);
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
                newObj = Physics::Create(RigidBodySetup::Sphere(glm::translate(glm::mat4(), glm::vec3(0, 3, 0)), SphereRadius, 1.0f, 0.5f));
            }
            else {
                newObj = Physics::Create(RigidBodySetup::Box(glm::translate(glm::mat4(), glm::vec3(0, 3, 0)), glm::vec3(BoxSize), 1.0f, 0.5f));
            }
            Physics::Add(newObj);
            this->bodies[this->numBodies].id = newObj;
            this->bodies[this->numBodies].diffColor = glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f));
            this->numBodies++;

            btRigidBody* body = Physics::Body(newObj);
            glm::vec3 ang = glm::ballRand(10.0f);
            glm::vec3 lin = (glm::ballRand(1.0f) + glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec3(2.5f, 20.0f, 2.5f);
            body->setAngularVelocity(btVector3(ang.x, ang.y, ang.z));
            body->setLinearVelocity(btVector3(lin.x, lin.y, lin.z));
            body->setDamping(0.1f, 0.1f);
        }
    }

    // step the physics world
    Physics::Update(frameTime.AsSeconds());
    return Clock::Since(physStartTime);
}
