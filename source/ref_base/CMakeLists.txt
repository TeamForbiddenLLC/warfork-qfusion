

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


