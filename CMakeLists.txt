cmake_minimum_required(VERSION 3.18)
project(lunchtoast VERSION 0.1.0 DESCRIPTION "lunchtoast - a command line functional testing automation tool")

find_package(Boost 1.67 REQUIRED COMPONENTS system filesystem)
include(GNUInstallDirs)
include(external/seal_lake)

SealLake_Import(sfun 2.4.0
        GIT_REPOSITORY https://github.com/kamchatka-volcano/sfun
        GIT_TAG v3
)
SealLake_Import(cmdlime 2.3.1
        GIT_REPOSITORY https://github.com/kamchatka-volcano/cmdlime
        GIT_TAG v2.3.1
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

SealLake_Import(GSL 4.0.0
        GIT_REPOSITORY https://github.com/microsoft/GSL.git
        GIT_TAG        v4.0.0
)

SealLake_Import(platform_folders 4.2.0
        GIT_REPOSITORY https://github.com/sago007/PlatformFolders.git
        GIT_TAG        4.2.0
)

set(SRC
    src/testcontentsgenerator.cpp
    src/compareoutput.cpp
    src/compareerroroutput.cpp
    src/comparefilecontent.cpp
    src/comparefiles.cpp
    src/compareexitcode.cpp
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
        LIBRARIES
            Boost::boost
            Boost::filesystem
            spdlog::spdlog
            sfun::sfun
            cmdlime::cmdlime
            fmt::fmt
            Microsoft.GSL::GSL
            sago::platform_folders
)
SealLake_OptionalBuildSteps(tests)
