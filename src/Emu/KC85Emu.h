#pragma once
//------------------------------------------------------------------------------
/**
    @class KC85Emu
    @brief emulator wrapper class
*/
#include "Core/Types.h"
#include "yakc/KC85Oryol.h"
#include "yakc/Draw.h"
#include "yakc/Audio.h"
#include "Core/Time/Duration.h"
#include "glm/mat4x4.hpp"

namespace Oryol {

class KC85Emu {
public:
    /// setup the KC85 emulator
    void Setup(const GfxSetup& gfxSetup);
    /// discard the KC85 emulator
    void Discard();
    /// tick the emulator
    void Update(Duration frameTime);
    /// render the emulator output
    void Render(const glm::mat4& mvp);

    void handleInput();

    DrawState drawState;
    uint8_t last_ascii;
    yakc::emu emu;
    Draw draw;            // FIXME: should be behind yakc namespace!
    Audio audio;          // FIXME: ditto!
    Id renderTarget;
};

} // namespace Oryol
