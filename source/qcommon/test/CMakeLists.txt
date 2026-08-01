project(qcommon_test)

#
# Test and fuzz harnesses for the network parsers.
#
# msg.c, snap_read.c and net_chan.c only reach the rest of the engine through
# Com_Error / Com_Printf / a handful of NET_ helpers, so they can be linked into a
# standalone binary against net_test_stubs.c instead of dragging in the client or
# server. See net_test_stubs.h for how parser rejection is trapped.
#

set(QCOMMON_TEST_STUBS
    "${CMAKE_CURRENT_SOURCE_DIR}/net_test_stubs.c"
)

# the parsers under test, plus the gameshared helpers they call
set(QCOMMON_TEST_PARSER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../msg.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../snap_read.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../gameshared/q_math.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../gameshared/q_shared.c"
)

set(QCOMMON_TEST_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${MINIZ_INCLUDE_DIR}"
    # snap_read.c includes tracy/TracyC.h; the zone macros compile to nothing
    # unless TRACY_ENABLE is set, so only the header path is needed
    "${CMAKE_CURRENT_SOURCE_DIR}/../../extern/tracy/public"
)

#
# qcommon.h defines a file-static `default_fs_imports_s` table holding the
# address of every FS_* entry point. Nothing here calls it, but at -O0 the
# compiler still emits it, and its relocations then demand the whole filesystem
# layer at link time. Discarding unreferenced sections drops the table with
# them, so these targets link against msg.c / snap_read.c alone at any
# optimisation level.
#
function(qcommon_test_trim_unused target)
    target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
    if(APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    else()
        target_link_options(${target} PRIVATE -Wl,--gc-sections)
    endif()
endfunction()

#
# Regression corpus. Runs under the normal BUILD_UNIT_TEST flag alongside the
# qcore tests; build it with -DUSE_ASAN=ON to get the full signal.
#
if(BUILD_UNIT_TEST)
    add_executable(msg_corpus_test
        "${CMAKE_CURRENT_SOURCE_DIR}/msg_corpus_test.c"
        ${QCOMMON_TEST_PARSER_SOURCES}
        ${QCOMMON_TEST_STUBS}
    )
    target_include_directories(msg_corpus_test PRIVATE
        ${UTEST_INCLUDE_DIR}
        ${QCOMMON_TEST_INCLUDE_DIRS}
    )
    target_link_libraries(msg_corpus_test PRIVATE m)
    qcommon_test_trim_unused(msg_corpus_test)
    qf_set_output_dir(msg_corpus_test test)
endif()

#
# libFuzzer targets. Require clang; the fuzzer only generates input, so these are
# only useful together with a sanitizer.
#
if(BUILD_FUZZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "BUILD_FUZZERS requires clang (libFuzzer). Configure with CC=clang.")
    endif()

    function(add_qcommon_fuzzer)
        cmake_parse_arguments(P_ARGS "" "NAME" "SRCS" ${ARGN})
        add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS})
        target_include_directories(${P_ARGS_NAME} PRIVATE ${QCOMMON_TEST_INCLUDE_DIRS})
        target_compile_options(${P_ARGS_NAME} PRIVATE -fsanitize=fuzzer,address -g -O1)
        target_link_options(${P_ARGS_NAME} PRIVATE -fsanitize=fuzzer,address)
        target_link_libraries(${P_ARGS_NAME} PRIVATE m)
        qcommon_test_trim_unused(${P_ARGS_NAME})
        qf_set_output_dir(${P_ARGS_NAME} fuzz)
    endfunction()

    add_qcommon_fuzzer(NAME msg_fuzz SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/msg_fuzz.c"
        ${QCOMMON_TEST_PARSER_SOURCES}
        ${QCOMMON_TEST_STUBS})

    add_qcommon_fuzzer(NAME snap_fuzz SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/snap_fuzz.c"
        ${QCOMMON_TEST_PARSER_SOURCES}
        ${QCOMMON_TEST_STUBS})

    # net_chan.c additionally needs miniz for the zlib chunk helpers
    add_qcommon_fuzzer(NAME netchan_fuzz SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/netchan_fuzz.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../net_chan.c"
        "${MINIZ_SOURCE_DIR}/miniz.c"
        ${QCOMMON_TEST_PARSER_SOURCES}
        ${QCOMMON_TEST_STUBS})

    add_qcommon_fuzzer(NAME info_fuzz SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/info_fuzz.c"
        ${QCOMMON_TEST_PARSER_SOURCES}
        ${QCOMMON_TEST_STUBS})
endif()
