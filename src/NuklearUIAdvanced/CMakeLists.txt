fips_begin_app(NuklearUIAdvanced windowed)
    if (FIPS_MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()
    fips_vs_warning_level(3)
    fips_files(
        NuklearUIAdvanced.cc
        stb_image.h
    )
    fips_deps(IO HTTP Gfx Dbg NKUI)
    oryol_add_web_sample(NuklearUIAdvanced "Nuklear UI advanced demo" "emscripten" "NuklearUIAdvanced.jpg" "NuklearUIAdvanced/NuklearUIAdvanced.cc")
fips_end_app()
