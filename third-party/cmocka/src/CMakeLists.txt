project(cmocka-library C)

set(CMOCKA_PLATFORM_INCLUDE CACHE PATH "Path to include directory for cmocka_platform.h")
mark_as_advanced(CMOCKA_PLATFORM_INCLUDE)

set(CMOCKA_LINK_LIBRARIES
    ${CMOCKA_REQUIRED_LIBRARIES}
    CACHE INTERNAL "cmocka link libraries"
)

set(cmocka_SRCS
    cmocka.c
)

add_library(cmocka ${cmocka_SRCS})

target_include_directories(cmocka
                           PRIVATE
                               ${CMOCKA_PLATFORM_INCLUDE}
                               ${cmocka_BINARY_DIR}
                           PUBLIC
                               $<BUILD_INTERFACE:${cmocka-header_SOURCE_DIR}>
                               $<INSTALL_INTERFACE:include>)

target_compile_options(cmocka
                       PRIVATE
                           ${DEFAULT_C_COMPILE_FLAGS}
                           -DHAVE_CONFIG_H)

target_compile_definitions(cmocka
                           PRIVATE
                               _GNU_SOURCE
                               _XOPEN_SOURCE=700
                           PUBLIC
                               # hides a warning on Windows when using static lib
                               "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:CMOCKA_STATIC>"
                           )

if (CMOCKA_PLATFORM_INCLUDE)
    target_compile_options(cmocka
                           PRIVATE
                               -DCMOCKA_PLATFORM_INCLUDE)
endif()

target_link_libraries(cmocka PRIVATE ${CMOCKA_LINK_LIBRARIES})
set_target_properties(cmocka PROPERTIES
                      DEFINE_SYMBOL         CMOCKA_EXPORTS
                      VERSION               ${LIBRARY_VERSION}
                      SOVERSION             ${LIBRARY_SOVERSION}
                      LINK_FLAGS            "${DEFAULT_LINK_FLAGS}"
                      C_STANDARD            99
                      C_STANDARD_REQUIRED   ON
                      # cmocka uses POSIX-2008 C extensions
                      C_EXTENSIONS          ON
                      # automatically export all global objects into DLL on Windows
                      WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_library(cmocka::cmocka ALIAS cmocka)

install(TARGETS cmocka
        EXPORT cmocka-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT ${PROJECT_NAME})

install(EXPORT cmocka-targets
        NAMESPACE cmocka::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cmocka)

if (UNIT_TESTING)
    # Internal library
    add_library(cmocka-static STATIC ${cmocka_SRCS})

    target_include_directories(cmocka-static
                               PRIVATE
                                   ${CMOCKA_PLATFORM_INCLUDE}
                                   ${cmocka_BINARY_DIR}
                               PUBLIC
                                   ${cmocka-header_SOURCE_DIR})


    target_compile_options(cmocka-static
                           PRIVATE
                               ${DEFAULT_C_COMPILE_FLAGS}
                               -DHAVE_CONFIG_H)

    target_compile_definitions(cmocka-static
                               PRIVATE
                                   _GNU_SOURCE
                                   _XOPEN_SOURCE=700
                               PUBLIC
                                   CMOCKA_STATIC # hides a warning on Windows
                              )

    if (CMOCKA_PLATFORM_INCLUDE)
        target_compile_options(cmocka-static
                               PRIVATE
                                   -DCMOCKA_PLATFORM_INCLUDE)
    endif()

    target_link_libraries(cmocka-static PRIVATE ${CMOCKA_LINK_LIBRARIES})

    add_library(cmocka::static ALIAS cmocka-static)
endif()
