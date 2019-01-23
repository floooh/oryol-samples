//------------------------------------------------------------------------------
//  BulletPhysicsBasic.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Core/Time/Clock.h"
#include "Common/CameraHelper.h"
#include "PhysicsCommon/Physics.h"
#include "PhysicsCommon/ShapeRenderer.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Input/Input.h"
#include "shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"

using namespace Oryol;

static const float SphereRadius = 1.0f;
static const float BoxSize = 1.5f;

class BulletPhysicsBasicApp : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    Duration updatePhysics();
    Duration updateInstanceData();

    int frameIndex = 0;
    TimePoint lapTimePoint;
    CameraHelper camera;
    ShapeRenderer shapeRenderer;
    ColorShader::vsParams colorVSParams;
    ColorShader::fsParams colorFSParams;
    ShadowShader::vsParams shadowVSParams;
    glm::mat4 lightProjView;

    Id planeShape;
    Id boxShape;
    Id sphereShape;
    Id groundRigidBody;
    static const int MaxNumBodies = 512;
    int numBodies = 0;
    StaticArray<Id, MaxNumBodies> bodies;
};
OryolMain(BulletPhysicsBasicApp);

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnInit() {

    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsBasic");
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f));
    gfxSetup.HtmlTrackElementSize = true;
    Gfx::Setup(gfxSetup);
    this->colorFSParams.shadowMapSize = glm::vec2(float(this->shapeRenderer.ShadowMapSize));

    // instanced shape rendering helper class
    this->shapeRenderer.ColorShader = Gfx::CreateResource(ColorShader::Setup());
    this->shapeRenderer.ColorShaderInstanced = Gfx::CreateResource(ColorShaderInstanced::Setup());
    this->shapeRenderer.ShadowShader = Gfx::CreateResource(ShadowShader::Setup());
    this->shapeRenderer.ShadowShaderInstanced = Gfx::CreateResource(ShadowShaderInstanced::Setup());
    this->shapeRenderer.SphereRadius = SphereRadius;
    this->shapeRenderer.BoxSize = BoxSize;
    this->shapeRenderer.Setup(gfxSetup);

    // setup directional light (for lighting and shadow rendering)
    glm::mat4 lightView = glm::lookAt(glm::vec3(50.0f, 50.0f, -50.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // this shifts the post-projection Z coordinate into the range 0..1 (like D3D), 
    // instead of -1..+1 (like OpenGL), which makes sure that objects in the
    // range -1.0 to +1.0 are not clipped away in D3D, the shadow map lookup in D3D
    // also needs to invert the Y coordinate (that's handled in the pixel shader
    // where the lookup happens)
    glm::mat4 lightProj = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 1.0f));
    lightProj = glm::scale(lightProj, glm::vec3(1.0f, 1.0f, 0.5f));
    lightProj = lightProj * glm::ortho(-75.0f, +75.0f, -75.0f, +75.0f, 1.0f, 400.0f);
    this->lightProjView = lightProj * lightView;
    this->colorFSParams.lightDir = glm::vec3(glm::column(glm::inverse(lightView), 2));

    Input::Setup();
    Dbg::Setup();
    this->camera.Setup();

    // setup the initial physics world
    Physics::Setup();
    this->boxShape = Physics::Create(CollideShapeSetup::Box(glm::vec3(BoxSize)));
    this->sphereShape = Physics::Create(CollideShapeSetup::Sphere(SphereRadius));
    this->planeShape = Physics::Create(CollideShapeSetup::Plane(glm::vec4(0, 1, 0, 0)));
    this->groundRigidBody = Physics::Create(RigidBodySetup::FromShape(this->planeShape, glm::mat4(1.0f), 0.0f, 0.25f));
    Physics::Add(this->groundRigidBody);

    this->lapTimePoint = Clock::Now();
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    Duration frameTime = Clock::LapTime(this->lapTimePoint);
    Duration physicsTime = this->updatePhysics();
    Duration instUpdTime = this->updateInstanceData();
    this->camera.Update();

    // the shadow pass
    this->shadowVSParams.mvp = this->lightProjView;
    Gfx::BeginPass(this->shapeRenderer.ShadowPass);
    this->shapeRenderer.DrawShadows(this->shadowVSParams);
    Gfx::EndPass();

    // the color pass
    Gfx::BeginPass();

    // draw ground
    const glm::mat4 model = Physics::Transform(this->groundRigidBody);
    this->colorVSParams.model = model;
    this->colorVSParams.mvp = this->camera.ViewProj * model;
    this->colorVSParams.lightMVP = lightProjView * model;
    this->colorVSParams.diffColor = glm::vec3(0.5, 0.5, 0.5);
    this->colorFSParams.eyePos = this->camera.EyePos;
    this->shapeRenderer.DrawGround(this->colorVSParams, this->colorFSParams);

    // draw the dynamic shapes
    this->colorVSParams.model = glm::mat4();
    this->colorVSParams.mvp = this->camera.ViewProj;
    this->colorVSParams.lightMVP = lightProjView;
    this->colorVSParams.diffColor = glm::vec3(1.0f, 1.0f, 1.0f);
    this->shapeRenderer.DrawShapes(this->colorVSParams, this->colorFSParams);

    Dbg::PrintF("\n\r"
                "  Mouse left click + drag: rotate camera\n\r"
                "  Mouse wheel: zoom camera\n\r"
                "  P: pause/continue\n\n\r"
                "  Frame time:          %.4f ms\n\r"
                "  Physics time:        %.4f ms\n\r"
                "  Instance buffer upd: %.4f ms\n\r"
                "  Num Rigid Bodies:    %d\n\r",
                frameTime.AsMilliSeconds(),
                physicsTime.AsMilliSeconds(),
                instUpdTime.AsMilliSeconds(),
                this->numBodies);
    Dbg::DrawTextBuffer();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnCleanup() {
    // FIXME: Physics should just cleanup this stuff on shutdown!
    Physics::Remove(this->groundRigidBody);
    Physics::Destroy(this->groundRigidBody);
    for (int i = 0; i < this->numBodies; i++) {
        Physics::Remove(this->bodies[i]);
        Physics::Destroy(this->bodies[i]);
    }
    Physics::Destroy(this->sphereShape);
    Physics::Destroy(this->boxShape);
    Physics::Destroy(this->planeShape);
    Physics::Discard();
    Dbg::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
Duration
BulletPhysicsBasicApp::updatePhysics() {
    TimePoint startTime = Clock::Now();
    if (!this->camera.Paused) {
        // emit new rigid bodies
        this->frameIndex++;
        if ((this->frameIndex % 10) == 0) {
            if (this->numBodies < MaxNumBodies) {
                static const glm::mat4 tform = glm::translate(glm::mat4(), glm::vec3(0, 3, 0));
                Id newObj;
                if (this->numBodies & 1) {
                    newObj = Physics::Create(RigidBodySetup::FromShape(this->sphereShape, tform, 1.0f, 0.5f));
                }
                else {
                    newObj = Physics::Create(RigidBodySetup::FromShape(this->boxShape, tform, 1.0f, 0.5f));
                }
                Physics::Add(newObj);
                this->bodies[this->numBodies] = newObj;
                this->numBodies++;

                btRigidBody* body = Physics::RigidBody(newObj);
                glm::vec3 ang = glm::ballRand(10.0f);
                glm::vec3 lin = (glm::ballRand(1.0f) + glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec3(2.5f, 20.0f, 2.5f);
                body->setAngularVelocity(btVector3(ang.x, ang.y, ang.z));
                body->setLinearVelocity(btVector3(lin.x, lin.y, lin.z));
                body->setDamping(0.1f, 0.1f);
            }
        }
        // step the physics world
        Physics::Update(1.0f / 60.0f);
    }
    return Clock::Since(startTime);
}

//------------------------------------------------------------------------------
Duration
BulletPhysicsBasicApp::updateInstanceData() {
    TimePoint startTime = Clock::Now();
    this->shapeRenderer.BeginTransforms();
    for (int i = 0; i < numBodies; i++) {
        CollideShapeSetup::ShapeType type = Physics::RigidBodyShapeType(this->bodies[i]);
        if (CollideShapeSetup::SphereShape == type) {
            this->shapeRenderer.UpdateSphereTransform(Physics::Transform(this->bodies[i]));
        }
        else {
            this->shapeRenderer.UpdateBoxTransform(Physics::Transform(this->bodies[i]));
        }
    }
    this->shapeRenderer.EndTransforms();
    return Clock::Since(startTime);
}

