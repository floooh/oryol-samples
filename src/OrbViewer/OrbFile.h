#pragma once
//------------------------------------------------------------------------------
/**
    @class OrbFile
    @brief implement a safe loader for .orb files
    
    This parses an raw in-memory dump of an .orb file into array slices
    which allow structured access to the file content. After calling
    the Parse() method, the original data must remain valid, the OrbFile
    object will only reference this data, not take ownership!
*/
#include "OrbFileFormat.h"
#include "Core/Containers/Slice.h"
#include "Core/Containers/InlineArray.h"
#include "Gfx/GfxTypes.h"
#include "Anim/AnimTypes.h"

namespace Oryol {

struct OrbFile {
    /// parse from in-memory .orb file
    bool Parse(const uint8_t* orbFileData, int orbFileSize);
    /// create a mesh setup object for the mesh data in the orb file
    MeshSetup MakeMeshSetup() const;
    /// test if the file contains a character
    bool HasCharacter() const;
    /// create an AnimSkeletonSetup object (must be a character)
    AnimSkeletonSetup MakeSkeletonSetup(const StringAtom& name) const;
    /// create an AnimLibrarySetup object (must be a character)
    AnimLibrarySetup MakeAnimLibSetup(const StringAtom& name) const;

    Slice<OrbVertexComponent> VertexComps;
    Slice<OrbValueProperty> ValueProps;
    Slice<OrbTextureProperty> TexProps;
    Slice<OrbMaterial> Materials;
    Slice<OrbMesh> Meshes;
    Slice<OrbBone> Bones;
    Slice<OrbNode> Nodes;
    Slice<OrbAnimKeyComponent> AnimKeyComps;
    Slice<OrbAnimCurve> AnimCurves;
    Slice<OrbAnimClip> AnimClips;
    int VertexDataOffset = 0;
    int VertexDataSize = 0;
    int IndexDataOffset = 0;
    int IndexDataSize = 0;
    int AnimDataOffset = 0;
    int AnimDataSize = 0;
    static const int MaxStringPoolSize = 1024;
    InlineArray<const char*, MaxStringPoolSize> Strings;
};

} // namespace Oryol
