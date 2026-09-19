add_executable(libdds_tests Tests.cpp)
target_link_libraries(libdds_tests PRIVATE libdds::libdds)
add_test(NAME libdds.cpu COMMAND libdds_tests)
set_tests_properties(libdds.cpu PROPERTIES RUN_SERIAL TRUE
    ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1;ASAN_OPTIONS=detect_leaks=1")

# Optional differential check. The upstream checkout is read-only and is compiled
# into a separate process to avoid collisions with its preserved DirectX symbols.
set(LIBDDS_REFERENCE_SOURCE_DIR "" CACHE PATH "Pinned upstream DirectXTex checkout for Windows differential tests")
if(LIBDDS_REFERENCE_SOURCE_DIR AND WIN32)
    get_target_property(extracted_sources libdds SOURCES)
    set(reference_sources)
    foreach(source IN LISTS extracted_sources)
        list(APPEND reference_sources "${LIBDDS_REFERENCE_SOURCE_DIR}/${source}")
    endforeach()
    add_library(libdds_reference STATIC ${reference_sources})
    target_compile_features(libdds_reference PUBLIC cxx_std_17)
    target_include_directories(libdds_reference SYSTEM PUBLIC
        ${LIBDDS_REFERENCE_SOURCE_DIR}/DirectXTex
        ${CMAKE_CURRENT_SOURCE_DIR}/../External/DirectXMath/Inc)
    target_compile_definitions(libdds_reference PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(libdds_reference PRIVATE ole32 windowscodecs)
    add_executable(libdds_reference_tests Tests.cpp)
    target_compile_definitions(libdds_reference_tests PRIVATE LIBDDS_REFERENCE)
    target_link_libraries(libdds_reference_tests PRIVATE libdds_reference)
    add_test(NAME libdds.upstream_parity COMMAND ${CMAKE_COMMAND}
        -DEXTRACTED=$<TARGET_FILE:libdds_tests>
        -DREFERENCE=$<TARGET_FILE:libdds_reference_tests>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/Compare.cmake)
    set_tests_properties(libdds.upstream_parity PROPERTIES RUN_SERIAL TRUE)
endif()
