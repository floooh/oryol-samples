//------------------------------------------------------------------------------
//  OrbFile.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include <stdint.h>
#include "OrbFile.h"
#include "Anim/Anim.h"

namespace Oryol {

//------------------------------------------------------------------------------
bool
OrbFile::HasCharacter() const {
    return !this->Bones.Empty();
}

//------------------------------------------------------------------------------
bool
OrbFile::Parse(const uint8_t* orbFileData, int orbFileSize) {
    const uint8_t* start = orbFileData;
    this->Start = start;
    const uint8_t* end = orbFileData + orbFileSize;

    if ((start + sizeof(OrbHeader)) >= end) return false;
    const OrbHeader* hdr = (const OrbHeader*) start;
    if (hdr->Magic != 'ORB1') return false;
    for (int i = 0; i < 3; i++) {
        this->VertexMagnitude[i] = hdr->VertexMagnitude[i];
    }

    // setup item array slices
    VertexComps = Slice<OrbVertexComponent>((OrbVertexComponent*)&start[hdr->VertexComponentOffset], hdr->NumVertexComponents);
    if ((const uint8_t*)VertexComps.end() > end) return false;
    ValueProps = Slice<OrbValueProperty>((OrbValueProperty*)&start[hdr->ValuePropOffset], hdr->NumValueProps);
    if ((const uint8_t*)ValueProps.end() > end) return false;
    TexProps = Slice<OrbTextureProperty>((OrbTextureProperty*)&start[hdr->TexturePropOffset], hdr->NumTextureProps);
    if ((const uint8_t*)TexProps.end() > end) return false;
    Materials = Slice<OrbMaterial>((OrbMaterial*)&start[hdr->MaterialOffset], hdr->NumMaterials);
    if ((const uint8_t*)Materials.end() > end) return false;
    Meshes = Slice<OrbMesh>((OrbMesh*)&start[hdr->MeshOffset], hdr->NumMeshes);
    if ((const uint8_t*)Meshes.end() > end) return false;
    Bones = Slice<OrbBone>((OrbBone*)&start[hdr->BoneOffset], hdr->NumBones);
    if ((const uint8_t*)Bones.end() > end) return false;
    Nodes = Slice<OrbNode>((OrbNode*)&start[hdr->NodeOffset], hdr->NumNodes);
    if ((const uint8_t*)Nodes.end() > end) return false;
    AnimKeyComps = Slice<OrbAnimKeyComponent>((OrbAnimKeyComponent*)&start[hdr->AnimKeyComponentOffset], hdr->NumAnimKeyComponents);
    if ((const uint8_t*)AnimKeyComps.end() > end) return false;
    AnimCurves = Slice<OrbAnimCurve>((OrbAnimCurve*)&start[hdr->AnimCurveOffset], hdr->NumAnimCurves);
    if ((const uint8_t*)AnimCurves.end() > end) return false;
    AnimClips = Slice<OrbAnimClip>((OrbAnimClip*)&start[hdr->AnimClipOffset], hdr->NumAnimClips);
    if ((const uint8_t*)AnimClips.end() > end) return false;

    // vertex-, index- and anim data blobs
    if ((start + hdr->VertexDataOffset + hdr->VertexDataSize) > end) return false;
    if ((start + hdr->IndexDataOffset + hdr->IndexDataSize) > end) return false;
    if ((start + hdr->AnimKeyDataOffset + hdr->AnimKeyDataSize) > end) return false;
    if ((start + hdr->StringPoolDataOffset + hdr->StringPoolDataSize) > end) return false;
    VertexDataOffset = hdr->VertexDataOffset;
    VertexDataSize = hdr->VertexDataSize;
    IndexDataOffset = hdr->IndexDataOffset;
    IndexDataSize = hdr->IndexDataSize;
    AnimDataOffset = hdr->AnimKeyDataOffset;
    AnimDataSize = hdr->AnimKeyDataSize;

    // build the string pool
    const char* stringStart = (const char*) (start + hdr->StringPoolDataOffset);
    const char* stringEnd = stringStart;
    const char* stringPoolEnd = stringStart + hdr->StringPoolDataSize;
    if ((const uint8_t*)stringPoolEnd > end) return false;
    while (stringEnd < stringPoolEnd) {
        if (*stringEnd++ == 0) {
            Strings.Add(stringStart);
            stringStart = stringEnd;
        }
    }
    return true;
}

} // namespace Oryol


