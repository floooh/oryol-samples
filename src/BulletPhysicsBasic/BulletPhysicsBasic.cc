//------------------------------------------------------------------------------
//  BulletPhysicsBasic.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/Time/Clock.h"
#include "PhysicsCommon/Physics.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Input/Input.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/polar_coordinates.hpp"

using namespace Oryol;

static const float SphereRadius = 1.0f;
static const float BoxSize = 1.5f;
const float MinCamDist = 5.0f;
const float MaxCamDist = 100.0f;
const float MinLatitude = -85.0f;
const float MaxLatitude = 85.0f;

class BulletPhysicsBasicApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();

    void setupGraphics();
    void drawShadowPass();
    void drawColorPass();
    void setupPhysics();
    void discardPhysics();
    Duration updatePhysics();
    void setupInput();
    void handleInput();
    void updateCamera();
    void updateInstanceData();

    int frameIndex = 0;
    TimePoint lapTimePoint;

    // rendering state
    static const int ShadowMapSize = 2048;
    Id shadowMap;
    DrawState colorDrawState;
    DrawState shadowDrawState;
    DrawState colorInstancedDrawState;
    DrawState shadowInstancedDrawState;
    ColorShader::ColorVSParams colorVSParams;
    ColorShader::ColorFSParams colorFSParams;
    ShadowShader::ShadowVSParams shadowVSParams;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::mat4 lightProjView;

    // physics rigid body state
    Id groundRigidBody;
    static const int MaxNumBodies = 512;
    int numBodies = 0;
    struct body {
        Id id;
        glm::vec3 diffColor;
    };
    StaticArray<body, MaxNumBodies> bodies;

    // camera state
    struct {
        float dist = 60.0f;
        glm::vec2 orbital = glm::vec2(glm::radians(25.0f), glm::radians(180.0f));
        bool dragging = false;
        glm::vec3 eyePos;
    } camera;

    // instancing stuff
    static const int MaxNumInstances = (MaxNumBodies/2)+1;
    int numSpheres = 0;
    int numBoxes = 0;
    Id sphereInstMesh;
    Id boxInstMesh;
    struct instData {
        glm::vec4 xxxx;
        glm::vec4 yyyy;
        glm::vec4 zzzz;
        glm::vec4 color;
    };
    instData sphereInstData[MaxNumInstances];
    instData boxInstData[MaxNumInstances];
};
OryolMain(BulletPhysicsBasicApp);

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnInit() {
    this->setupGraphics();
    this->setupPhysics();
    this->setupInput();

    Dbg::Setup();
    this->lapTimePoint = Clock::Now();
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    this->frameIndex++;
    Duration frameTime = Clock::LapTime(this->lapTimePoint);
    Duration physicsTime = this->updatePhysics();
    this->handleInput();
    this->updateCamera();
    this->updateInstanceData();
    this->drawShadowPass();
    this->drawColorPass();
    Dbg::PrintF("\n\r"
                "  Mouse left click + drag: rotate camera\n\r"
                "  Mouse wheel: zoom camera\n\n\r"
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
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::setupGraphics() {

    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsBasic");
    Gfx::Setup(gfxSetup);

    // create 2 instance-data buffers, one for the spheres, one for the boxes,
    // per-instance data is a 4x3 transposed model matrix, and vec4 color
    auto instSetup = MeshSetup::Empty(MaxNumInstances, Usage::Stream);
    instSetup.Layout
        .EnableInstancing()
        .Add(VertexAttr::Instance0, VertexFormat::Float4)
        .Add(VertexAttr::Instance1, VertexFormat::Float4)
        .Add(VertexAttr::Instance2, VertexFormat::Float4)
        .Add(VertexAttr::Color0, VertexFormat::Float4);
    this->sphereInstMesh = Gfx::CreateResource(instSetup);
    this->boxInstMesh    = Gfx::CreateResource(instSetup);

    // pre-initialize the instance data with random colors
    for (int i = 0; i < MaxNumInstances; i++) {
        this->sphereInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }
    for (int i = 0; i < MaxNumInstances; i++) {
        this->boxInstData[i].color = glm::linearRand(glm::vec4(0.0f), glm::vec4(1.0f));
    }

    // setup a mesh with a sphere and box submesh, which is used by all draw states
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::Normal, VertexFormat::Byte4N);
    shapeBuilder
        .Plane(100, 100, 1)
        .Sphere(SphereRadius, 15, 11)
        .Box(BoxSize, BoxSize, BoxSize, 1);
    this->colorDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());
    this->colorInstancedDrawState.Mesh[0] = this->colorDrawState.Mesh[0];
    this->shadowDrawState.Mesh[0] = this->colorDrawState.Mesh[0];
    this->shadowInstancedDrawState.Mesh[0] = this->colorDrawState.Mesh[0];

    // create color pass pipeline states (one for non-instanced, one for instanced rendering)
    Id shd = Gfx::CreateResource(ColorShader::Setup());
    auto ps = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    ps.DepthStencilState.DepthWriteEnabled = true;
    ps.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    ps.RasterizerState.CullFaceEnabled = true;
    ps.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->colorDrawState.Pipeline = Gfx::CreateResource(ps);
    ps.Shader = Gfx::CreateResource(ColorShaderInstanced::Setup());
    ps.Layouts[1] = instSetup.Layout;
    this->colorInstancedDrawState.Pipeline = Gfx::CreateResource(ps);

    // create shadow map, use RGBA8 format and encode/decode depth in pixel shader
    auto smSetup = TextureSetup::RenderTarget(ShadowMapSize, ShadowMapSize);
    smSetup.ColorFormat = PixelFormat::RGBA8;
    smSetup.DepthFormat = PixelFormat::DEPTH;
    this->shadowMap = Gfx::CreateResource(smSetup);
    this->colorDrawState.FSTexture[0] = this->shadowMap;
    this->colorFSParams.ShadowMapSize = glm::vec2(float(ShadowMapSize));

    // create shadow pass pipeline states (one for non-instanced, one for instanced rendering)
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
    ps.Shader = Gfx::CreateResource(ShadowShaderInstanced::Setup());
    ps.Layouts[1] = instSetup.Layout;
    this->shadowInstancedDrawState.Pipeline = Gfx::CreateResource(ps);

    // setup view and projection matrices
    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->proj = glm::perspectiveFov(glm::radians(45.0f), fbWidth, fbHeight, 0.1f, 200.0f);
    this->updateCamera();

    // setup directional light (for lighting and shadow rendering)
    glm::mat4 lightView = glm::lookAt(glm::vec3(50.0f, 50.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-75.0f, +75.0f, -75.0f, +75.0f, 1.0f, 400.0f);
    this->lightProjView = lightProj * lightView;
    this->colorFSParams.LightDir = glm::vec3(glm::column(glm::inverse(lightView), 2));
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawShadowPass() {

    this->shadowVSParams.MVP = this->lightProjView;
    Gfx::ApplyRenderTarget(this->shadowMap, ClearState::ClearAll(glm::vec4(0.0f), 1.0f, 0));

    // one instanced drawcall for all sphere, and one for all boxes
    if (this->numSpheres > 0) {
        this->shadowInstancedDrawState.Mesh[1] = this->sphereInstMesh;
        Gfx::ApplyDrawState(this->shadowInstancedDrawState);
        Gfx::ApplyUniformBlock(this->shadowVSParams);
        Gfx::DrawInstanced(1, this->numSpheres);
    }
    if (this->numBoxes > 0) {
        this->shadowInstancedDrawState.Mesh[1] = this->boxInstMesh;
        Gfx::ApplyDrawState(this->shadowInstancedDrawState);
        Gfx::ApplyUniformBlock(this->shadowVSParams);
        Gfx::DrawInstanced(2, this->numBoxes);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::drawColorPass() {
    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f), 1.0f, 0));

    glm::mat4 projView = this->proj * this->view;

    // draw ground
    Gfx::ApplyDrawState(this->colorDrawState);
    glm::mat4 model = Physics::Transform(this->groundRigidBody);
    this->colorVSParams.Model = model;
    this->colorVSParams.MVP = projView * model;
    this->colorVSParams.LightMVP = lightProjView * model;
    this->colorVSParams.DiffColor = glm::vec3(0.5, 0.5, 0.5);
    this->colorFSParams.EyePos = this->camera.eyePos;
    Gfx::ApplyUniformBlock(this->colorVSParams);
    Gfx::ApplyUniformBlock(this->colorFSParams);
    Gfx::Draw(0);

    // draw rigid bodies using instancing
    this->colorVSParams.Model = glm::mat4();
    this->colorVSParams.MVP = projView;
    this->colorVSParams.LightMVP = lightProjView;
    this->colorVSParams.DiffColor = glm::vec3(1.0f, 1.0f, 1.0f);

    if (this->numSpheres > 0) {
        // draw all spheres in one instanced drawcall
        this->colorInstancedDrawState.Mesh[1] = this->sphereInstMesh;
        Gfx::ApplyDrawState(this->colorInstancedDrawState);
        Gfx::ApplyUniformBlock(this->colorVSParams);
        Gfx::ApplyUniformBlock(this->colorFSParams);
        Gfx::DrawInstanced(1, this->numSpheres);
    }
    if (this->numBoxes > 0) {
        // ...and one instanced drawcall for all boxes
        this->colorInstancedDrawState.Mesh[1] = this->boxInstMesh;
        Gfx::ApplyDrawState(this->colorInstancedDrawState);
        Gfx::ApplyUniformBlock(this->colorVSParams);
        Gfx::ApplyUniformBlock(this->colorFSParams);
        Gfx::DrawInstanced(2, this->numBoxes);
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
BulletPhysicsBasicApp::updatePhysics() {
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
    Physics::Update(1.0f / 60.0f);
    return Clock::Since(physStartTime);
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::updateInstanceData() {

    // this transfers the current rigid body world space transforms into the instance buffers
    this->numBoxes = 0;
    this->numSpheres = 0;
    glm::mat4 model;
    instData* item = nullptr;
    int i;
    for (i = 0; i < numBodies; i++) {
        RigidBodySetup::ShapeType type = Physics::ShapeType(this->bodies[i].id);
        if (RigidBodySetup::SphereShape == type) {
            item = &this->sphereInstData[this->numSpheres++];
        }
        else {
            item = &this->boxInstData[this->numBoxes++];
        }
        model = Physics::Transform(this->bodies[i].id);
        item->xxxx = glm::row(model, 0);
        item->yyyy = glm::row(model, 1);
        item->zzzz = glm::row(model, 2);
    }
    if (this->numSpheres > 0) {
        Gfx::UpdateVertices(this->sphereInstMesh, this->sphereInstData, sizeof(instData) * this->numSpheres);
    }
    if (this->numBoxes > 0) {
        Gfx::UpdateVertices(this->boxInstMesh, this->boxInstData, sizeof(instData) * this->numBoxes);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::setupInput() {
    Input::Setup();
    Input::SetMousePointerLockHandler([this] (const Mouse::Event& event) -> Mouse::PointerLockMode {
        if (event.Button == Mouse::LMB) {
            if (event.Type == Mouse::Event::ButtonDown) {
                this->camera.dragging = true;
                return Mouse::PointerLockModeEnable;
            }
            else if (event.Type == Mouse::Event::ButtonUp) {
                if (this->camera.dragging) {
                    this->camera.dragging = false;
                    return Mouse::PointerLockModeDisable;
                }
            }
        }
        return Mouse::PointerLockModeDontCare;
    });
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::handleInput() {
    const Mouse& mouse = Input::Mouse();
    if (mouse.Attached) {
        if (this->camera.dragging) {
            if (mouse.ButtonPressed(Mouse::LMB)) {
                this->camera.orbital.y -= mouse.Movement.x * 0.01f;
                this->camera.orbital.x = glm::clamp(this->camera.orbital.x + mouse.Movement.y * 0.01f, glm::radians(MinLatitude), glm::radians(MaxLatitude));
            }
        }
        this->camera.dist = glm::clamp(this->camera.dist + mouse.Scroll.y * 0.1f, MinCamDist, MaxCamDist);
    }
}

//------------------------------------------------------------------------------
void
BulletPhysicsBasicApp::updateCamera() {
    this->camera.eyePos = glm::euclidean(this->camera.orbital) * this->camera.dist;
    this->view = glm::lookAt(this->camera.eyePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    this->viewProj = this->proj * this->view;
}

