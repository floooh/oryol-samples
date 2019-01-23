//------------------------------------------------------------------------------
//  Emu.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#define CHIPS_IMPL
#include "Emu.h"
#define SOKOL_IMPL
#include "sokol_audio.h"
#include "Input/Input.h"
#include "IO/IO.h"
#include "Assets/Gfx/ShapeBuilder.h"
#include "shaders.h"
#include "emu/kc85-roms.h"
#include <cctype>

namespace Oryol {

//------------------------------------------------------------------------------
//  audio-streaming callback to forward audio samples from the emulator
//  to sokol-audio
//
static void push_audio(const float* samples, int num_samples, void* user_data) {
    saudio_push(samples, num_samples);
}

//------------------------------------------------------------------------------
// a callback to patch some known problems in game snapshot files
//
static void patch_snapshots(const char* snapshot_name, void* user_data) {
    Emu* self = (Emu*) user_data;
    if (strcmp(snapshot_name, "JUNGLE     ") == 0) {
        /* patch start level 1 into memory */
        mem_wr(&self->kc85.mem, 0x36b7, 1);
        mem_wr(&self->kc85.mem, 0x3697, 1);
        for (int i = 0; i < 5; i++) {
            mem_wr(&self->kc85.mem, 0x1770 + i, mem_rd(&self->kc85.mem, 0x36b6 + i));
        }
    }
    else if (strcmp(snapshot_name, "DIGGER  COM\x01") == 0) {
        mem_wr16(&self->kc85.mem, 0x09AA, 0x0160);    /* time for delay-loop 0160 instead of 0260 */
        mem_wr(&self->kc85.mem, 0x3d3a, 0xB5);        /* OR L instead of OR (HL) */
    }
    else if (strcmp(snapshot_name, "DIGGERJ") == 0) {
        mem_wr16(&self->kc85.mem, 0x09AA, 0x0260);
        mem_wr(&self->kc85.mem, 0x3d3a, 0xB5);       /* OR L instead of OR (HL) */
    }
}
//------------------------------------------------------------------------------
void Emu::Setup(const GfxSetup& gfxSetup) {

    // setup the KC85 emulator
    this->TogglePower();

    // setup sokol-audio
    saudio_desc audio_desc = { };
    saudio_setup(&audio_desc);

    // create a mesh, pipeline and texture for rendering the emulator framebuffer
    ShapeBuilder shapeBuilder;
    shapeBuilder.Layout
        .Add(VertexAttr::Position, VertexFormat::Float3)
        .Add(VertexAttr::TexCoord0, VertexFormat::Float2);
    shapeBuilder.Plane(1.0f, 1.0f, 1);
    this->drawState.Mesh[0] = Gfx::CreateResource(shapeBuilder.Build());

    Id shd = Gfx::CreateResource(EmuShader::Setup());
    auto pip = PipelineSetup::FromLayoutAndShader(shapeBuilder.Layout, shd);
    pip.DepthStencilState.DepthWriteEnabled = true;
    pip.DepthStencilState.DepthCmpFunc = CompareFunc::LessEqual;
    pip.RasterizerState.SampleCount = gfxSetup.SampleCount;
    pip.BlendState.ColorFormat = gfxSetup.ColorFormat;
    pip.BlendState.DepthFormat = gfxSetup.DepthFormat;
    this->drawState.Pipeline = Gfx::CreateResource(pip);

    const int w = kc85_std_display_width();
    const int h = kc85_std_display_height();
    auto texSetup = TextureSetup::Empty2D(w, h, 1, PixelFormat::RGBA8, Usage::Stream);
    texSetup.Sampler.MinFilter = TextureFilterMode::Linear;
    texSetup.Sampler.MagFilter = TextureFilterMode::Linear;
    texSetup.Sampler.WrapU = TextureWrapMode::ClampToEdge;
    texSetup.Sampler.WrapV = TextureWrapMode::ClampToEdge;
    this->drawState.FSTexture[EmuShader::irm] = Gfx::CreateResource(texSetup);

    // keyboard input callback
    this->inputCallbackId = Input::SubscribeEvents([this](const InputEvent& e) {
        if (e.Type == InputEvent::WChar) {
            int c = (int) e.WCharCode;
            if ((c > 0x20) && (c < 0x7f)) {
                if (std::islower(c)) {
                    c = std::toupper(c);
                }
                else if (std::isupper(c)) {
                    c = std::tolower(c);
                }
                kc85_key_down(&this->kc85, c);
                kc85_key_up(&this->kc85, c);
            }
        }
        else if ((e.Type == InputEvent::KeyUp) || (e.Type == InputEvent::KeyDown)) {
            int c = 0;
            bool shift = Input::KeyPressed(Key::LeftShift);
            switch (e.KeyCode) {
                case Key::Space:        c = shift ? 0x5B : 0x20; break;
                case Key::Enter:        c = 0x0D; break;
                case Key::Right:        c = 0x09; break;
                case Key::Left:         c = 0x08; break;
                case Key::Down:         c = 0x0A; break;
                case Key::Up:           c = 0x0B; break;
                case Key::Home:         c = 0x10; break;
                case Key::Insert:       c = shift ? 0x0C : 0x1A; break;
                case Key::BackSpace:    c = shift ? 0x0C : 0x01; break;
                case Key::Escape:       c = shift ? 0x13 : 0x03; break;
                case Key::F1:           c = 0xF1; break;
                case Key::F2:           c = 0xF2; break;
                case Key::F3:           c = 0xF3; break;
                case Key::F4:           c = 0xF4; break;
                case Key::F5:           c = 0xF5; break;
                case Key::F6:           c = 0xF6; break;
                case Key::F7:           c = 0xF7; break;
                case Key::F8:           c = 0xF8; break;
                case Key::F9:           c = 0xF9; break;
                case Key::F10:          c = 0xFA; break;
                case Key::F11:          c = 0xFB; break;
                case Key::F12:          c = 0xFC; break;
                default:                c = 0; break;
            }
            if (c) {
                if (e.Type == InputEvent::KeyDown) {
                    kc85_key_down(&this->kc85, c);
                }
                else {
                    kc85_key_up(&this->kc85, c);
                }
            }
        }
    });
}

//------------------------------------------------------------------------------
void Emu::Discard() {
    saudio_shutdown();
    Input::UnsubscribeEvents(this->inputCallbackId);
}

//------------------------------------------------------------------------------
void Emu::Tick(Duration frameTime) {
    this->frameIndex++;
    if (this->switchedOn) {
        uint32_t microSecs = (uint32_t)frameTime.AsMicroSeconds();
        // clamp to 30Hz, in case we've been switched to the background
        if (microSecs > 33333) {
            microSecs = 33333;
        }
        kc85_exec(&this->kc85, microSecs);
    }
    // start a game?
    if (this->startGamePath && (this->startGameFrameIndex == this->frameIndex)) {
        IO::Load(this->startGamePath, [this](IO::LoadResult ioResult) {
            kc85_quickload(&this->kc85, ioResult.Data.Data(), ioResult.Data.Size());
        });
        this->startGamePath = nullptr;
    }
}

//------------------------------------------------------------------------------
void Emu::Render(const glm::mat4& mvp) {
    if (this->switchedOn) {
        ImageDataAttrs updAttrs;
        updAttrs.NumFaces = 1;
        updAttrs.NumMipMaps = 1;
        updAttrs.Sizes[0][0] = sizeof(this->pixelBuffer);
        Gfx::UpdateTexture(this->drawState.FSTexture[0], this->pixelBuffer, updAttrs);
        EmuShader::vsParams vsParams;
        vsParams.mvp = mvp;
        Gfx::ApplyDrawState(this->drawState);
        Gfx::ApplyUniformBlock(vsParams);
        Gfx::Draw();
    }
}

//------------------------------------------------------------------------------
void Emu::TogglePower() {
    this->switchedOn = !this->switchedOn;
    if (this->switchedOn) {
        kc85_desc_t kc85_desc = { };
        kc85_desc.type = KC85_TYPE_3;
        kc85_desc.pixel_buffer = this->pixelBuffer;
        kc85_desc.pixel_buffer_size = sizeof(this->pixelBuffer);
        kc85_desc.audio_cb = push_audio;
        kc85_desc.audio_sample_rate = saudio_sample_rate();
        kc85_desc.patch_cb = patch_snapshots;
        kc85_desc.rom_caos31 = dump_caos31;
        kc85_desc.rom_caos31_size = sizeof(dump_caos31);
        kc85_desc.rom_kcbasic = dump_basic_c0;
        kc85_desc.rom_kcbasic_size = sizeof(dump_basic_c0);
        kc85_desc.user_data = (void*) this;
        kc85_init(&this->kc85, &kc85_desc);
        kc85_insert_ram_module(&this->kc85, 0x08, KC85_MODULE_M022_16KBYTE);
    }
    else {
        kc85_discard(&this->kc85);
    }
}

//------------------------------------------------------------------------------
bool Emu::SwitchedOn() const {
    return this->switchedOn;
}

//------------------------------------------------------------------------------
void Emu::Reset() {
    if (this->switchedOn) {
        kc85_reset(&this->kc85);
    }
}

//------------------------------------------------------------------------------
void Emu::StartGame(const char* path) {
    if (this->switchedOn) {
        this->TogglePower();
    }
    this->TogglePower();
    this->startGameFrameIndex = this->frameIndex + 3 * 60;
    this->startGamePath = path;
}

} // namespace Oryol

