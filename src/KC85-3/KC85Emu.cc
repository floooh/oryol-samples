//------------------------------------------------------------------------------
//  Emu/KC85Emu.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "KC85Emu.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Input/Input.h"
#include "emu_shaders.h"

using namespace YAKC;

namespace Oryol {

//------------------------------------------------------------------------------
void
KC85Emu::Setup(const GfxSetup& gfxSetup) {

    // initialize the emulator as KC85/3
    this->emu.kc85.roms.add(kc85_roms::caos31, dump_caos31, sizeof(dump_caos31));
    this->emu.kc85.roms.add(kc85_roms::basic_rom, dump_basic_c0, sizeof(dump_basic_c0));
    ext_funcs funcs;
    funcs.assertmsg_func = Log::AssertMsg;
    funcs.malloc_func = [] (size_t s) -> void* { return Memory::Alloc(int(s)); };
    funcs.free_func   = [] (void* p) { Memory::Free(p); };
    this->emu.init(funcs);
    this->draw.Setup(gfxSetup, 5);
    this->audio.Setup(this->emu.board.clck);
    this->keyboard.Setup(this->emu);
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
    this->keyboard.Discard();
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
        this->keyboard.HandleInput();
        int micro_secs = (int) frameTime.AsMicroSeconds();
        const uint64_t cpu_min_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/100;
        const uint64_t cpu_max_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/25;
        const uint64_t audio_cycle_count = this->audio.GetProcessedCycles();
        uint64_t min_cycle_count = 0;
        uint64_t max_cycle_count = 0;
        if (audio_cycle_count > 0) {
            const uint64_t cpu_min_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/100;
            const uint64_t cpu_max_ahead_cycles = (this->emu.board.clck.base_freq_khz*1000)/25;
            min_cycle_count = audio_cycle_count + cpu_min_ahead_cycles;
            max_cycle_count = audio_cycle_count + cpu_max_ahead_cycles;
        }
        this->emu.onframe(1, micro_secs, min_cycle_count, max_cycle_count);
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

} // namespace Oryol
