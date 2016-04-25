#
# project: oryol-samples
#
cmake_minimum_required(VERSION 2.8)

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

# include_directories(src)

# this hack is required for turbobadger since we need to make
# sure that our overriden config.h is included
include_directories(../oryol-tbui/src/TBUI/tb)

fips_setup()
fips_project(oryol-samples)
fips_add_subdirectory(src)
fips_finish()


