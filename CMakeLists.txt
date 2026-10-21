cmake_minimum_required(VERSION 3.21)

# The version lives in VERSION so the release notes, the package config and Doxygen read one value.
file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" xvfs_version)
string(STRIP "${xvfs_version}" xvfs_version)

project(xvfs
    VERSION ${xvfs_version}
    DESCRIPTION "Wildcard virtual file system paths and whole-file reads"
    HOMEPAGE_URL "https://github.com/Dyronix/xvfs"
    LANGUAGES CXX)

# Tests, installing and strict warnings are on only when xvfs is the top-level project, so a
# project that vendors it with add_subdirectory gets the library and nothing else.
option(XVFS_BUILD_TESTS "Build the xvfs tests and register them with CTest" ${PROJECT_IS_TOP_LEVEL})
option(XVFS_INSTALL "Generate the xvfs install and package export rules" ${PROJECT_IS_TOP_LEVEL})
option(XVFS_STRICT_WARNINGS "Compile xvfs with -Wall -Wextra -Wpedantic (/W4 on MSVC)" ${PROJECT_IS_TOP_LEVEL})
option(XVFS_WARNINGS_AS_ERRORS "Treat xvfs compiler warnings as errors" OFF)

add_library(xvfs STATIC)
add_library(xvfs::xvfs ALIAS xvfs)

target_sources(xvfs
    PRIVATE
        src/context.cpp
        src/directory_info.cpp
        src/error.cpp
        src/file_info.cpp
        src/fileio.cpp
        src/input_file.cpp
        src/log_message.cpp
        src/log_message.h
        src/native_path.cpp
        src/path.cpp
        src/path_text.h
)

set(xvfs_public_headers
    include/xvfs/context.h
    include/xvfs/directory_info.h
    include/xvfs/error.h
    include/xvfs/file_info.h
    include/xvfs/fileio.h
    include/xvfs/input_file.h
    include/xvfs/log.h
    include/xvfs/native_path.h
    include/xvfs/path.h
    include/xvfs/result.h
)
target_sources(xvfs PRIVATE ${xvfs_public_headers})

include(GNUInstallDirs)

target_include_directories(xvfs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(xvfs PUBLIC cxx_std_20)

set_target_properties(xvfs PROPERTIES
    CXX_EXTENSIONS OFF
    FOLDER "ThirdParty/xvfs"
    VERSION ${PROJECT_VERSION}
)

# Applied to the library and the tests, never to the targets of a project that vendors xvfs.
add_library(xvfs_warnings INTERFACE)
if(XVFS_STRICT_WARNINGS)
    target_compile_options(xvfs_warnings INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)
endif()
if(XVFS_WARNINGS_AS_ERRORS)
    target_compile_options(xvfs_warnings INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>)
endif()
target_link_libraries(xvfs PRIVATE $<BUILD_INTERFACE:xvfs_warnings>)

if(XVFS_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS xvfs
        EXPORT xvfs-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(DIRECTORY include/xvfs DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES LICENSE README.md CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/xvfs)

    set(xvfs_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/xvfs)
    install(EXPORT xvfs-targets
        NAMESPACE xvfs::
        DESTINATION ${xvfs_config_dir})
    configure_package_config_file(cmake/xvfs-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/xvfs-config.cmake
        INSTALL_DESTINATION ${xvfs_config_dir})
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/xvfs-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/xvfs-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/xvfs-config-version.cmake
        DESTINATION ${xvfs_config_dir})
endif()

if(XVFS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
