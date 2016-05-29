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
#include "ui/FileLoader.h"  // FIXME: this should not go under 'UI'
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

    /// switch on/off
    void TogglePower();
    /// return true if switched on
    bool SwitchedOn() const;
    /// reset
    void Reset();
    /// load and start a game by name
    void StartGame(const char* name);

    void handleInput();

    uint32_t frameIndex = 0;
    DrawState drawState;
    uint8_t last_ascii;
    yakc::emu emu;
    yakc::Draw draw;
    yakc::Audio audio;
    yakc::FileLoader fileLoader;
    Id renderTarget;
};

} // namespace Oryol
