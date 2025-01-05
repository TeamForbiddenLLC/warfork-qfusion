# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs CACHE PATH "Build directory for static libraries and import libraries")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs CACHE PATH "Build directory for shared libraries")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE PATH "Build directory for executables")

# We install outside of the binary dir because our install support muliple platforms to 
# be installed together. We also have an exclusion rule in the AP that filters out the 
# "install" folder to avoid the AP picking it up
unset(define_with_force)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(define_with_force FORCE)
endif()
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install CACHE PATH "Install directory" ${define_with_force})

cmake_path(ABSOLUTE_PATH CMAKE_BINARY_DIR NORMALIZE OUTPUT_VARIABLE cmake_binary_dir_normalized)
cmake_path(ABSOLUTE_PATH CMAKE_INSTALL_PREFIX NORMALIZE OUTPUT_VARIABLE cmake_install_prefix_normalized)
cmake_path(COMPARE ${cmake_binary_dir_normalized} EQUAL ${cmake_install_prefix_normalized} are_paths_equal)
if(are_paths_equal)
    message(FATAL_ERROR "Binary dir is the same path as install prefix, indicate a different install prefix with "
        "CMAKE_INSTALL_PREFIX or a different binary dir with -B <binary dir>")
endif()

