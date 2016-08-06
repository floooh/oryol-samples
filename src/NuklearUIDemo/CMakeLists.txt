fips_begin_app(NuklearUIDemo windowed)
    fips_vs_warning_level(3)
    fips_files(
        NuklearUIDemo.cc
    )
    fips_deps(Gfx Dbg NKUI)
    oryol_add_web_sample(NuklearUIDemo "Nuklear UI integration" "emscripten,pnacl" none "NuklearUIDemo/NuklearUIDemo.cc")
fips_end_app()
