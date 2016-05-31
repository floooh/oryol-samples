//------------------------------------------------------------------------------
//  Emu/KC85Emu.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "KC85Emu.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Input/Input.h"
#include "emu_shaders.h"

using namespace yakc;

namespace Oryol {

//------------------------------------------------------------------------------
void
KC85Emu::Setup(const GfxSetup& gfxSetup) {

    // initialize the emulator as KC85/3
    this->emu.kc85.roms.add(kc85_roms::caos31, dump_caos31, sizeof(dump_caos31));
    this->emu.kc85.roms.add(kc85_roms::basic_rom, dump_basic_c0, sizeof(dump_basic_c0));
    this->emu.init();
    this->draw.Setup(gfxSetup, 5);
    this->audio.Setup(this->emu.board.clck);
    this->emu.kc85.audio.setup_callbacks(&this->audio, Audio::cb_sound, Audio::cb_volume, Audio::cb_stop);
    this->fileLoader.Setup(this->emu);

    // register modules
    this->emu.kc85.exp.register_none_module("NO MODULE", "Click to insert module!");
    this->emu.kc85.exp.register_ram_module(kc85_exp::m022_16kbyte, 0xC0, 0x4000, "nohelp");

    // setup a mesh and draw state to render a simple plane
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder.Plane(1.0f, 1.0f, 1);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    Id shd = Gfx::CreateResource(KCShader::Setup());
    auto pip = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    pip.DepthStencilState.DepthWriteEnabled = true;
    pip.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pip.RasterizerState.SampleCount = gfxSetup.SampleCount;
    pip.BlendState.ColorFormat = gfxSetup.ColorFormat;
    pip.BlendState.DepthFormat = gfxSetup.DepthFormat;
    this->drawState.Pipeline = Gfx::CreateResource(pip);
}

//------------------------------------------------------------------------------
void
KC85Emu::Discard() {
    this->fileLoader.Discard();
    this->emu.poweroff();
    this->audio.Discard();
    this->draw.Discard();
}

//------------------------------------------------------------------------------
void
KC85Emu::Update(Duration frameTime) {
    this->frameIndex++;
    if (this->SwitchedOn()) {
        // need to start a game?
        if (this->startGameFrameIndex == this->frameIndex) {
            for (const auto& item : this->fileLoader.Items) {
                if ((int(item.Compat) & int(this->emu.model)) && (item.Name == this->startGameName)) {
                    this->fileLoader.LoadAndStart(this->emu, item);
                    break;
                }
            }
        }

        // handle the normal emulator per-frame update
        this->handleInput();
        int micro_secs = (int) frameTime.AsMicroSeconds();
        const uint64_t cpu_min_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/100;
        const uint64_t cpu_max_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/25;
        const uint64_t audio_cycle_count = this->audio.GetProcessedCycles();
        const uint64_t min_cycle_count = audio_cycle_count + cpu_min_ahead_cycles;
        const uint64_t max_cycle_count = audio_cycle_count + cpu_max_ahead_cycles;
        this->emu.onframe(2, micro_secs, min_cycle_count, max_cycle_count);
        this->audio.Update(this->emu.board.clck);
    }
    else {
        // switch KC on once after a little while
        if (this->frameIndex == 120) {
            if (!this->SwitchedOn()) {
                this->TogglePower();
            }
        }
    }
}

//------------------------------------------------------------------------------
void
KC85Emu::TogglePower() {
    if (this->SwitchedOn()) {
        this->emu.poweroff();
    }
    else {
        this->emu.poweron(device::kc85_3, os_rom::caos_3_1);
        if (!this->emu.kc85.exp.slot_occupied(0x08)) {
            this->emu.kc85.exp.insert_module(0x08, kc85_exp::m022_16kbyte);
        }
        if (!this->emu.kc85.exp.slot_occupied(0x0C)) {
            this->emu.kc85.exp.insert_module(0x0C, kc85_exp::none);
        }
    }
}

//------------------------------------------------------------------------------
bool
KC85Emu::SwitchedOn() const {
    return this->emu.kc85.on;
}

//------------------------------------------------------------------------------
void
KC85Emu::Reset() {
    if (this->SwitchedOn()) {
        this->emu.kc85.reset();
    }
}

//------------------------------------------------------------------------------
void
KC85Emu::StartGame(const char* name) {
    o_assert_dbg(name);

    // switch KC off and on, and start game after it has booted
    if (this->SwitchedOn()) {
        this->TogglePower();
    }
    this->TogglePower();
    this->startGameFrameIndex = this->frameIndex + 5 * 60;
    this->startGameName = name;
}

//------------------------------------------------------------------------------
void
KC85Emu::Render(const glm::mat4& mvp) {

    if (this->SwitchedOn()) {
        // update the offscreen texture
        const int width = 320;
        const int height = 256;
        const Id tex = this->draw.irmTexture320x256;
        this->draw.texUpdateAttrs.Sizes[0][0] = width*height*4;
        Gfx::UpdateTexture(tex, this->emu.kc85.video.LinearBuffer, this->draw.texUpdateAttrs);

        KCShader::KCVSParams vsParams;
        vsParams.ModelViewProjection = mvp;
        this->drawState.FSTexture[KCTextures::IRM] = tex;
        Gfx::ApplyDrawState(this->drawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw(0);
    }
}

//------------------------------------------------------------------------------
void
KC85Emu::handleInput() {
    // FIXME: this should go into the yakc emulator
    const Keyboard& kbd = Input::Keyboard();

    #if YAKC_UI
    const Touchpad& touch = Input::Touchpad();
    // don't handle KC input if IMGUI has the keyboard focus
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    // toggle UI?
    if (kbd.KeyDown(Key::Tab) || touch.DoubleTapped) {
        this->ui.Toggle();
    }
    #endif

    const wchar_t* text = kbd.CapturedText();
    ubyte ascii = 0;

    // alpha-numerics
    if (text[0] && (text[0] >= 32) && (text[0] < 127)) {
        ascii = (ubyte) text[0];
        // shift is inverted on KC
        if (islower(ascii)) {
            ascii = toupper(ascii);
        }
        else if (isupper(ascii)) {
            ascii = tolower(ascii);
        }
    }
    if ((ascii == 0) && (kbd.AnyKeyPressed())) {
        ascii = this->last_ascii;
    }
    this->last_ascii = ascii;

    // special keys
    struct toAscii {
        Key::Code key;
        ubyte ascii;
    };
    const static toAscii keyTable[] = {
        // FIXME:
        //  HOME, PAGE UP/DOWN, START/END of line, INSERT,
        { Key::Left, 0x08 },
        { Key::Right, 0x09 },
        { Key::Down, 0x0A },
        { Key::Up, 0x0B },
        { Key::Enter, 0x0D },
        { Key::BackSpace, 0x01 },
        { Key::Escape, 0x03 },
        { Key::F1, 0xF1 },
        { Key::F2, 0xF2 },
        { Key::F3, 0xF3 },
        { Key::F4, 0xF4 },
        { Key::F5, 0xF5 },
        { Key::F6, 0xF6 },
        { Key::F7, 0xF7 },
        { Key::F8, 0xF8 },
        { Key::F9, 0xF9 },
        { Key::F10, 0xFA },
        { Key::F11, 0xFB },
        { Key::F12, 0xFC },
    };
    for (const auto& key : keyTable) {
        if (kbd.KeyPressed(key.key)) {
            // special case: shift-backspace clears screen shift-escape is STP
            if (kbd.KeyPressed(Key::LeftShift) && (key.key == Key::BackSpace)) {
                ascii = 0x0C;
            }
            else if (kbd.KeyPressed(Key::LeftShift) && (key.key == Key::Escape)) {
                ascii = 0x13;
            }
            else {
                ascii = key.ascii;
            }
            break;
        }
    }
    this->emu.put_key(ascii);
}

} // namespace Oryol
