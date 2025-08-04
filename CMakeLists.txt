cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# set up project
project(reloaded2ps3
        DESCRIPTION "Convert the PC version of Portal Reloaded to a playable PS3 game."
        VERSION 0.1.0
        HOMEPAGE_URL "https://github.com/craftablescience/reloaded2ps3")
set(PROJECT_NAME_PRETTY "Reloaded2PS3")
set(PROJECT_ORGANIZATION_NAME "craftablescience" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(CS_All)

# initial VS integration
cs_setup_vs_defaults(STARTUP_PROJECT "${PROJECT_NAME}")

# set common compile options
option(R2PS3_USE_LTO "Compile ${PROJECT_NAME_PRETTY} with LTO/IPO" OFF)
cs_configure_defaults(R2PS3_USE_LTO)

# external libraries
cs_include_directory(ext)

# reloaded2ps3
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h")
add_executable(${PROJECT_NAME}
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/R2PS3.cpp")
cs_configure_target(${PROJECT_NAME} LOGO)
target_link_libraries(${PROJECT_NAME} PRIVATE argparse::argparse sourcepp::bsppp sourcepp::steampp sourcepp::vpkpp sourcepp::vtfpp)

# copy the license
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE" COPYONLY)
