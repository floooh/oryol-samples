//------------------------------------------------------------------------------
//  BulletPhysicsBasic.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "IMUI/IMUI.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "btBulletDynamicsCommon.h"
#include "shaders.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace Oryol;

class BulletPhysicsBasicApp : public App {
public:
    AppState::Code OnRunning();
    AppState::Code OnInit();
    AppState::Code OnCleanup();

    btBroadphaseInterface* broadphase = nullptr;
    btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
    btCollisionDispatcher* dispatcher = nullptr;
    btSequentialImpulseConstraintSolver* solver = nullptr;
    btDiscreteDynamicsWorld* dynamicsWorld = nullptr;

    btCollisionShape* groundShape = nullptr;
    btDefaultMotionState* groundMotionState = nullptr;
    btRigidBody* groundRigidBody = nullptr;

    btCollisionShape* fallShape = nullptr;
    btDefaultMotionState* fallMotionState = nullptr;
    btRigidBody* fallRigidBody = nullptr;

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
    // - Bullet seems to be designed around creating objects with new/delete, which sucks a bit
    this->broadphase = new btDbvtBroadphase();
    this->collisionConfiguration = new btDefaultCollisionConfiguration();
    this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
    this->solver = new btSequentialImpulseConstraintSolver();
    this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher,
        this->broadphase, this->solver, this->collisionConfiguration);

    this->groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    this->groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, this->groundMotionState, this->groundShape, btVector3(0, 0, 0));
    groundRigidBodyCI.m_restitution = 0.5f;
    this->groundRigidBody = new btRigidBody(groundRigidBodyCI);
    this->dynamicsWorld->addRigidBody(this->groundRigidBody);

    this->fallShape = new btSphereShape(1);
    this->fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
    btScalar mass = 1;
    btVector3 fallInertia(0, 0, 0);
    this->fallShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, this->fallMotionState, this->fallShape, fallInertia);
    fallRigidBodyCI.m_restitution = 1.0f;
    this->fallRigidBody = new btRigidBody(fallRigidBodyCI);
    this->dynamicsWorld->addRigidBody(this->fallRigidBody);

    return App::OnInit();
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnRunning() {

    this->dynamicsWorld->stepSimulation(1.0 / 60.0, 10);

    Gfx::ApplyDefaultRenderTarget(ClearState::ClearAll(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), 1.0f, 0));
    Gfx::ApplyDrawState(this->drawState);

    // draw ground
    btTransform groundTrans;
    this->groundMotionState->getWorldTransform(groundTrans);
    const btVector3& groundPos = groundTrans.getOrigin();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(groundPos.getX(), groundPos.getY(), groundPos.getZ()));
    this->vsParams.ModelViewProjection = this->proj * this->view * model;
    this->vsParams.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(0);

    // draw ball
    btTransform ballTrans;
    this->fallMotionState->getWorldTransform(ballTrans);
    const btVector3& ballPos = ballTrans.getOrigin();
    model = glm::translate(glm::mat4(), glm::vec3(ballPos.getX(), ballPos.getY(), ballPos.getZ()));
    this->vsParams.ModelViewProjection = this->proj * this->view * model;
    this->vsParams.Color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    Gfx::ApplyUniformBlock(this->vsParams);
    Gfx::Draw(1);

    IMUI::NewFrame();
    if (ImGui::Begin("Physics")) {
        ImGui::Text("Ball Height: %f", ballPos.getY());
    }
    ImGui::End();

    ImGui::Render();
    Gfx::CommitFrame();
    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
BulletPhysicsBasicApp::OnCleanup() {


    this->dynamicsWorld->removeRigidBody(this->fallRigidBody);
    delete this->fallMotionState;
    delete this->fallRigidBody;
    delete this->fallShape;

    this->dynamicsWorld->removeRigidBody(this->groundRigidBody);
    delete this->groundMotionState;
    delete this->groundRigidBody;
    delete this->groundShape;

    delete this->dynamicsWorld;
    delete this->solver;
    delete this->dispatcher;
    delete this->collisionConfiguration;
    delete this->broadphase;

    IMUI::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}
