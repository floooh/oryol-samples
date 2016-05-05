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
    void drawGround();
    void drawRigidBodies();

    void setupPhysics();
    void discardPhysics();
    Duration updatePhysics(Duration frameTime);

    Id ground;
    DrawState drawState;

    glm::mat4 proj;
    glm::mat4 view;
    Shader::Params vsParams;

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

    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f), 1.0f, 0));
    this->drawGround();
    this->drawRigidBodies();

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

    // setup graphics shapes
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout.Add(VertexAttr::Position, VertexFormat::Float3);
    shapeBuilder.Layout.Add(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder.Layout.Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder
        .Plane(100, 100, 1)
        .Sphere(1, 13, 7)
        .Box(1.5f, 1.5f, 1.5f, 1);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    // create pipeline state
    Id shd = Gfx::CreateResource(Shader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->drawState.Pipeline = Gfx::CreateResource(ps);

    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.1f, 200.0f);
    this->view = glm::lookAt(glm::vec3(0.0f, 25.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::discardGraphics() {
    Gfx::Discard();
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawGround() {
    Gfx::ApplyDrawState(this->drawState);
    glm::mat4 groundTransform = Physics::Transform(this->ground);
    this->vsParams.ModelViewProjection = this->proj * this->view * groundTransform;
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(0);
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawRigidBodies() {
    // FIXME: use instancing
    Gfx::ApplyDrawState(this->drawState);
    for (int i = 0; i < this->numBodies; i++) {
        glm::mat4 ballTransform = Physics::Transform(this->bodies[i]);
        this->vsParams.ModelViewProjection = this->proj * this->view * ballTransform;
        Gfx::ApplyUniformBlock(this->vsParams);
        if (Physics::ShapeType(this->bodies[i]) == RigidBodySetup::SphereShape) {
            // a sphere
            Gfx::Draw(1);
        }
        else {
            // a box
            Gfx::Draw(2);
        }
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::setupPhysics() {
    Physics::Setup();
    this->ground = Physics::Create(RigidBodySetup::Plane(glm::vec4(0, 1, 0, 0), 0.25f));
    Physics::Add(this->ground);
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::discardPhysics() {
    Physics::Destroy(this->ground);
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
