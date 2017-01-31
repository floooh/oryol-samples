#pragma once
//------------------------------------------------------------------------------
/**
    @class KC85Emu
    @brief emulator wrapper class
*/
#include "Core/Types.h"
#include "yakc/yakc.h"
#include "yakc_oryol/Draw.h"
#include "yakc_oryol/Audio.h"
#include "yakc_oryol/Keyboard.h"
#include "yakc_oryol/FileLoader.h"
#include "Core/Time/Duration.h"
#include "glm/mat4x4.hpp"

namespace Oryol {

class KC85Emu {
public:
    /// setup the KC85 emulator
    void Setup(const GfxSetup& gfxSetup, YAKC::system m=YAKC::system::kc85_3, YAKC::os_rom os=YAKC::os_rom::caos_3_1);
    /// discard the KC85 emulator
    void Discard();
    /// tick the emulator
    void Update(Duration frameTime);
    /// render the emulator output
    void Render(const glm::mat4& mvp, bool onlyUpdateTexture=false);

    /// switch on/off
    void TogglePower();
    /// return true if switched on
    bool SwitchedOn() const;
    /// reset
    void Reset();
    /// load and start a game by name
    void StartGame(const char* name);

    static const uint32_t InvalidFrameIndex = 0xFFFFFFFF;
    uint32_t frameIndex = 0;
    uint32_t startGameFrameIndex = InvalidFrameIndex;
    const char* startGameName = nullptr;

    uint32_t switchOnDelayFrames = 120;
    DrawState drawState;
    YAKC::system model = YAKC::system::kc85_3;
    YAKC::os_rom rom = YAKC::os_rom::caos_3_1;
    YAKC::yakc emu;
    YAKC::Draw draw;
    YAKC::Audio audio;
    YAKC::Keyboard keyboard;
    YAKC::FileLoader fileLoader;
    Id renderTarget;
};

} // namespace Oryol
