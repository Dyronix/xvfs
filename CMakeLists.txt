cmake_minimum_required(VERSION 3.0...3.20 FATAL_ERROR)

project(vfs)

set(VFS_INCLUDE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include)
set(VFS_SOURCE_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/src)

add_library(vfs STATIC)

# Sources
target_sources(vfs PRIVATE
    ${VFS_SOURCE_DIRECTORY}/fileio.cpp
    ${VFS_SOURCE_DIRECTORY}/vfs.cpp
    ${VFS_SOURCE_DIRECTORY}/file_info.cpp
)

target_sources(vfs PUBLIC
    #string
    ${VFS_INCLUDE_DIRECTORY}/string/string_ops.h

    ${VFS_INCLUDE_DIRECTORY}/fileio.h
    ${VFS_INCLUDE_DIRECTORY}/vfs.h
    ${VFS_INCLUDE_DIRECTORY}/file_info.h
)

target_include_directories(vfs PUBLIC ${VFS_INCLUDE_DIRECTORY})

set_target_properties(vfs PROPERTIES
                      FOLDER "ThirdParty/VFS")