//------------------------------------------------------------------------------
//  RayCheck.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "RayCheck.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Gfx/Gfx.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "shaders.h"

namespace Oryol {

//------------------------------------------------------------------------------
void
RayCheck::Setup(const GfxSetup& gfxSetup) {

    // setup a debug draw shape
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout.Add(VertexAttr::Position, VertexFormat::Float3);
    shapeBuilder.Box(1.0f, 1.0f, 1.0f, 1);
    this->dbgDrawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    Id shd = Gfx::CreateResource(DbgShader::Setup());
    auto pip = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    pip.RasterizerState.SampleCount = gfxSetup.SampleCount;
    pip.BlendState.ColorFormat = gfxSetup.ColorFormat;
    pip.BlendState.DepthFormat = gfxSetup.DepthFormat;
    this->dbgDrawState.Pipeline = Gfx::CreateResource(pip);
}

//------------------------------------------------------------------------------
void
RayCheck::Discard() {
    // empty
}

//------------------------------------------------------------------------------
void
RayCheck::Add(int id, const glm::vec3& p0, const glm::vec3& p1) {
    o_assert_dbg(id >= 0);
    o_assert_dbg((p0.x < p1.x) && (p0.y < p1.y) && (p0.z < p1.z));
    this->boxes[this->numBoxes++] = box(id, p0, p1);
}

//------------------------------------------------------------------------------
int
RayCheck::Test(const glm::vec2& mousePos, const glm::mat4& invView, glm::mat4& invProj) {

    this->dbgIntersectId = -1;

    glm::vec4 nearPlanePos((mousePos.x-0.5f)*2.0f, -(mousePos.y-0.5f)*2.0f, -1.0f, 1.0f);
    glm::vec4 farPlanePos(nearPlanePos);
    farPlanePos.z = +1.0f;

    // FIXME merge invView and invProj
    glm::vec4 nearViewPos  = invProj * nearPlanePos;
    glm::vec4 farViewPos   = invProj * farPlanePos;
    nearViewPos /= nearViewPos.w;
    farViewPos  /= farViewPos.w;
    glm::vec4 nearWorldPos = invView * nearViewPos;
    glm::vec4 farWorldPos  = invView * farViewPos;

    this->dbgMouseRayPos = glm::vec3(nearWorldPos);
    this->dbgMouseRayVec = glm::normalize(glm::vec3(farWorldPos - nearWorldPos));

    const glm::vec3& p = this->dbgMouseRayPos;
    const glm::vec3& n = this->dbgMouseRayVec * 1000.0f;
    for (int i = 0; i < this->numBoxes; i++) {
        float tmin = -1000000.0f;
        float tmax = +1000000.0f;
        const box& b = this->boxes[i];
        if (n.x != 0.0f) {      // if not parallel to X world axis
            float tx0 = (b.p0.x - p.x) / n.x;
            float tx1 = (b.p1.x - p.x) / n.x;
            tmin = glm::max(tmin, glm::min(tx0, tx1));
            tmax = glm::min(tmax, glm::max(tx0, tx1));
        }
        if (n.y != 0.0f) {
            float ty0 = (b.p0.y - p.y) / n.y;
            float ty1 = (b.p1.y - p.y) / n.y;
            tmin = glm::max(tmin, glm::min(ty0, ty1));
            tmax = glm::min(tmax, glm::max(ty0, ty1));
        }
        if (n.z != 0.0f) {
            float tz0 = (b.p0.z - p.z) / n.z;
            float tz1 = (b.p1.z - p.z) / n.z;
            tmin = glm::max(tmin, glm::min(tz0, tz1));
            tmax = glm::min(tmax, glm::max(tz0, tz1));
        }
        if (tmax >= tmin) {
            this->dbgIntersectId = b.id;
            return b.id;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
void
RayCheck::RenderDebug(const glm::mat4& viewProj) {
    static const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
    static const glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
    glm::mat4 m = glm::translate(glm::mat4(1.0f), this->dbgMouseRayPos + this->dbgMouseRayVec*50.0f);
    DbgShader::KCDBGVSParams vsParams;
    vsParams.ModelViewProjection = viewProj * m;
    vsParams.Color = this->dbgIntersectId == -1 ? red : green;
    Gfx::ApplyDrawState(this->dbgDrawState);
    Gfx::ApplyUniformBlock(vsParams);
    Gfx::Draw();
}

} // namespace Oryol
