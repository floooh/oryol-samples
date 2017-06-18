fips_begin_app(OrbViewer windowed)
    fips_vs_warning_level(3)
    fips_files(
        Main.cc
        OrbFile.h OrbFile.cc
        Wireframe.h Wireframe.cc
    )
    oryol_shader(shaders.glsl)
    fips_deps(Gfx HttpFS Input IMUI Common Anim)
    oryol_add_web_sample(OrbViewer "Load and render .orb files" "emscripten" none "OrbViewer/Main.cc")
fips_end_app()
