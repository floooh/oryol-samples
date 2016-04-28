//------------------------------------------------------------------------------
//  VoxelTest Main.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "Dbg/Dbg.h"
#include "Core/Time/Clock.h"
#include "shaders.h"
#include "GeomPool.h"
#include "GeomMesher.h"
#include "VoxelGenerator.h"
#include "VisTree.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"

using namespace Oryol;

const int MaxChunksGeneratedPerFrame = 1;

class VoxelTest : public App {
public:
    AppState::Code OnInit();
    AppState::Code OnRunning();
    AppState::Code OnCleanup();

    void init_blocks(int frameIndex);
    int bake_geom(const GeomMesher::Result& meshResult);
    void handle_input();

    int frameIndex = 0;
    int lastFrameIndex = -1;
    glm::vec3 lightDir;
    ClearState clearState;

    Camera camera;
    GeomPool geomPool;
    GeomMesher geomMesher;
    VoxelGenerator voxelGenerator;
    VisTree visTree;
};
OryolMain(VoxelTest);

//------------------------------------------------------------------------------
AppState::Code
VoxelTest::OnInit() {
    this->clearState = ClearState::ClearAll(glm::vec4(0.2f, 0.2f, 0.5f, 1.0f), 1.0f, 0);
    auto gfxSetup = GfxSetup::WindowMSAA4(800, 600, "Oryol Voxel Test");
    gfxSetup.SetPoolSize(GfxResourceType::Pipeline, 1024);
    gfxSetup.SetPoolSize(GfxResourceType::Mesh, 1024);
    gfxSetup.ClearHint = this->clearState;
    Gfx::Setup(gfxSetup);
    Input::Setup();
    Input::SetMousePointerLockHandler([](const Mouse::Event& event) -> Mouse::PointerLockMode {
        // switch pointer-lock on/off on left-mouse-button
        if ((event.Button == Mouse::LMB) || (event.Button == Mouse::RMB)) {
            if (event.Type == Mouse::Event::ButtonDown) {
                return Mouse::PointerLockModeEnable;
            }
            else if (event.Type == Mouse::Event::ButtonUp) {
                return Mouse::PointerLockModeDisable;
            }
        }
        return Mouse::PointerLockModeDontCare;
    });
    Dbg::Setup();

    const float fbWidth = (const float) Gfx::DisplayAttrs().FramebufferWidth;
    const float fbHeight = (const float) Gfx::DisplayAttrs().FramebufferHeight;
    this->camera.Setup(glm::vec3(4096, 128, 4096), glm::radians(45.0f), fbWidth, fbHeight, 0.1f, 10000.0f);
    this->lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.25f));

    this->geomPool.Setup(gfxSetup);
    this->geomMesher.Setup();
    // use a fixed display width, otherwise the geom pool could
    // run out of items at high resolutions
    const float displayWidth = 800;
    this->visTree.Setup(displayWidth, glm::radians(45.0f));

    return App::OnInit();
}

//------------------------------------------------------------------------------
int
VoxelTest::bake_geom(const GeomMesher::Result& meshResult) {
    if (meshResult.NumQuads > 0) {
        int geomIndex = this->geomPool.Alloc();
        auto& geom = this->geomPool.Geoms[geomIndex];
        Gfx::UpdateVertices(geom.Mesh, meshResult.Vertices, meshResult.NumBytes);
        geom.NumQuads = meshResult.NumQuads;
        geom.VSParams.Model = glm::mat4();
        geom.VSParams.LightDir = this->lightDir;
        geom.VSParams.Scale = meshResult.Scale;
        geom.VSParams.Translate = meshResult.Translate;
        geom.VSParams.TexTranslate = meshResult.TexTranslate;
        return geomIndex;
    }
    else {
        return VisNode::EmptyGeom;
    }
}

//------------------------------------------------------------------------------
AppState::Code
VoxelTest::OnRunning() {
    this->frameIndex++;
    this->handle_input();

    Gfx::ApplyDefaultRenderTarget(this->clearState);

    // traverse the vis-tree
    this->visTree.Traverse(this->camera);
    // free any geoms to be freed
    while (!this->visTree.freeGeoms.Empty()) {
        int geom = this->visTree.freeGeoms.PopBack();
        this->geomPool.Free(geom);
    }
    // init new geoms
    if (!this->visTree.geomGenJobs.Empty()) {
        int numProcessedJobs = 0;
        while ((numProcessedJobs < MaxChunksGeneratedPerFrame) && !this->visTree.geomGenJobs.Empty()) {
            numProcessedJobs++;
            int16_t     geoms[VisNode::NumGeoms];
            int numGeoms = 0;
            VisTree::GeomGenJob job = this->visTree.geomGenJobs.PopBack();
            Volume vol = this->voxelGenerator.GenSimplex(job.Bounds);
            GeomMesher::Result meshResult;
            this->geomMesher.Start();
            this->geomMesher.StartVolume(vol);
            do {
                meshResult = this->geomMesher.Meshify();
                meshResult.Scale = job.Scale;
                meshResult.Translate = job.Translate;
                if (meshResult.BufferFull) {
                    int geom = this->bake_geom(meshResult);
                    o_assert(numGeoms < VisNode::NumGeoms);
                    geoms[numGeoms++] = geom;
                }
            }
            while (!meshResult.VolumeDone);
            int geom = this->bake_geom(meshResult);
            o_assert(numGeoms < VisNode::NumGeoms);
            geoms[numGeoms++] = geom;
            this->visTree.ApplyGeoms(job.NodeIndex, geoms, numGeoms);
        }
    }

    // render visible geoms
    const int numDrawNodes = this->visTree.drawNodes.Size();
    int numQuads = 0;
    int numGeoms = 0;
    DrawState drawState;
    drawState.Mesh[0] = this->geomPool.IndexMesh;
    drawState.Pipeline = this->geomPool.Pipeline;
    for (int i = 0; i < numDrawNodes; i++) {
        const VisNode& node = this->visTree.NodeAt(this->visTree.drawNodes[i]);
        for (int geomIndex = 0; geomIndex < VisNode::NumGeoms; geomIndex++) {
            if (node.geoms[geomIndex] >= 0) {
                auto& geom = this->geomPool.Geoms[node.geoms[geomIndex]];
                drawState.Mesh[1] = geom.Mesh;
                geom.VSParams.ModelViewProjection = this->camera.ViewProj;
                Gfx::ApplyDrawState(drawState);
                Gfx::ApplyUniformBlock(geom.VSParams);
                Gfx::Draw(PrimitiveGroup(0, geom.NumQuads*6));
                numQuads += geom.NumQuads;
                numGeoms++;
            }
        }
    }
    Dbg::PrintF("\n\r"
                " Desktop:  LMB+Mouse or AWSD to move, RMB+Mouse to look around\n\r"
                " Mobile:   touch+pan to fly\n\n\r"
                " draws: %d\n\r"
                " tris: %d\n\r"
                " avail geoms: %d\n\r"
                " avail nodes: %d\n\r"
                " pending chunks: %d\n\r",
                numGeoms, numQuads*2,
                this->geomPool.freeGeoms.Size(),
                this->visTree.freeNodes.Size(),
                this->visTree.geomGenJobs.Size());
    Dbg::DrawTextBuffer();
    Gfx::CommitFrame();

    return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

//------------------------------------------------------------------------------
AppState::Code
VoxelTest::OnCleanup() {
    this->visTree.Discard();
    this->geomMesher.Discard();
    this->geomPool.Discard();
    Dbg::Discard();
    Input::Discard();
    Gfx::Discard();
    return App::OnCleanup();
}

//------------------------------------------------------------------------------
void
VoxelTest::handle_input() {
    glm::vec3 move;
    glm::vec2 rot;
    const float vel = 0.75f;
    const Keyboard& kbd = Input::Keyboard();
    if (kbd.Attached) {
        if (kbd.KeyPressed(Key::W) || kbd.KeyPressed(Key::Up)) {
            move.z -= vel;
        }
        if (kbd.KeyPressed(Key::S) || kbd.KeyPressed(Key::Down)) {
            move.z += vel;
        }
        if (kbd.KeyPressed(Key::A) || kbd.KeyPressed(Key::Left)) {
            move.x -= vel;
        }
        if (kbd.KeyPressed(Key::D) || kbd.KeyPressed(Key::Right)) {
            move.x += vel;
        }
    }
    const Mouse& mouse = Input::Mouse();
    if (mouse.Attached) {
        if (mouse.ButtonPressed(Mouse::Button::LMB)) {
            move.z -= vel;
        }
        if (mouse.ButtonPressed(Mouse::Button::LMB) || mouse.ButtonPressed(Mouse::Button::RMB)) {
            rot = mouse.Movement * glm::vec2(-0.01f, -0.007f);
        }
    }
    const Touchpad& touch = Input::Touchpad();
    if (touch.Attached) {
        if (touch.Panning) {
            move.z -= vel;
            rot = touch.Movement[0] * glm::vec2(-0.01f, 0.01f);
        }
    }
    this->camera.MoveRotate(move, rot);
}
