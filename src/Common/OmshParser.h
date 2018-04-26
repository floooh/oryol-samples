#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::OmshParser
    @ingroup Assets
    @brief parse .omsh file content into gfx resource desc structs
*/
#include "Gfx/GfxTypes.h"

namespace Oryol {

class OmshParser {
public:
    /// parsing result
    struct Result {
        bool Valid = false;
        BufferDesc VertexBufferDesc;
        BufferDesc IndexBufferDesc;
        VertexLayout Layout;
        Oryol::IndexType::Code IndexType = IndexType::None;
        Array<PrimitiveGroup> PrimitiveGroups;
    };
    /// parse .omsh file content from memory
    static Result Parse(const void* ptr, int numBytes);
};

} // namespace Oryol

