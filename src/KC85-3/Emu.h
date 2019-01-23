#pragma once
//------------------------------------------------------------------------------
/**
    @class Emu
    @brief wrapper class for the actual KC85/3 emulator.
*/
#include "emu/emu.h"
#include "Gfx/Gfx.h"
#include "Core/Time/Duration.h"
#include "Input/Input.h"
#include "glm/mat4x4.hpp"

namespace Oryol {

class Emu {
public:
    /// setup KC85/3 instance
    void Setup(const GfxSetup& gfxSetup);
    /// discard the KC85/3 instance
    void Discard();
    /// tick the emulator
    void Tick(Duration frameTime);
    /// render the emulator output
    void Render(const glm::mat4& mvp);

    /// switch on/off
    void TogglePower();
    /// return true if switched on
    bool SwitchedOn() const;
    /// reset the KC85/3
    void Reset();
    /// load and start a game by name
    void StartGame(const char* name);

    static const uint32_t InvalidFrameIndex = 0xFFFFFFFF;
    uint32_t frameIndex = 0;
    uint32_t startGameFrameIndex = InvalidFrameIndex;
    const char* startGamePath = nullptr;

    bool switchedOn = false;
    DrawState drawState;
    Input::CallbackId inputCallbackId = 0;

    kc85_t kc85;
    uint32_t pixelBuffer[320 * 256];
};

} // namespace Oryol

