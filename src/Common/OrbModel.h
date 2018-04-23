#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::OrbModel
    @brief wrap graphics resources for a 3d model created from a .orb file
*/
#include "Core/Containers/InlineArray.h"
#include "Gfx/GfxTypes.h"
#include <glm/vec4.hpp>

namespace Oryol {

struct OrbModel {
    static const int MaxNumMaterials = 8;
    static const int MaxNumSubmeshes = 8;

    struct Material {
        // FIXME
        uint32_t dummy = 0;
    };
    struct Submesh {
        int MaterialIndex = 0;
        PrimitiveGroup PrimGroup;
        bool Visible = false;
    };

    bool IsValid = false;
    BufferDesc VertexBufferDesc;
    BufferDesc IndexBufferDesc;
    glm::vec4 VertexMagnitude;
    Id VertexBuffer;
    Id IndexBuffer;
    Id Skeleton;
    Id AnimLib;
    InlineArray<Material, MaxNumMaterials> Materials;
    InlineArray<Submesh, MaxNumSubmeshes> Submeshes;
};

} // namespace Oryol

