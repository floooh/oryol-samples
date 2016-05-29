fips_begin_app(KC85-3 windowed)
    fips_vs_warning_level(3)
    fips_files(Main.cc 
        KC85Emu.cc KC85Emu.h 
        SceneRenderer.cc SceneRenderer.h
        RayCheck.cc RayCheck.h
    )
    fips_generate(FROM voxels.py OUT_OF_SOURCE)
    oryol_shader(emu_shaders.shd)
    fips_deps(Gfx Input Dbg Common yakc)
    oryol_add_web_sample(KC85-3 "Test" "emscripten" none "KC85-3/Main.cc")
fips_end_app()
