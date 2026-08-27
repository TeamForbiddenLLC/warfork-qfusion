project(vectmath)


function(add_ref_math_test) 
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

if(BUILD_UNIT_TEST )
  add_ref_math_test(NAME vec_test SRCS "test/vector_test.c")
endif()

