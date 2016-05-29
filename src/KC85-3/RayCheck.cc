//------------------------------------------------------------------------------
//  RayCheck.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "RayCheck.h"
#include "glm/gtc/matrix_transform.hpp"
#include "Gfx/Gfx.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "emu_shaders.h"

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

    glm::vec4 nearPlanePos((mousePos.x-0.5f)*2.0f, -(mousePos.y-0.5f)*2.0f, -1.0f, 1.0f);
    glm::vec4 farPlanePos(nearPlanePos);
    farPlanePos.z = +1.0f;

    // FIXME: merge invView and invProj
    glm::vec4 nearViewPos  = invProj * nearPlanePos;
    glm::vec4 farViewPos   = invProj * farPlanePos;
    nearViewPos /= nearViewPos.w;
    farViewPos  /= farViewPos.w;
    glm::vec4 nearWorldPos = invView * nearViewPos;
    glm::vec4 farWorldPos  = invView * farViewPos;

    this->mouseRayPos = glm::vec3(nearWorldPos);
    this->mouseRayVec = glm::normalize(glm::vec3(farWorldPos - nearWorldPos));

    return -1;
}

//------------------------------------------------------------------------------
void
RayCheck::RenderDebug(const glm::mat4& viewProj) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), this->mouseRayPos + this->mouseRayVec*50.0f);
    DbgShader::KCVSParams vsParams;
    vsParams.ModelViewProjection = viewProj * m;
    Gfx::ApplyDrawState(this->dbgDrawState);
    Gfx::ApplyUniformBlock(vsParams);
    Gfx::Draw(0);
}

} // namespace Oryol
