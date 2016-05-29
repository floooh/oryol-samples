#pragma once
//------------------------------------------------------------------------------
/**
    @class RayCheck
    @brief test mouse ray against a set of AABB's
*/
#include "Core/Types.h"
#include "Core/Containers/StaticArray.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "Gfx/Gfx.h"

namespace Oryol {

class RayCheck {
public:
    /// setup the ray checker
    void Setup(const GfxSetup& gfxSetup);
    /// discard the ray checker
    void Discard();

    /// add a axis-aligned bounding box in global space
    void Add(int id, const glm::vec3& p0, const glm::vec3& p1);
    /// test mouse ray against bounding box, return id, or -1 if nothing hit
    int Test(const glm::vec2& mousePos, const glm::mat4& invView, glm::mat4& invProj);

    /// render debug visualization
    void RenderDebug(const glm::mat4& viewProj);

    float nearZ = 1.0f;
    float farZ = 100.0f;
    struct box {
        box() : id(-1) { };
        box(int id_, const glm::vec3& p0_, const glm::vec3& p1_) : p0(p0_), p1(p1_), id(id_) { };
        glm::vec3 p0;
        glm::vec3 p1;
        int id;
    };
    static const int MaxNumBoxes = 16;
    int numBoxes = 0;
    StaticArray<box, MaxNumBoxes> boxes;
    glm::vec3 mouseRayPos;
    glm::vec3 mouseRayVec;

    DrawState dbgDrawState;
};

} // namespace Oryol
