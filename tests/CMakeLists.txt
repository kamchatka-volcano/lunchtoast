cmake_minimum_required(VERSION 3.18)

project(test_lunchtoast)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckPIESupported)
check_pie_supported()

set(SRC
    test_sectionsreader.cpp
    ../src/sectionsreader.cpp
    )

add_executable(test_lunchtoast ${SRC})
add_test(NAME test_lunchtoast COMMAND test_lunchtoast)
target_compile_features(test_lunchtoast PUBLIC cxx_std_17)
set_target_properties(test_lunchtoast PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE OFF)
target_include_directories(test_lunchtoast PRIVATE ../src)
target_link_libraries(test_lunchtoast PRIVATE ${GTEST_BOTH_LIBRARIES} Threads::Threads sfun)
