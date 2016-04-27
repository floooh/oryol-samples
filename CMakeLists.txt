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

fips_setup(PROJECT oryol-samples)

# helper macro to add a sample to the webpage (called from the sample cmake files)
file(REMOVE ${FIPS_DEPLOY_DIR}/oryol-samples-webpage/websamples.yml)
macro(oryol_add_web_sample name desc type image src)
    file(APPEND ${FIPS_DEPLOY_DIR}/oryol-samples-webpage/websamples.yml "- name: ${name}\n  desc: ${desc}\n  type: ${type}\n  image: ${CMAKE_CURRENT_LIST_DIR}/${image}\n  src: ${src}\n")
endmacro()

fips_add_subdirectory(src)
fips_finish()


