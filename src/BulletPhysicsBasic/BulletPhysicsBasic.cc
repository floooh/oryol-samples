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

    Id ground;
    Id ballTexture;
    Id groundTexture;
    DrawState drawState;

    glm::mat4 proj;
    glm::mat4 view;
    Shader::Params vsParams;

    int frameIndex = 0;
    static const int MaxNumBodies = 1024;
    int numBodies = 0;
    StaticArray<Id, MaxNumBodies> bodies;

    TimePoint lapTimePoint;
};
OryolMain(BulletPhysicsBasicApp);

// a simple ball texture
uint32_t BallImageData[] = {
    0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFF00FFFF, // red, green, blue, yello
};

// a checkboard ground texture
static const int GroundTexSize = 16;
uint32_t GroundImageData[GroundTexSize][GroundTexSize];

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsBasic");
    Gfx::Setup(gfxSetup);
    Dbg::Setup();

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

    // create textures
    auto texSetup = TextureSetup::FromPixelData(4, 1, 1, TextureType::Texture2D, PixelFormat::RGBA8);
    texSetup.ImageData.Sizes[0][0] = sizeof(BallImageData);
    this->ballTexture = Gfx::CreateResource(texSetup, BallImageData, sizeof(BallImageData));

    for (int i = 0, x = 0; x < GroundTexSize; x++, i++) {
        for (int y = 0; y < GroundTexSize; y++, i++) {
            GroundImageData[x][y] = i&1 ? 0xFF8080B0 : 0xFFB08080;
        }
    }
    texSetup = TextureSetup::FromPixelData(GroundTexSize, GroundTexSize, 1, TextureType::Texture2D, PixelFormat::RGBA8);
    texSetup.ImageData.Sizes[0][0] = sizeof(GroundImageData);
    this->groundTexture = Gfx::CreateResource(texSetup, GroundImageData, sizeof(GroundImageData));

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

    // setup the physics world
    Physics::Setup();
    this->ground = Physics::Create(RigidBodySetup::Plane(glm::vec4(0, 1, 0, 0), 0.25f));
    Physics::Add(this->ground);

    this->lapTimePoint = Clock::Now();
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    this->frameIndex++;
    Duration frameTime = Clock::LapTime(this->lapTimePoint);

    // emit new balls
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

    TimePoint physStartTime = Clock::Now();
    Physics::Update(1.0f/60.0f);
    Duration physTime = Clock::Since(physStartTime);
    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f), 1.0f, 0));

    // draw ground
    this->drawState.FSTexture[0] = this->groundTexture;
    Gfx::ApplyDrawState(this->drawState);
    glm::mat4 groundTransform = Physics::Transform(this->ground);
    this->vsParams.ModelViewProjection = this->proj * this->view * groundTransform;
    this->vsParams.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(0);

    // draw balls (fixme: use instancing)
    this->drawState.FSTexture[0] = this->ballTexture;
    Gfx::ApplyDrawState(this->drawState);
    for (int i = 0; i < this->numBodies; i++) {
        glm::mat4 ballTransform = Physics::Transform(this->bodies[i]);
        this->vsParams.ModelViewProjection = this->proj * this->view * ballTransform;
        this->vsParams.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
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

    Dbg::PrintF("\n\r"
                "  Frame time:   %.4f ms\n\r"
                "  Physics time: %.4f ms\n\r"
                "  Num Bodies:   %d\n\r",
                frameTime.AsMilliSeconds(),
                physTime.AsMilliSeconds(),
                this->numBodies);
    Dbg::DrawTextBuffer();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnCleanup() {

    Physics::Destroy(this->ground);
    for (int i = 0; i < this->numBodies; i++) {
        Physics::Destroy(this->bodies[i]);
    }
    Physics::Discard();
    Dbg::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
