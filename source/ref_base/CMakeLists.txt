

function(add_ref_gl_test) 
	cmake_parse_arguments(
	   P_ARGS
	   ""
	   "NAME"
	   "SRCS"
	   ${ARGN}
	)
	add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS} ${UNITY_SRC_FILES})
	target_include_directories(${P_ARGS_NAME} PRIVATE ${UNITY_INCLUDE_DIR} ${STB_INCLUDE_DIR} "../ref_base")
	qf_set_output_dir(${P_ARGS_NAME} test)
endfunction()

if(BUILD_UNIT_TEST )

add_ref_gl_test(NAME ref_gl_texture_buffer_text SRCS "./test/r_texture_buffer_test.c")

endif()


