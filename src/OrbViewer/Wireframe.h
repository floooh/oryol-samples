#pragma once
//------------------------------------------------------------------------------
/**
    @class Wireframe
    @brief simple wireframe debug renderer
*/
#include "Gfx/Gfx.h"
#include "Core/Containers/InlineArray.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Oryol {

struct Wireframe {
    
    /// set this to the current ViewProj matrix
    glm::mat4 ViewProj;
    /// set this to the current model matrix
    glm::mat4 Model;
    /// set this to the current line color
    glm::vec4 Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    /// setup the debug renderer
    void Setup(const GfxSetup& setup);
    /// discard the debug renderer
    void Discard();
    /// call once before Gfx::CommitFrame to render the debug scene
    void Render();
    /// add a wireframe line (vec3)
    void Line(const glm::vec3& p0, const glm::vec3& p1);
    /// add a wireframe line (vec4, w must be 1)
    void Line(const glm::vec4& p0, const glm::vec4& p1);

    DrawState drawState;
    ResourceLabel label;
    struct Vertex {
        float x, y, z, r, g, b, a;
        Vertex() { };
        Vertex(const glm::vec4& p, const glm::vec4& c):
            x(p.x), y(p.y), z(p.z), r(c.x), g(c.y), b(c.z), a(c.w) { };
    };
    static const int MaxNumVertices = 4096;
    InlineArray<Vertex, MaxNumVertices> vertices;
};

}
