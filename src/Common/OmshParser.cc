//------------------------------------------------------------------------------
//  OmshParser.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "OmshParser.h"

namespace Oryol {

//------------------------------------------------------------------------------
OmshParser::Result
OmshParser::Parse(const void* ptr, int size) {
    o_assert_dbg(ptr);
    o_assert_dbg(size > 4);
    Result res;

    // size must be multiple of 4
    if ((size & 3) != 0) {
        return res;
    }

    const uint32_t* u32StartPtr = (const uint32_t*) ptr;
    const uint32_t* u32Ptr = u32StartPtr;
    const uint32_t u32Size = size >> 2;
    const uint32_t* u32EndPtr = u32StartPtr + u32Size;

    // check if enough data for header
    uint32_t u32CheckSize = 7;
    if (u32CheckSize > u32Size) {
        return res;
    }

    // start parsing static header
    const uint32_t magic = *u32Ptr++;
    if (magic != 'OMSH') {
        return res;
    }
    const uint32_t numVertices = *u32Ptr++;
    const uint32_t vertexSize = *u32Ptr++;
    const uint32_t numIndices = *u32Ptr++;
    const uint32_t indexSize = *u32Ptr++;
    const uint32_t numVertexAttrs = *u32Ptr++;
    const uint32_t numPrimGroups = *u32Ptr++;

    // check if enough data for vertex components
    u32CheckSize += numVertexAttrs * 2;
    if (u32CheckSize > u32Size) {
        return res;
    }
    VertexLayout layout;
    for (uint32_t i = 0; i < numVertexAttrs; i++) {
        const char* attrName = nullptr;
        switch (*u32Ptr++) {
            case 0:     attrName = "position"; break;
            case 1:     attrName = "normal"; break;
            case 2:     attrName = "texcoord0"; break;
            case 3:     attrName = "texcoord1"; break;
            case 4:     attrName = "texcoord2"; break;
            case 5:     attrName = "texcoord3"; break;
            case 6:     attrName = "tangent"; break;
            case 7:     attrName = "binormal"; break;
            case 8:     attrName = "weights"; break;
            case 9:     attrName = "indices"; break;
            case 10:    attrName = "color0"; break;
            case 11:    attrName = "color1"; break;
            case 12:    attrName = "instance0"; break;
            case 13:    attrName = "instance1"; break;
            case 14:    attrName = "instance2"; break;
            case 15:    attrName = "instance3"; break;
        }
        o_assert(attrName);
        VertexFormat::Code attrFmt = VertexFormat::Invalid;
        switch (*u32Ptr++) {
            case 0:     attrFmt = VertexFormat::Float; break;
            case 1:     attrFmt = VertexFormat::Float2; break;
            case 2:     attrFmt = VertexFormat::Float3; break;
            case 3:     attrFmt = VertexFormat::Float4; break;
            case 4:     attrFmt = VertexFormat::Byte4; break;
            case 5:     attrFmt = VertexFormat::Byte4N; break;
            case 6:     attrFmt = VertexFormat::UByte4; break;
            case 7:     attrFmt = VertexFormat::UByte4N; break;
            case 8:     attrFmt = VertexFormat::Short2; break;
            case 9:     attrFmt = VertexFormat::Short2N; break;
            case 10:    attrFmt = VertexFormat::Short4; break;
            case 11:    attrFmt = VertexFormat::Short4N; break;
        }
        o_assert(attrFmt != VertexFormat::Invalid);
        layout.Add(attrName, attrFmt);
    }

    // check if enough data for primitive groups
    u32CheckSize += numPrimGroups * 3;
    if (u32CheckSize > u32Size) {
        return res;
    }
    for(uint32_t i = 0; i < numPrimGroups; i++) {
        PrimitiveGroup primGroup;
        // skip primitive type
        u32Ptr++;
        primGroup.BaseElement = *u32Ptr++;
        primGroup.NumElements = *u32Ptr++;
        res.PrimitiveGroups.Add(primGroup);
    }

    // check if enough data for vertices
    const uint32_t u32VertexDataSize = (numVertices * vertexSize) >> 2;
    u32CheckSize += u32VertexDataSize;
    if (u32CheckSize > u32Size) {
        return res;
    }
    const void* vertexDataPtr = u32Ptr;
    u32Ptr += u32VertexDataSize;

    // check if enough data for indices (index block is padded so that size is multiple of 4)
    uint32_t indexDataSize = numIndices * indexSize;
    if ((indexDataSize & 3) != 0) {
        indexDataSize += 2;
        o_assert_dbg((indexDataSize & 3) == 0);
    }
    const uint32_t u32IndexDataSize = indexDataSize >> 2;
    u32CheckSize += u32IndexDataSize;
    if (u32CheckSize > u32Size) {
        return res;
    }
    const void* indexDataPtr = u32Ptr;
    u32Ptr += u32IndexDataSize;
    if (u32Ptr != u32EndPtr) {
        return res;
    }

    // setup the result
    res.Valid = true;
    res.VertexBufferDesc = BufferDesc()
        .Type(BufferType::VertexBuffer)
        .Size(numVertices * vertexSize)
        .Content(vertexDataPtr);
    res.IndexBufferDesc = BufferDesc()
        .Type(BufferType::IndexBuffer)
        .Size(numIndices * indexSize)
        .Content(indexDataPtr);
    res.Layout = layout;
    res.IndexType = (indexSize == 2) ? IndexType::UInt16 : IndexType::UInt32;
    return res;
}

} // namespace Oryol

