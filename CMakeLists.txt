cmake_minimum_required(VERSION 3.10.2)

project(QuickBlast VERSION 1.0 DESCRIPTION "Maya Command for simultaneous Playblast across viewports.")

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
# FindMaya.cmake
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(src)
