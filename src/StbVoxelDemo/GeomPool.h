#pragma once
//------------------------------------------------------------------------------
/**
    @class GeomPool
    @brief a pool of reusable voxel meshes
*/
#include "Volume.h"
#include "Gfx/Setup/GfxSetup.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/Array.h"
#include "shaders.h"

class GeomPool {
public:
    /// initialize the geom pool
    void Setup(const Oryol::GfxSetup& gfxSetup);
    /// discard the geom pool
    void Discard();

    /// alloc a new geom, return geom index
    int Alloc();
    /// free a geom
    void Free(int index);
    /// free all geoms
    void FreeAll();

    Oryol::Id IndexMesh;
    Oryol::Id Pipeline;
    struct Geom {
        Oryol::Id Mesh;
        int NumQuads = 0;
        Oryol::Shader::VSParams VSParams;
    };
    static const int NumGeoms = 700;
    Oryol::StaticArray<Geom, NumGeoms> Geoms;
    Oryol::Array<int> freeGeoms;
};

//------------------------------------------------------------------------------
inline int
GeomPool::Alloc() {
    o_assert(!this->freeGeoms.Empty());
    int index = this->freeGeoms.PopBack();
    o_assert(Oryol::InvalidIndex != index);
    return index;
}

//------------------------------------------------------------------------------
inline void
GeomPool::Free(int index) {
    o_assert_dbg(Oryol::InvalidIndex != index);
    this->freeGeoms.Add(index);
}
