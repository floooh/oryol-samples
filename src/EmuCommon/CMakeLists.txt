fips_begin_lib(EmuCommon)
    fips_files(
        KC85Emu.cc KC85Emu.h 
    )
    oryol_shader(emu_shaders.shd)
    fips_deps(yakc yakc_oryol)
fips_end_lib()

