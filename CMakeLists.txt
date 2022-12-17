cmake_minimum_required(VERSION 3.18)
project(lunchtoast VERSION 1.0.0 DESCRIPTION "lunchtoast - a command line functional testing automation tool")

find_package(Boost 1.67 REQUIRED COMPONENTS system)
include(GNUInstallDirs)
include(external/seal_lake)

SealLake_Import(sfun 2.4.0
        GIT_REPOSITORY https://github.com/kamchatka-volcano/sfun
        GIT_TAG v3
)
SealLake_Import(cmdlime 2.1.1
        GIT_REPOSITORY https://github.com/kamchatka-volcano/cmdlime
        GIT_TAG v2.1.1
)
SealLake_Import(fmt 9.1.0
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 9.1.0
)
set(SPDLOG_FMT_EXTERNAL ON)
SealLake_Import(spdlog 1.11.0
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG v1.11.0
)

SealLake_Import(gsl 4.0.0
        GIT_REPOSITORY https://github.com/microsoft/GSL.git
        GIT_TAG        v4.0.0
)
SealLake_Bundle(
        NAME range-v3
        SKIP_LOAD
        GIT_REPOSITORY https://github.com/ericniebler/range-v3
        GIT_TAG        0.12.0
)
#SealLake_Import(range-v3 0.12.0
#        GIT_REPOSITORY https://github.com/ericniebler/range-v3
#        GIT_TAG        0.12.0
#)

set(SRC
        src/testcontentsgenerator.cpp
    src/comparefilecontent.cpp
    src/comparefiles.cpp
    src/filenamereader.cpp
    src/launchprocess.cpp
    src/linestream.cpp
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

SealLake_Executable(
        SOURCES ${SRC}
        COMPILE_FEATURES cxx_std_17
        PROPERTIES
            CXX_EXTENSIONS OFF
        INCLUDES ${SEAL_LAKE_SOURCE_range-v3}/include
        LIBRARIES
            boost_system boost_filesystem spdlog::spdlog sfun::sfun cmdlime::cmdlime fmt::fmt Microsoft.GSL::GSL
)
SealLake_OptionalBuildSteps(tests)
