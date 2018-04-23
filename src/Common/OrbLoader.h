#pragma once
//------------------------------------------------------------------------------
/**
    @class Oryol::OrbLoader
    @brief load an .orb file into an OrbModel
*/
#include "Common/OrbModel.h"
#include "Core/Containers/MemoryBuffer.h"

namespace Oryol {

class OrbLoader {
public:
    /// load .orb file data in Buffer object into OrbModel
    static bool Load(const MemoryBuffer& data, const StringAtom& name, OrbModel& outModel);
};

} // namespace Oryol

