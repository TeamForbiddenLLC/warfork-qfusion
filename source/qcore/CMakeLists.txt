project(qcore)

file (GLOB QCORE_SOURCE 
    "./*.c")

add_library(qcore STATIC ${QCORE_SOURCE})
target_include_directories (qcore PUBLIC "./")

function(add_qcore_test) 
	cmake_parse_arguments(
	   P_ARGS
	   ""
	   "NAME"
	   "SRCS"
	   ${ARGN}
	)
	add_executable(${P_ARGS_NAME} ${P_ARGS_SRCS} ${CMOCKA_SRC_FILES})
	target_include_directories(${P_ARGS_NAME} PRIVATE ${UTEST_INCLUDE_DIR} ".")
	target_link_libraries(${P_ARGS_NAME} PRIVATE qcore)
	qf_set_output_dir(${P_ARGS_NAME} test)
endfunction()


if(BUILD_UNIT_TEST)
    add_qcore_test(NAME qstr_test SRCS 
		"./test/qstr_test.c")
    add_qcore_test(NAME qstr_utf_test SRCS 
		"./test/qstr_utf_test.c")
    add_qcore_test(NAME scratch_alloc_test SRCS 
		"./test/scratch_alloc_test.c")
endif()


