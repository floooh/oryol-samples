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

constexpr uint32_t VBufSignature = 1;
constexpr uint32_t IBufSignature = 2;
constexpr uint32_t AnimSkeletonSignature = 3;
constexpr uint32_t AnimLibrarySignature = 4;

struct meshDesc {
    BufferDesc vertexBufferDesc;
    BufferDesc indexBufferDesc;
    VertexLayout layout;
    int numVertices = 0;
    int numIndices = 0;
    IndexType::Code indexType = IndexType::UInt16;
    InlineArray<PrimitiveGroup, 16> primGroups;
};

//------------------------------------------------------------------------------
static meshDesc makeMeshDesc(const uint8_t* data, const OrbFile& orb, const StringAtom& name) {
    meshDesc desc;
    for (const auto& src : orb.VertexComps) {
        VertexLayout::Component dst;
        switch (src.Attr) {
            case OrbVertexAttr::Position:   dst.Name = "position"; break;
            case OrbVertexAttr::Normal:     dst.Name = "normal"; break;
            case OrbVertexAttr::TexCoord0:  dst.Name = "texcoord0"; break;
            case OrbVertexAttr::TexCoord1:  dst.Name = "texcoord1"; break;
            case OrbVertexAttr::TexCoord2:  dst.Name = "texcoord2"; break;
            case OrbVertexAttr::TexCoord3:  dst.Name = "texcoord3"; break;
            case OrbVertexAttr::Tangent:    dst.Name = "tangent"; break;
            case OrbVertexAttr::Binormal:   dst.Name = "binormal"; break;
            case OrbVertexAttr::Weights:    dst.Name = "weights"; break;
            case OrbVertexAttr::Indices:    dst.Name = "indices"; break;
            case OrbVertexAttr::Color0:     dst.Name = "color0"; break;
            case OrbVertexAttr::Color1:     dst.Name = "color1"; break;
            default: break;
        }
        switch (src.Format) {
            case OrbVertexFormat::Float:    dst.Format = VertexFormat::Float; break;
            case OrbVertexFormat::Float2:   dst.Format = VertexFormat::Float2; break;
            case OrbVertexFormat::Float3:   dst.Format = VertexFormat::Float3; break;
            case OrbVertexFormat::Float4:   dst.Format = VertexFormat::Float4; break;
            case OrbVertexFormat::Byte4:    dst.Format = VertexFormat::Byte4; break;
            case OrbVertexFormat::Byte4N:   dst.Format = VertexFormat::Byte4N; break;
            case OrbVertexFormat::UByte4:   dst.Format = VertexFormat::UByte4N; break;  // NOT A BUG
            case OrbVertexFormat::UByte4N:  dst.Format = VertexFormat::UByte4N; break;
            case OrbVertexFormat::Short2:   dst.Format = VertexFormat::Short2; break;
            case OrbVertexFormat::Short2N:  dst.Format = VertexFormat::Short2N; break;
            case OrbVertexFormat::Short4:   dst.Format = VertexFormat::Short4; break;
            case OrbVertexFormat::Short4N:  dst.Format = VertexFormat::Short4N; break;
            default: break;
        }
        desc.layout.Add(dst);
    }
    for (const auto& subMesh : orb.Meshes) {
        desc.primGroups.Add(PrimitiveGroup(subMesh.FirstIndex, subMesh.NumIndices));
        desc.numVertices += subMesh.NumVertices;
        desc.numIndices += subMesh.NumIndices;
    }
    desc.vertexBufferDesc = BufferDesc()
        .Type(BufferType::VertexBuffer)
        .Locator(Locator(name, VBufSignature))
        .Size(desc.layout.ByteSize() * desc.numVertices)
        .Content(data + orb.VertexDataOffset);
    desc.indexBufferDesc = BufferDesc()
        .Type(BufferType::IndexBuffer)
        .Locator(Locator(name, IBufSignature))
        .Size(sizeof(uint16_t) * desc.numIndices)
        .Content(data + orb.IndexDataOffset);
    desc.indexType = IndexType::UInt16;
    return desc;
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
                dstCurve.Magnitude[i] = srcCurve.Magnitude[i];
            }
        }
    }
    return setup;
}

//------------------------------------------------------------------------------
bool
OrbLoader::Load(const MemoryBuffer& data, const StringAtom& name, OrbModel& model) {
    model = OrbModel();

    // parse the .orb file
    OrbFile orb;
    if (!orb.Parse(data.Data(), data.Size())) {
        return false;
    }
    model.VertexMagnitude = glm::vec4(orb.VertexMagnitude, 1.0f);

    // one mesh for entire model
    auto meshDesc = makeMeshDesc(data.Data(), orb, name);
    model.VertexBufferDesc = meshDesc.vertexBufferDesc;
    model.IndexBufferDesc = meshDesc.indexBufferDesc;
    model.VertexBuffer = Gfx::CreateBuffer(model.VertexBufferDesc);
    model.IndexBuffer = Gfx::CreateBuffer(model.IndexBufferDesc);

    // materials hold shader uniform blocks and textures
    for (int i = 0; i < orb.Materials.Size(); i++) {
        model.Materials.Add();
        // FIXME: setup textures here
    }

    // submeshes link materials to mesh primitive groups
    for (int i = 0; i < orb.Meshes.Size(); i++) {
        OrbModel::Submesh& m = model.Submeshes.Add();
        m.MaterialIndex = orb.Meshes[i].Material;
        m.PrimGroup = meshDesc.primGroups[i];
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

