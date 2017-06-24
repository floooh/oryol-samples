fips_begin_lib(Common)
    fips_files(
        stb_voxel_render.h
        CameraHelper.h CameraHelper.cc
        OrbModel.h
        OrbFile.h OrbFile.cc
        OrbLoader.h OrbLoader.cc
        Wireframe.h Wireframe.cc
    )
    oryol_shader(wireframe_shaders.glsl)
fips_end_lib(Common)
if (FIPS_CLANG)
    target_compile_options(Common PUBLIC -Wno-missing-field-initializers -Wno-shift-negative-value -Wno-unused-variable)
endif()

