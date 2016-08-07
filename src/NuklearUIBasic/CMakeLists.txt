fips_begin_app(NuklearUIBasic windowed)
    if (FIPS_MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()
    fips_vs_warning_level(3)
    fips_files(
        NuklearUIBasic.cc
    )
    fips_deps(Gfx Dbg NKUI)
    oryol_add_web_sample(NuklearUIBasic "Nuklear UI basic demo" "emscripten,pnacl" none "NuklearUIBasic/NuklearUIBasic.cc")
fips_end_app()
