//------------------------------------------------------------------------------
//  Emu/KC85Emu.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "KC85Emu.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "Input/Input.h"
#include "yakc/roms/rom_dumps.h"
#include "emu_shaders.h"

using namespace YAKC;

namespace Oryol {

//------------------------------------------------------------------------------
void
KC85Emu::Setup(const GfxSetup& gfxSetup, YAKC::system m, os_rom os) {

    this->model = m;
    this->rom = os;

    // initialize the emulator
    if (this->model == YAKC::system::kc85_3) {
        this->emu.roms.add(rom_images::caos31, dump_caos31, sizeof(dump_caos31));
        this->emu.roms.add(rom_images::kc85_basic_rom, dump_basic_c0, sizeof(dump_basic_c0));
    }

    ext_funcs sys_funcs;
    sys_funcs.assertmsg_func = Log::AssertMsg;
    sys_funcs.malloc_func = [] (size_t s) -> void* { return Memory::Alloc(int(s)); };
    sys_funcs.free_func   = [] (void* p) { Memory::Free(p); };
    this->emu.init(sys_funcs);
    this->draw.Setup(gfxSetup, 5, 5);
    this->audio.Setup(&this->emu);
    this->keyboard.Setup(this->emu);
    this->fileLoader.Setup(this->emu);

    // register modules
    if (int(this->model) & int(YAKC::system::any_kc85)) {
        this->emu.kc85.exp.register_none_module("NO MODULE", "Click to insert module!");
        this->emu.kc85.exp.register_ram_module(kc85_exp::m022_16kbyte, 0xC0, 0x4000, "nohelp");
    }

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
                    this->fileLoader.LoadAndStart(item);
                    break;
                }
            }
        }

        // handle the normal emulator per-frame update
        this->keyboard.HandleInput();
        int micro_secs = (int) frameTime.AsMicroSeconds();
        const uint64_t audio_cycle_count = this->audio.GetProcessedCycles();
        this->emu.step(micro_secs, audio_cycle_count);
        this->audio.Update();
    }
    else {
        // switch KC on once after a little while
        if (this->frameIndex == this->switchOnDelayFrames) {
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
        this->emu.poweron(this->model, this->rom);
        if (int(this->model) & int(YAKC::system::any_kc85)) {
            if (!this->emu.kc85.exp.slot_occupied(0x08)) {
                this->emu.kc85.exp.insert_module(0x08, kc85_exp::m022_16kbyte);
            }
            if (!this->emu.kc85.exp.slot_occupied(0x0C)) {
                this->emu.kc85.exp.insert_module(0x0C, kc85_exp::none);
            }
        }
    }
}

//------------------------------------------------------------------------------
bool
KC85Emu::SwitchedOn() const {
    return this->emu.switchedon();
}

//------------------------------------------------------------------------------
void
KC85Emu::Reset() {
    if (this->SwitchedOn()) {
        this->emu.reset();
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
KC85Emu::Render(const glm::mat4& mvp, bool onlyUpdateTexture) {

    if (this->SwitchedOn()) {
        // get the emulator's framebuffer and fb size
        int width = 0, height = 0;
        const void* fb = this->emu.framebuffer(width, height);
        if (fb) {
            this->draw.validateTexture(width, height);
            this->draw.texUpdateAttrs.Sizes[0][0] = width*height*4;
            if (this->draw.texture.IsValid()) {
                Gfx::UpdateTexture(this->draw.texture, fb, this->draw.texUpdateAttrs);
                if (!onlyUpdateTexture) {
                    this->drawState.FSTexture[KCTextures::IRM] = this->draw.texture;
                    KCShader::KCVSParams vsParams;
                    vsParams.ModelViewProjection = mvp;
                    Gfx::ApplyDrawState(this->drawState);
                    Gfx::ApplyUniformBlock(vsParams);
                    Gfx::Draw();
                }
            }
        }
    }
}

} // namespace Oryol
