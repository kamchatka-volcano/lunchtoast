cmake_minimum_required(VERSION 3.13)

project(lunchtoast VERSION 1.0.0 DESCRIPTION "lunchtoast - a command line functional testing automation tool")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
 
find_package(Boost 1.67 REQUIRED COMPONENTS
             program_options filesystem system)

set(SRC
    src/cleanupwhitelistgenerator.cpp
    src/comparefilecontent.cpp
    src/comparefiles.cpp
    src/filenamereader.cpp
    src/launchprocess.cpp
    src/main.cpp
    src/sectionsreader.cpp
    src/string_utils.cpp
    src/testaction.cpp
    src/testactionresult.cpp
    src/test.cpp
    src/testlauncher.cpp
    src/testreporter.cpp
    src/testresult.cpp
    src/utils.cpp
    src/writefile.cpp
    )

add_subdirectory(lib/spdlog)
    
add_executable(lunchtoast ${SRC})
target_include_directories(lunchtoast PRIVATE lib/spdlog/include)
target_link_libraries(lunchtoast boost_system boost_filesystem boost_program_options spdlog::spdlog)

option(ENABLE_TESTS "Enable tests" OFF)
if (${ENABLE_TESTS})
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS lunchtoast RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
