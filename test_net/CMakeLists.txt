# Do not change the order of these commands
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS ../components)
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_net)
idf_build_set_property(COMPILE_DEFINITIONS "-DHW_TARGET_LTAG" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
