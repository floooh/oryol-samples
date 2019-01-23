//------------------------------------------------------------------------------
//  BulletPhysicsCloth.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Dbg/Dbg.h"
#include "Input/Input.h"
#include "Core/Time/Clock.h"
#include "Common/CameraHelper.h"
#include "PhysicsCommon/Physics.h"
#include "PhysicsCommon/ShapeRenderer.h"
#include "shaders.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"
#include <string.h>

using namespace Oryol;

static const float SphereRadius = 1.0f;
static const float BoxSize = 1.5f;
static const int NumClothSegments = 25;
static const int NumClothQuads = (NumClothSegments-1)*(NumClothSegments-1);
static const int NumClothTriangles = NumClothQuads*2;
static const int NumClothVertices = NumClothTriangles*3;

class BulletPhysicsClothApp : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    Duration updatePhysics();
    Duration updateInstanceData();
    Duration updateClothData();

    bool clothFlatShading = false;
    int frameIndex = 0;
    TimePoint lapTimePoint;
    CameraHelper camera;
    ShapeRenderer shapeRenderer;
    Id clothMesh;
    DrawState clothColorDrawState;
    DrawState clothShadowDrawState;
    ColorShader::vsParams colorVSParams;
    ColorShader::fsParams colorFSParams;
    ShadowShader::vsParams shadowVSParams;
    glm::mat4 lightProjView;

    Id planeShape;
    Id boxShape;
    Id sphereShape;
    Id groundRigidBody;
    Id clothSoftBody;
    static const int MaxNumBodies = 256;
    int numBodies = 0;
    StaticArray<Id, MaxNumBodies> bodies;

    struct {
        float pos[3] = { 0, 0, 0 };
        float nrm[3] = { 0, 0, 0 };
    } clothVertices[NumClothVertices];
};
OryolMain(BulletPhysicsClothApp);

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsClothApp::OnInit() {
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "BulletPhysicsCloth");
    gfxSetup.DefaultPassAction = PassAction::Clear(glm::vec4(0.2f, 0.4f, 0.8f, 1.0f));
    gfxSetup.HtmlTrackElementSize = true;
    Gfx::Setup(gfxSetup);
    this->colorFSParams.shadowMapSize = glm::vec2(float(this->shapeRenderer.ShadowMapSize));

    // instanced shape rendering helper class
    const Id colorShader = Gfx::CreateResource(ColorShader::Setup());
    const Id shadowShader = Gfx::CreateResource(ShadowShader::Setup());
    this->shapeRenderer.ColorShader = colorShader;
    this->shapeRenderer.ColorShaderInstanced = Gfx::CreateResource(ColorShaderInstanced::Setup());
    this->shapeRenderer.ShadowShader = shadowShader;
    this->shapeRenderer.ShadowShaderInstanced = Gfx::CreateResource(ShadowShaderInstanced::Setup());
    this->shapeRenderer.SphereRadius = SphereRadius;
    this->shapeRenderer.BoxSize = BoxSize;
    this->shapeRenderer.Setup(gfxSetup);

    // setup a mesh with dynamic vertex buffer and no indices
    // FIXME: use index rendering later
    auto meshSetup = MeshSetup::Empty(NumClothVertices, Usage::Stream);
    meshSetup.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::Normal, VertexFormat::Float3);
    meshSetup.AddPrimitiveGroup(PrimitiveGroup(0, NumClothTriangles*3));
    this->clothMesh = Gfx::CreateResource(meshSetup);
    this->clothColorDrawState.Mesh[0] = this->clothMesh;
    this->clothShadowDrawState.Mesh[0] = this->clothMesh;

    // setup pipeline states (color and shadow pass) for cloth rendering
    auto pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, colorShader);
    pipSetup.DepthStencilState.DepthWriteEnabled = true;
    pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pipSetup.RasterizerState.CullFaceEnabled = false;
    pipSetup.RasterizerState.SampleCount = gfxSetup.SampleCount;
    this->clothColorDrawState.Pipeline = Gfx::CreateResource(pipSetup);

    pipSetup = PipelineSetup::FromLayoutAndShader(meshSetup.Layout, shadowShader);
    pipSetup.DepthStencilState.DepthWriteEnabled = true;
    pipSetup.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pipSetup.RasterizerState.CullFaceEnabled = false;
    pipSetup.RasterizerState.SampleCount = 1;
    pipSetup.BlendState.ColorFormat = PixelFormat::RGBA8;
    pipSetup.BlendState.DepthFormat = PixelFormat::DEPTH;
    this->clothShadowDrawState.Pipeline = Gfx::CreateResource(pipSetup);

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

    // setup the initial physics world
    Physics::Setup();
    this->boxShape = Physics::Create(CollideShapeSetup::Box(glm::vec3(BoxSize)));
    this->sphereShape = Physics::Create(CollideShapeSetup::Sphere(SphereRadius));
    this->planeShape = Physics::Create(CollideShapeSetup::Plane(glm::vec4(0, 1, 0, 0)));

    // create a fixed ground rigid body
    this->groundRigidBody = Physics::Create(RigidBodySetup::FromShape(this->planeShape, glm::mat4(1.0f), 0.0f, 0.25f));
    Physics::Add(this->groundRigidBody);

    // create cloth shape
    auto clothSetup = SoftBodySetup::Patch();
    clothSetup.NumSegments = NumClothSegments;
    clothSetup.Points[0] = glm::vec3(-10.0f, 10.0f, -10.0f);
    clothSetup.Points[1] = glm::vec3(+10.0f, 10.0f, -10.0f);
    clothSetup.Points[2] = glm::vec3(-10.0f, 10.0f, +10.0f);
    clothSetup.Points[3] = glm::vec3(+10.0f, 10.0f, +10.0f);
    this->clothSoftBody = Physics::Create(clothSetup);
    Physics::Add(this->clothSoftBody);

    Input::Setup();
    Dbg::Setup();
    this->camera.Setup();
    camera.Orbital = glm::vec2(glm::radians(40.0f), glm::radians(180.0f));
    camera.Center = glm::vec3(0.0f, 7.0f, 0.0f);
    this->lapTimePoint = Clock::Now();
    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsClothApp::OnRunning() {
    Duration frameTime = Clock::LapTime(this->lapTimePoint);
    Duration physicsTime = this->updatePhysics();
    Duration instUpdTime = this->updateInstanceData();
    Duration clothUpdTime = this->updateClothData();
    this->camera.Update();
    if (Input::KeyboardAttached() && Input::KeyDown(Key::F)) {
        this->clothFlatShading = !this->clothFlatShading;
    }

    // the shadow pass
    Gfx::BeginPass(this->shapeRenderer.ShadowPass);
    this->shadowVSParams.mvp = this->lightProjView;
    this->shapeRenderer.DrawShadows(this->shadowVSParams);
    Gfx::ApplyDrawState(this->clothShadowDrawState);
    Gfx::ApplyUniformBlock(this->shadowVSParams);
    Gfx::Draw();
    Gfx::EndPass();

    // begin color pass rendering
    Gfx::BeginPass();

    // draw ground
    const glm::mat4 model = Physics::Transform(this->groundRigidBody);
    this->colorVSParams.model = model;
    this->colorVSParams.mvp = this->camera.ViewProj * model;
    this->colorVSParams.lightMVP = lightProjView * model;
    this->colorVSParams.diffColor = glm::vec3(0.5, 0.5, 0.5);
    this->colorFSParams.eyePos = this->camera.EyePos;
    this->colorFSParams.specIntensity = 1.0f;
    this->shapeRenderer.DrawGround(this->colorVSParams, this->colorFSParams);

    // draw the dynamic shapes
    this->colorVSParams.model = glm::mat4();
    this->colorVSParams.mvp = this->camera.ViewProj;
    this->colorVSParams.lightMVP = lightProjView;
    this->colorVSParams.diffColor = glm::vec3(1.0f, 1.0f, 1.0f);
    this->shapeRenderer.DrawShapes(this->colorVSParams, this->colorFSParams);

    // draw the cloth patch
    this->colorVSParams.diffColor = glm::vec3(0.6f, 0.0f, 0.05f);
    this->colorFSParams.specIntensity = 0.15f;
    Gfx::ApplyDrawState(this->clothColorDrawState);
    Gfx::ApplyUniformBlock(this->colorVSParams);
    Gfx::ApplyUniformBlock(this->colorFSParams);
    Gfx::Draw();

    Dbg::PrintF("\n\r"
                "  Mouse left click + drag: rotate camera\n\r"
                "  Mouse wheel: zoom camera\n\r"
                "  P: pause/continue\n\r"
                "  F: toggle cloth flat shading\n\n\r"
                "  Frame time:          %.4f ms\n\r"
                "  Physics time:        %.4f ms\n\r"
                "  Instance buffer upd: %.4f ms\n\r"
                "  Cloth buffer upd:    %.4f ms\n\r"
                "  Num Rigid Bodies:    %d\n\r",
                frameTime.AsMilliSeconds(),
                physicsTime.AsMilliSeconds(),
                instUpdTime.AsMilliSeconds(),
                clothUpdTime.AsMilliSeconds(),
                this->numBodies);
    Dbg::DrawTextBuffer();
    Gfx::EndPass();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsClothApp::OnCleanup() {
    // FIXME: Physics should just cleanup this stuff on shutdown!
    Physics::Remove(this->clothSoftBody);
    Physics::Destroy(this->clothSoftBody);
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
BulletPhysicsClothApp::updatePhysics() {
    TimePoint physStartTime = Clock::Now();
    if (!this->camera.Paused) {
        // emit new rigid bodies
        this->frameIndex++;
        if ((this->frameIndex % 60) == 0) {
            if (this->numBodies < MaxNumBodies) {
                float x = this->numBodies & 1 ? -1.0f : 1.0f;
                const glm::mat4 tform = glm::translate(glm::mat4(), glm::vec3(20*x, 3, 0));
                Id newObj = Physics::Create(RigidBodySetup::FromShape(this->sphereShape, tform, 25.0f, 0.5f));
                Physics::Add(newObj);
                this->bodies[this->numBodies] = newObj;
                this->numBodies++;

                btRigidBody* body = Physics::RigidBody(newObj);
                glm::vec3 vel = glm::vec3(-6*x, 30, 0);
                glm::vec3 ang = glm::ballRand(10.0f);
                body->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
                body->setAngularVelocity(btVector3(ang.x, ang.y, ang.z));
                body->setDamping(0.1f, 0.1f);
            }
        }
        Physics::Update(1.0f/60.0f);
    }
    return Clock::Since(physStartTime);
}

//------------------------------------------------------------------------------
Duration
BulletPhysicsClothApp::updateInstanceData() {
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

//------------------------------------------------------------------------------
Duration
BulletPhysicsClothApp::updateClothData() {
    TimePoint startTime = Clock::Now();
    btSoftBody* body = Physics::SoftBody(this->clothSoftBody);
    const int numTris = body->m_faces.size();
    o_assert(numTris == NumClothTriangles);
    for (int triIndex = 0, vtxIndex=0; triIndex < numTris; triIndex++) {
        const auto& face = body->m_faces[triIndex];
        for (int i = 0; i < 3; i++, vtxIndex++) {
            auto& vtx = this->clothVertices[vtxIndex];
            vtx.pos[0] = face.m_n[i]->m_x.m_floats[0];
            vtx.pos[1] = face.m_n[i]->m_x.m_floats[1];
            vtx.pos[2] = face.m_n[i]->m_x.m_floats[2];
            if (this->clothFlatShading) {
                vtx.nrm[0] = -face.m_normal.m_floats[0];
                vtx.nrm[1] = -face.m_normal.m_floats[1];
                vtx.nrm[2] = -face.m_normal.m_floats[2];
            }
            else {
                vtx.nrm[0] = -face.m_n[i]->m_n.m_floats[0];
                vtx.nrm[1] = -face.m_n[i]->m_n.m_floats[1];
                vtx.nrm[2] = -face.m_n[i]->m_n.m_floats[2];
            }
        }
    }
    Gfx::UpdateVertices(this->clothMesh, this->clothVertices, sizeof(this->clothVertices));
    return Clock::Since(startTime);
}
