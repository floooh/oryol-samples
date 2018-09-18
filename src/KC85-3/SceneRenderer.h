#pragma once
//------------------------------------------------------------------------------
/**
    @class SceneRenderer
    @brief setup and render the voxel scene
*/
#include "Core/Types.h"
#include "Gfx/Gfx.h"
#include "shaders.h"

namespace Oryol {

class SceneRenderer {
public:
    /// setup the scene renderer
    void Setup(const GfxDesc& gfxDesc);
    /// discard the scene renderer
    void Discard();
    /// render the voxel scene
    void Render(const glm::mat4& viewProj);

    static const int MaxNumVertices = (1<<16);          // max vertices per voxel mesh
    static const int MaxNumQuads = MaxNumVertices / 4;
    static const int MaxNumIndices = MaxNumQuads * 6;

    struct voxMesh {
        Id buffer;
        int numQuads = 0;
    };

    Id createIndexBuffer();
    Array<voxMesh> createVoxelBuffers(const VertexLayout& layout);
    void setupDrawState(const GfxDesc& gfxDesc, const VertexLayout& layout);
    void setupShaderParams();

    DrawState drawState;
    VoxelShader::vsParams vsParams;
    Id indexBuffer;
    Array<voxMesh> voxelBuffers;
};

} // namespace Oryol
