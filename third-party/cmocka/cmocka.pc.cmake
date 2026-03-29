# cmocka pkg-config source file

prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: @PROJECT_NAME@
Description: The cmocka unit testing library
Version: @PROJECT_VERSION@
Libs: -L${libdir} -lcmocka
Cflags: -I${includedir}
