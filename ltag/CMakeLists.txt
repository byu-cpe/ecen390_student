# Do not change the order of these commands
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS ../components ./main/test)
set(COMPONENTS main)
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/main/test/m3t2)
    list(APPEND EXTRA_COMPONENT_DIRS ./main/test/m3t2)
    list(APPEND COMPONENTS m3t2)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ltag)
idf_build_set_property(COMPILE_DEFINITIONS "-DHW_TARGET_LTAG" APPEND)
# idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
