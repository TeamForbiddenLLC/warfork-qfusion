

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
#   r_q3bsp.c, r_q1q2bsp.c, r_skin.c, r_trace.c, r_cull.c, r_texture_buffer_load.c
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




# Content-derived shader cache keys (replaces the manual GLSL_BITS_VERSION bump).
# Consumers: link ref_generated and #include "r_generated.h".
set(REF_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(REF_GENERATED_HEADER "${REF_GENERATED_DIR}/r_generated.h")
set(REF_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/data0_21pure")

file(GLOB_RECURSE REF_GENERATED_DEPS CONFIGURE_DEPENDS
	"${REF_ASSETS_DIR}/glsl/*"
	"${REF_ASSETS_DIR}/glsl_nri/*")
list(APPEND REF_GENERATED_DEPS
	"${CMAKE_CURRENT_SOURCE_DIR}/../ref_gl/r_program.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/../ref_gl/r_program.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../ref_nri/r_program.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/../ref_nri/r_program.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../glslang/known_good.json")

add_custom_command(
	OUTPUT "${REF_GENERATED_HEADER}"
	COMMAND ${CMAKE_COMMAND}
		-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/..
		-DASSETS_DIR=${REF_ASSETS_DIR}
		-DOUT=${REF_GENERATED_HEADER}
		-P "${CMAKE_CURRENT_SOURCE_DIR}/gen_glsl_cache_key.cmake"
	DEPENDS ${REF_GENERATED_DEPS} "${CMAKE_CURRENT_SOURCE_DIR}/gen_glsl_cache_key.cmake"
	COMMENT "Generating r_generated.h (shader cache keys)"
	VERBATIM)
add_custom_target(ref_generated_header DEPENDS "${REF_GENERATED_HEADER}")

add_library(ref_generated INTERFACE)
target_include_directories(ref_generated INTERFACE "${REF_GENERATED_DIR}")
add_dependencies(ref_generated ref_generated_header)
