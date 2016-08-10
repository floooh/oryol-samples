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
    void Setup(const GfxSetup& gfxSetup);
    /// discard the scene renderer
    void Discard();
    /// render the voxel scene
    void Render(const glm::mat4& viewProj);

    static const int MaxNumVertices = (1<<16);          // max vertices per voxel mesh
    static const int MaxNumQuads = MaxNumVertices / 4;
    static const int MaxNumIndices = MaxNumQuads * 6;

    struct voxMesh {
        Id mesh;
        int numQuads = 0;
    };

    Id createIndexMesh();
    Array<voxMesh> createVoxelMeshes(const VertexLayout& layout);
    void setupDrawState(const GfxSetup& gfxSetup, const VertexLayout& layout);
    void setupShaderParams();

    DrawState drawState;
    VoxelShader::VoxelVSParams vsParams;
    Id indexMesh;
    Array<voxMesh> voxelMeshes;
};

} // namespace Oryol
