fips_begin_app(KC85-3 windowed)
    fips_vs_warning_level(3)
    fips_files(Main.cc 
        Emu.cc Emu.h
        SceneRenderer.cc SceneRenderer.h
        RayCheck.cc RayCheck.h
        sokol_audio.h
    )
    oryol_shader(shaders.shd)
    fips_generate(FROM voxels.py OUT_OF_SOURCE)
    fips_dir(emu)
    fips_files(kc85.h z80.h _z80_decoder.h z80ctc.h z80pio.h mem.h kbd.h beeper.h clk.h kc85-roms.h)
    fips_deps(IO HttpFS Gfx Assets Input Dbg Common)
    oryol_add_web_sample(KC85-3 "Test" "emscripten" KC85-3.jpg "KC85-3/Main.cc")
    if (FIPS_OSX)
        fips_frameworks_osx(AudioToolbox)
    elseif (FIPS_LINUX)
        fips_libs(asound)
    endif()
fips_end_app()
