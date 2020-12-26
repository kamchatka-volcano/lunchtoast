cmake_minimum_required(VERSION 2.6)
 
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS}, ..)

set(SRC
    test_sectionsreader.cpp
    ../src/sectionsreader.cpp    
    ../src/string_utils.cpp
    )

add_executable(test_lunchtoast ${SRC})
add_test(NAME test_lunchtoast COMMAND test_lunchtoast)
target_include_directories(test_lunchtoast PRIVATE ../src)
target_link_libraries(test_lunchtoast gtest gmock gtest_main pthread)
