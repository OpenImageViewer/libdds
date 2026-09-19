cmake_minimum_required(VERSION 3.20)
project(libdds VERSION 1.0.0 LANGUAGES CXX)

add_library(libdds STATIC
    DirectXTex/BC.cpp
    DirectXTex/BC4BC5.cpp
    DirectXTex/BC6HBC7.cpp
    DirectXTex/DirectXTexCompress.cpp
    DirectXTex/DirectXTexConvert.cpp
    DirectXTex/DirectXTexDDS.cpp
    DirectXTex/DirectXTexHDR.cpp
    DirectXTex/DirectXTexImage.cpp
    DirectXTex/DirectXTexMipmaps.cpp
    DirectXTex/DirectXTexMisc.cpp
    DirectXTex/DirectXTexNormalMaps.cpp
    DirectXTex/DirectXTexPMAlpha.cpp
    DirectXTex/DirectXTexResize.cpp
    DirectXTex/DirectXTexTGA.cpp
    DirectXTex/DirectXTexUtil.cpp
)
# Keep the scanline implementation visible to the block decoder without changing
# either upstream source file. Other files keep their original translation units.
set_source_files_properties(DirectXTex/DirectXTexCompress.cpp DirectXTex/DirectXTexConvert.cpp
    PROPERTIES UNITY_GROUP block_codec)
set_target_properties(libdds PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE GROUP)
add_library(libdds::libdds ALIAS libdds)
target_compile_features(libdds PUBLIC cxx_std_17)
set_target_properties(libdds PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(libdds SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex
    ${CMAKE_CURRENT_SOURCE_DIR}/External/DirectXMath/Inc)
if(NOT WIN32)
    target_include_directories(libdds SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/External/SAL)
endif()
if(WIN32)
    target_compile_definitions(libdds PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
    target_compile_definitions(libdds PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(libdds PRIVATE -Wno-unknown-pragmas)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(LIBDDS_TESTS_DEFAULT ON)
else()
    set(LIBDDS_TESTS_DEFAULT OFF)
endif()
option(LIBDDS_BUILD_TESTS "Build standalone libdds tests" ${LIBDDS_TESTS_DEFAULT})
if(LIBDDS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
