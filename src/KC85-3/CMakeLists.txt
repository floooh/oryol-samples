fips_begin_app(KC85-3 windowed)
    fips_vs_warning_level(3)
    fips_files(Main.cc 
        SceneRenderer.cc SceneRenderer.h
        RayCheck.cc RayCheck.h
    )
    oryol_shader(shaders.shd)
    fips_generate(FROM voxels.py OUT_OF_SOURCE)
    fips_deps(Gfx Input Dbg Common EmuCommon)
    oryol_add_web_sample(KC85-3 "Test" "emscripten,pnacl" KC85-3.jpg "KC85-3/Main.cc")
fips_end_app()
