find_package(Threads REQUIRED)

add_executable(xvfs_tests
    main.cpp
    test_check.cpp
    test_check.h
    context_tests.cpp
    fileio_tests.cpp
    path_tests.cpp
    threading_tests.cpp
    utf8_tests.cpp
)
target_link_libraries(xvfs_tests PRIVATE xvfs::xvfs Threads::Threads xvfs_warnings)
set_target_properties(xvfs_tests PROPERTIES CXX_EXTENSIONS OFF FOLDER "ThirdParty/xvfs")

# One CTest entry per group, so a failure names the area. The runner takes the group name.
foreach(group context fileio path threading utf8)
    add_test(NAME xvfs.${group} COMMAND xvfs_tests ${group})
endforeach()
