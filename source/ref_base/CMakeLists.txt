

function(add_ref_base_test)
	cmake_parse_arguments(
	   P_ARGS
	   ""
	   "NAME"
	   "SRCS"
	   ${ARGN}
	)
	add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS} ${CMOCKA_SRC_FILES})
	target_include_directories(${P_ARGS_NAME} PRIVATE ${CMOCKA_INCLUDE_DIR} ${STB_INCLUDE_DIR} ".")
	target_link_libraries(${P_ARGS_NAME} PRIVATE "m")
	qf_set_output_dir(${P_ARGS_NAME} test)
endfunction()


# NOTE: this "*.c" glob now also sweeps up shared renderer sources that only
# compile against a consuming renderer's r_local.h:
#   r_q3bsp.c, r_q1q2bsp.c, r_skin.c, r_trace.c, r_texture_buffer_load.c
# Whoever re-enables the test block below must exclude them. The backend-neutral
# sources the test actually wants (r_texture_buf.c, r_texture_decode.c,
# r_texture_format.c) are already listed explicitly there.
file(GLOB REF_GL_COMMON_SOURCES
	"*.c"
	"../gameshared/q_*.c"
	"../qalgo/*.c"
	"../qcommon/bsp.c"
	"../qcommon/patch.c"
	)


#if(BUILD_UNIT_TEST )
#	add_ref_base_test(NAME ref_base_texture_buffer_test SRCS
#		"./test/r_texture_buffer_test.c"
#		${REF_GL_COMMON_SOURCES}
#		"./r_texture_buf.c"
#		"./r_texture_decode.c"
#		"./r_texture_format.c")
#endif()


# ---------------------------------------------------------------------------
# BSP loader fuzzing
#
# r_q3bsp.c parses a file the client downloads from whatever server it joins.
# The harness stubs out the renderer and drives Mod_LoadQ3BrushModel directly.
#
# Sources are listed explicitly rather than taken from the glob above: the glob
# pulls in renderer-coupled files that will not link standalone. The include
# order puts ../ref_gl first so "r_local.h" resolves to the GL renderer's
# header, which (unlike ref_nri's) needs no Vulkan/NRI headers to parse.
# ---------------------------------------------------------------------------

set(REF_BASE_FUZZ_SOURCES
	"./r_q3bsp.c"
	"../qcommon/bsp.c"
	"../qcommon/patch.c"
	"../gameshared/q_math.c"
	"../gameshared/q_shared.c"
	)
file(GLOB REF_BASE_FUZZ_QALGO "../qalgo/*.c")
list(APPEND REF_BASE_FUZZ_SOURCES ${REF_BASE_FUZZ_QALGO})

set(REF_BASE_FUZZ_CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/test/fuzz/corpus")

function(add_ref_base_fuzz)
	cmake_parse_arguments(P_ARGS "LIBFUZZER" "NAME" "SRCS" ${ARGN})

	add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS} ${REF_BASE_FUZZ_SOURCES})
	target_include_directories(${P_ARGS_NAME} PRIVATE
		"../ref_gl" "." ".." "../qcore" ${STB_INCLUDE_DIR})
	target_compile_definitions(${P_ARGS_NAME} PRIVATE
		"FUZZ_CORPUS_DIR=\"${REF_BASE_FUZZ_CORPUS}\"")
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

if(BUILD_UNIT_TEST OR BUILD_FUZZ_TEST)
	# corpus generator: regenerate the checked-in seeds after changing the
	# on-disk BSP structs
	add_executable(gen_bsp_seed "./test/fuzz/gen_seed.c")
	target_include_directories(gen_bsp_seed PRIVATE "." ".." "../qcore")
	qf_set_output_dir(gen_bsp_seed test)
endif()

if(BUILD_UNIT_TEST)
	# replay driver - runs the checked-in corpus (seeds plus any crashers found
	# by the fuzzer) so regressions are caught by the normal CI test sweep
	add_ref_base_fuzz(NAME r_q3bsp_fuzz_test SRCS "./test/fuzz/r_q3bsp_fuzz.c")
endif()

if(BUILD_FUZZ_TEST)
	add_ref_base_fuzz(NAME r_q3bsp_fuzz LIBFUZZER SRCS "./test/fuzz/r_q3bsp_fuzz.c")
endif()
