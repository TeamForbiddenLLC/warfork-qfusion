# qcommon has no library target of its own - its sources are compiled directly
# into the client, server and tv_server. This file exists only to build the
# collision-model fuzz targets.
#
# cm_q3bsp.c parses the same attacker-controlled .bsp as ref_base/r_q3bsp.c,
# but on the server side, so it gets its own harness.

set(CM_FUZZ_SOURCES
	"./cm_q3bsp.c"
	"./bsp.c"
	"./patch.c"
	"../gameshared/q_math.c"
	"../gameshared/q_shared.c"
	)
file(GLOB CM_FUZZ_QALGO "../qalgo/*.c")
list(APPEND CM_FUZZ_SOURCES ${CM_FUZZ_QALGO})

# the seed corpus is shared with the renderer-side target: it is the same file
# format, and a crasher for one loader is worth replaying through the other
set(CM_FUZZ_CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/../ref_base/test/fuzz/corpus")

function(add_cm_fuzz)
	cmake_parse_arguments(P_ARGS "LIBFUZZER" "NAME" "SRCS" ${ARGN})

	add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS} ${CM_FUZZ_SOURCES})
	target_include_directories(${P_ARGS_NAME} PRIVATE "." ".." "../qcore")
	target_compile_definitions(${P_ARGS_NAME} PRIVATE
		"FUZZ_CORPUS_DIR=\"${CM_FUZZ_CORPUS}\"")
	target_link_libraries(${P_ARGS_NAME} PRIVATE "m")

	# -O1 regardless of build type: libFuzzer wants optimized code, and at -O0
	# the compiler emits the unused file-scope default_fs_imports_s initializer
	# from mod_fs.h, dragging in all ~60 FS_* entry points the harness has no
	# reason to provide.
	if(NOT MSVC)
		target_compile_options(${P_ARGS_NAME} PRIVATE -O1)
	endif()

	if(P_ARGS_LIBFUZZER)
		target_compile_definitions(${P_ARGS_NAME} PRIVATE FUZZ_LIBFUZZER)
		target_compile_options(${P_ARGS_NAME} PRIVATE -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
		target_link_options(${P_ARGS_NAME} PRIVATE -fsanitize=fuzzer,address,undefined)
	endif()

	qf_set_output_dir(${P_ARGS_NAME} test)
endfunction()

if(BUILD_UNIT_TEST)
	add_cm_fuzz(NAME cm_q3bsp_fuzz_test SRCS "./test/fuzz/cm_q3bsp_fuzz.c")
endif()

if(BUILD_FUZZ_TEST)
	add_cm_fuzz(NAME cm_q3bsp_fuzz LIBFUZZER SRCS "./test/fuzz/cm_q3bsp_fuzz.c")
endif()
