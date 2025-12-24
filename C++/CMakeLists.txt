cmake_minimum_required(VERSION 3.10)
project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(${TARGET_NAME}
        ${MAIN_FILE}
        Sources/MurmurHash3.cpp
        Sources/bitmap.cpp
        Sources/PSS_BM.cpp
        Sources/PSS_HLL.cpp
        Sources/KTSketch.cpp
        Sources/HLLsampler.cpp
        Sources/GMF.cpp
        Sources/SpreadSketch.cpp
        Sources/AROMA.cpp
        Sources/PSS_SBM.cpp
        Sources/FreeRS.cpp
        Sources/PSSv2_BM.cpp
        Sources/RGS.cpp
        )