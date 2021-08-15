cmake_minimum_required(VERSION 3.18)

project(lunchtoast VERSION 1.0.0 DESCRIPTION "lunchtoast - a command line functional testing automation tool")

find_package(Boost 1.67 REQUIRED COMPONENTS system)

set(SRC
    src/cleanupwhitelistgenerator.cpp
    src/comparefilecontent.cpp
    src/comparefiles.cpp
    src/filenamereader.cpp
    src/launchprocess.cpp
    src/main.cpp
    src/sectionsreader.cpp
    src/string_utils.cpp
    src/testactionresult.cpp
    src/test.cpp
    src/testlauncher.cpp
    src/testreporter.cpp
    src/testresult.cpp
    src/utils.cpp
    src/writefile.cpp
    )

add_subdirectory(lib/spdlog)
add_subdirectory(lib/cmdlime)
    
add_executable(lunchtoast ${SRC})
target_compile_features(lunchtoast PUBLIC cxx_std_17)
set_target_properties(lunchtoast PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(lunchtoast PRIVATE lib/spdlog/include)
target_link_libraries(lunchtoast PRIVATE cmdlime boost_system boost_filesystem spdlog::spdlog)

option(ENABLE_TESTS "Enable tests" OFF)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_custom_command(
        TARGET lunchtoast POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                lunchtoast
                ${CMAKE_SOURCE_DIR}/functional_tests/bin/lunchtoast)
endif()

include(GNUInstallDirs)
install(TARGETS lunchtoast RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
