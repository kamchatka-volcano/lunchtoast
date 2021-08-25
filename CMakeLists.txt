cmake_minimum_required(VERSION 3.18)
project(lunchtoast VERSION 1.0.0 DESCRIPTION "lunchtoast - a command line functional testing automation tool")

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

find_package(Boost 1.67 REQUIRED COMPONENTS system)

FetchContent_Declare(
    cmdlime
    GIT_REPOSITORY https://github.com/kamchatka-volcano/cmdlime.git
    GIT_TAG        e30e7afd9870051757f374a00c1c2a02fb7f22c1
    GIT_SHALLOW    ON
    GIT_PROGRESS   TRUE
)
FetchContent_Declare(
    sfun
    GIT_REPOSITORY https://github.com/kamchatka-volcano/sfun.git
    GIT_TAG        fe5b60f3fd7a545515ca2e50a406c6bc2c924ac7
    GIT_SHALLOW    ON
    GIT_PROGRESS   TRUE
)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        eb3220622e73a4889eee355ffa37972b3cac3df5
    GIT_SHALLOW    ON
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(cmdlime sfun spdlog)

set(SRC
    src/cleanupwhitelistgenerator.cpp
    src/comparefilecontent.cpp
    src/comparefiles.cpp
    src/filenamereader.cpp
    src/launchprocess.cpp
    src/main.cpp
    src/sectionsreader.cpp
    src/testactionresult.cpp
    src/test.cpp
    src/testlauncher.cpp
    src/testreporter.cpp
    src/testresult.cpp
    src/utils.cpp
    src/writefile.cpp
    )
    
add_executable(lunchtoast ${SRC})
target_compile_features(lunchtoast PUBLIC cxx_std_17)
set_target_properties(lunchtoast PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(lunchtoast PRIVATE cmdlime boost_system boost_filesystem spdlog::spdlog sfun)

option(ENABLE_TESTS "Enable tests" OFF)
if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS lunchtoast RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
