#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::OrbModel
    @brief wrap graphics resources for a 3d model created from a .orb file
*/
#include "Core/Containers/InlineArray.h"
#include "Gfx/GfxTypes.h"

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
        int PrimitiveGroupIndex = 0;
        bool Visible = false;
    };

    bool IsValid = false;
    VertexLayout Layout;
    Id Mesh;
    Id Skeleton;
    Id AnimLib;
    InlineArray<Material, MaxNumMaterials> Materials;
    InlineArray<Submesh, MaxNumSubmeshes> Submeshes;
};

} // namespace Oryol

