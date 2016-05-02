fips_begin_app(StbVoxelDemo windowed)
    fips_files(
        Main.cc
        Volume.h Config.h
        VoxelGenerator.h VoxelGenerator.cc
        GeomPool.h GeomPool.cc
        GeomMesher.h GeomMesher.cc
        VisNode.h VisBounds.h
        VisTree.h VisTree.cc
        Camera.h Camera.cc
        stb_voxel_render.h)
    oryol_shader(shaders.shd)
    fips_deps(Gfx Input Dbg)
    oryol_add_web_sample(StbVoxelDemo "Voxel Demo using stb_voxel_render.h" "emscripten,pnacl" StbVoxelDemo.jpg "StbVoxelDemo/")
fips_end_app()

# silence some stb_voxel_render warnings
if (FIPS_CLANG OR FIPS_GCC)
    target_compile_options(StbVoxelDemo PRIVATE -Wno-missing-field-initializers -Wno-unused-variable)
endif()
if (FIPS_GCC)
    target_compile_options(StbVoxelDemo PRIVATE -Wno-extra)
endif()
