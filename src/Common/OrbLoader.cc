//------------------------------------------------------------------------------
//  OrbLoader.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "OrbLoader.h"
#include "OrbFile.h"
#include "Gfx/Gfx.h"
#include "Anim/Anim.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Oryol {

const uint32_t MeshSignature = 1;
const uint32_t AnimSkeletonSignature = 2;
const uint32_t AnimLibrarySignature = 3;

//------------------------------------------------------------------------------
static MeshSetup makeMeshSetup(const OrbFile& orb, const Locator& loc) {
    auto setup = MeshSetup::FromData();
    setup.Locator = loc;
    for (const auto& src : orb.VertexComps) {
        VertexLayout::Component dst;
        switch (src.Attr) {
            case OrbVertexAttr::Position:   dst.Attr = VertexAttr::Position; break;
            case OrbVertexAttr::Normal:     dst.Attr = VertexAttr::Normal; break;
            case OrbVertexAttr::TexCoord0:  dst.Attr = VertexAttr::TexCoord0; break;
            case OrbVertexAttr::TexCoord1:  dst.Attr = VertexAttr::TexCoord1; break;
            case OrbVertexAttr::TexCoord2:  dst.Attr = VertexAttr::TexCoord2; break;
            case OrbVertexAttr::TexCoord3:  dst.Attr = VertexAttr::TexCoord3; break;
            case OrbVertexAttr::Tangent:    dst.Attr = VertexAttr::Tangent; break;
            case OrbVertexAttr::Binormal:   dst.Attr = VertexAttr::Binormal; break;
            case OrbVertexAttr::Weights:    dst.Attr = VertexAttr::Weights; break;
            case OrbVertexAttr::Indices:    dst.Attr = VertexAttr::Indices; break;
            case OrbVertexAttr::Color0:     dst.Attr = VertexAttr::Color0; break;
            case OrbVertexAttr::Color1:     dst.Attr = VertexAttr::Color1; break;
            default: break;
        }
        switch (src.Format) {
            case OrbVertexFormat::Float:    dst.Format = VertexFormat::Float; break;
            case OrbVertexFormat::Float2:   dst.Format = VertexFormat::Float2; break;
            case OrbVertexFormat::Float3:   dst.Format = VertexFormat::Float3; break;
            case OrbVertexFormat::Float4:   dst.Format = VertexFormat::Float4; break;
            case OrbVertexFormat::Byte4:    dst.Format = VertexFormat::Byte4; break;
            case OrbVertexFormat::Byte4N:   dst.Format = VertexFormat::Byte4N; break;
            case OrbVertexFormat::UByte4:   dst.Format = VertexFormat::UByte4; break;
            case OrbVertexFormat::UByte4N:  dst.Format = VertexFormat::UByte4N; break;
            case OrbVertexFormat::Short2:   dst.Format = VertexFormat::Short2; break;
            case OrbVertexFormat::Short2N:  dst.Format = VertexFormat::Short2N; break;
            case OrbVertexFormat::Short4:   dst.Format = VertexFormat::Short4; break;
            case OrbVertexFormat::Short4N:  dst.Format = VertexFormat::Short4N; break;
            default: break;
        }
        setup.Layout.Add(dst);
    }
    for (const auto& subMesh : orb.Meshes) {
        setup.AddPrimitiveGroup(PrimitiveGroup(subMesh.FirstIndex, subMesh.NumIndices));
        setup.NumVertices += subMesh.NumVertices;
        setup.NumIndices += subMesh.NumIndices;
    }
    setup.IndicesType = IndexType::Index16;
    setup.VertexDataOffset = orb.VertexDataOffset;
    setup.IndexDataOffset = orb.IndexDataOffset;
    return setup;
}

//------------------------------------------------------------------------------
static AnimSkeletonSetup makeSkeletonSetup(const OrbFile& orb, const Locator& loc) {
    AnimSkeletonSetup setup;
    setup.Locator = loc;
    setup.Bones.Reserve(orb.Bones.Size());
    for (const auto& src : orb.Bones) {
        setup.Bones.Add();
        auto& dst = setup.Bones.Back();
        dst.Name = orb.Strings[src.Name];
        dst.ParentIndex = src.Parent;
        // FIXME: inv bind pose should already be in orb file!
        glm::vec4 t(src.Translate[0], src.Translate[1], src.Translate[2], 1.0f);
        glm::vec3 s(src.Scale[0], src.Scale[1], src.Scale[2]);
        glm::quat r(src.Rotate[3], src.Rotate[0], src.Rotate[1], src.Rotate[2]);
        glm::mat4 m = glm::scale(glm::mat4(), s);
        m = m * glm::mat4_cast(r);
        m[3] = t;
        if (dst.ParentIndex != -1) {
            m = setup.Bones[dst.ParentIndex].BindPose * m;
        }
        dst.InvBindPose = glm::inverse(m);
        dst.BindPose = m;
    }
    return setup;
}

//------------------------------------------------------------------------------
static AnimLibrarySetup makeAnimLibSetup(const OrbFile& orb, const Locator& loc) {
    AnimLibrarySetup setup;
    setup.Locator = loc;
    setup.CurveLayout.Reserve(orb.AnimKeyComps.Size());
    for (const auto& src : orb.AnimKeyComps) {
        AnimCurveFormat::Enum dst;
        switch (src.KeyFormat) {
            case OrbAnimKeyFormat::Float:   dst = AnimCurveFormat::Float; break;
            case OrbAnimKeyFormat::Float2:  dst = AnimCurveFormat::Float2; break;
            case OrbAnimKeyFormat::Float3:  dst = AnimCurveFormat::Float3; break;
            case OrbAnimKeyFormat::Float4:  dst = AnimCurveFormat::Float4; break;
            case OrbAnimKeyFormat::Quaternion:  dst = AnimCurveFormat::Quaternion; break;
            default: dst = AnimCurveFormat::Invalid; break;
        }
        o_assert_dbg(AnimCurveFormat::Invalid != dst);
        setup.CurveLayout.Add(dst);
    }
    setup.Clips.Reserve(orb.AnimClips.Size());
    for (const auto& src : orb.AnimClips) {
        auto& dst = setup.Clips.Add();
        dst.Name = orb.Strings[src.Name];
        dst.Length = src.Length;
        dst.KeyDuration = src.KeyDuration;
        Slice<OrbAnimCurve> srcCurves = orb.AnimCurves.MakeSlice(src.FirstCurve, src.NumCurves);
        dst.Curves.Reserve(srcCurves.Size());
        for (const auto& srcCurve : srcCurves) {
            auto& dstCurve = dst.Curves.Add();
            dstCurve.Static = (-1 == srcCurve.KeyOffset);
            for (int i = 0; i < 4; i++) {
                dstCurve.StaticValue[i] = srcCurve.StaticKey[i];
            }
        }
    }
    return setup;
}

//------------------------------------------------------------------------------
bool
OrbLoader::Load(const Buffer& data, const StringAtom& name, OrbModel& model) {
    model = OrbModel();

    // parse the .n3o file
    OrbFile orb;
    if (!orb.Parse(data.Data(), data.Size())) {
        return false;
    }

    // one mesh for entire model
    auto meshSetup = makeMeshSetup(orb, Locator(name, MeshSignature));
    model.Layout = meshSetup.Layout;
    model.Mesh = Gfx::CreateResource(meshSetup, data.Data(), data.Size());

    // materials hold shader uniform blocks and textures
    for (int i = 0; i < orb.Materials.Size(); i++) {
        OrbModel::Material& mat = model.Materials.Add();
        // FIXME: setup textures here
    }

    // submeshes link materials to mesh primitive groups
    for (int i = 0; i < orb.Meshes.Size(); i++) {
        OrbModel::Submesh& m = model.Submeshes.Add();
        m.MaterialIndex = orb.Meshes[i].Material;
        m.PrimitiveGroupIndex = i;
    }

    // character stuff
    if (orb.HasCharacter()) {
        model.Skeleton = Anim::Create(makeSkeletonSetup(orb, Locator(name, AnimSkeletonSignature)));
        model.AnimLib = Anim::Create(makeAnimLibSetup(orb, Locator(name, AnimLibrarySignature)));
        Anim::WriteKeys(model.AnimLib, orb.Start+orb.AnimDataOffset, orb.AnimDataSize);
    }
    model.IsValid = true;
    return true;
}

} // namespace Oryol

