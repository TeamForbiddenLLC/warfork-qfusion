project(qfusion)

cmake_minimum_required(VERSION 2.8.12)

message(STATUS "CMAKE_SYSTEM: " ${CMAKE_SYSTEM})
message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_VERSION: " ${CMAKE_SYSTEM_VERSION})
message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})

set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake)
include(QfusionUtils)

set(QFUSION_GAME Warfork)						 
if (QFUSION_GAME)
    if (${QFUSION_GAME} MATCHES "Warfork")
        set(QFUSION_GAME_STEAMAPPID 671610)
        set(QFUSION_GAME_DIR basewf)
        set(QFUSION_CLIENT_NAME warfork)
        set(QFUSION_SERVER_NAME wf_server)
        set(QFUSION_TVSERVER_NAME wftv_server)
        set(QFUSION_APPLICATION_VERSION_HEADER \"version.h\")
        set(QFUSION_MAC_ICON ../../icons/warfork.icns)
        set(QFUSION_MAC_INFO_PLIST ../mac/Warfork-Info.plist)
    endif()
endif()

if (QFUSION_APPLICATION_VERSION_HEADER)
    add_definitions(-DAPPLICATION_VERSION_HEADER=${QFUSION_APPLICATION_VERSION_HEADER})
endif()

option(USE_SDL2 "Build using SDL2" OFF)
option(BUILD_UNIT_TEST "Build with unit test" OFF)
option(GAME_MODULES_ONLY "Only build game modules" OFF)
option(BUILD_STEAMLIB "Include the steamlib module (requires Steamworks SDK)" OFF)

# We compile third-party libs from source

set(ANGELSCRIPT_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/angelscript/sdk/angelscript)
set(ANGELSCRIPT_INCLUDE_DIR ${ANGELSCRIPT_DIR}/include/)
add_subdirectory(${ANGELSCRIPT_DIR}/projects/cmake ${ANGELSCRIPT_DIR}/lib/)

set(CMOCKA_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/cmocka)
set(CMOCKA_INCLUDE_DIR ${CMOCKA_DIR}/include/)
set(CMOCKA_SRC_DIR ${CMOCKA_DIR}/src/)
set(CMOCKA_SRC_FILES ${CMOCKA_SRC_DIR}/cmocka.c)

set(LIBROCKET_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/libRocket)
set(LIBROCKET_INCLUDE_DIR ${LIBROCKET_DIR}/Include)
set(LIBROCKET_SOURCE_DIR ${LIBROCKET_DIR}/Source)

set(MINIZ_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/miniz)
set(MINIZ_INCLUDE_DIR ${MINIZ_DIR})
set(MINIZ_SOURCE_DIR ${MINIZ_DIR})

set(STB_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/stb)
set(STB_INCLUDE_DIR ${STB_DIR})

set(DISCORD_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/discord)
set(DISCORD_INCLUDE_DIR ${DISCORD_DIR})

set(OPENAL_DIR ${CMAKE_HOME_DIRECTORY}/../libsrcs/src/openal-soft)
set(OPENAL_INCLUDE_DIR ${OPENAL_DIR}/include/AL)

# Windows specific settings
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(QFUSION_WINDOWS_ARCH "x86")

    if ((MINGW AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
        set(QFUSION_WINDOWS_ARCH "x64")
    endif()

    set(CMAKE_INCLUDE_PATH
		"../libsrcs/src/libogg"
		"../libsrcs/src/libvorbis"
		"../libsrcs/src/libcurl/include"
		"../libsrcs/src/libfreetype"
		"../libsrcs/src/SDL2/include"
		"../libsrcs/src/openal-soft/include/AL"
    )

    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DCURL_STATICLIB -DWIN32 -D_WINDOWS)

    if (MSVC)
        set(CMAKE_LIBRARY_PATH "../libsrcs/lib/win32/${QFUSION_WINDOWS_ARCH}/debug")
    else()
        set(CMAKE_LIBRARY_PATH "../libsrcs/lib/win32/${QFUSION_WINDOWS_ARCH}/mingw")
    endif()

    if (MSVC)
        add_release_link_flags("/LTCG")

		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /W4 /Ob1 /GL /Ot")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /W4")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /W4 /Ob1 /GL /Ot")
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /W4")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /W4")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MD /W4")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MD /W4")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MD /W4")
    endif()
endif()

# OS X specific settings
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(ARCHS_STANDARD_32_64_BIT "x86_64")                         # Compile only 64-bit version
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.11")     # Use 10.11 as deployment target
    set(CMAKE_XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD "gnu99")     # Use GNU99 standart for compiling C files
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11") # Use C++11 standart for compiling C++ files
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")          # Use modern libc++ instead of libstdc++
    set(CMAKE_FRAMEWORK_PATH "../libsrcs/lib/mac/Frameworks")
    set(CMAKE_MACOSX_RPATH 1)
    set(USE_SDL2 1)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function -fvisibility=hidden -Winvalid-pch")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -D_DEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing -ffast-math -funroll-loops -msse2")

    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

    if(XCODE)
        option(WF_BUILD_DSYM "Enable generation of dSYM information" OFF)
        if(WF_BUILD_DSYM)
            set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
            set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
            set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
            set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release] "YES")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
            set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        else()
            set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")
            set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf")
        endif()

        # Debug and RelWithDebInfo configurations should always generate symbols
        set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug] "dwarf-with-dsym")
        set(CMAKE_XCODE_ATTRIBUTE_GENERATE_PROFILING_CODE[variant=Debug] "YES")
        set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=RelWithDebInfo] "dwarf-with-dsym")
        set(CMAKE_XCODE_ATTRIBUTE_GENERATE_PROFILING_CODE[variant=RelWithDebInfo] "YES")
    endif()
endif()

# Linux or mingw specific settings
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR MINGW)
    # If no build type was specified - use release build settings
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()

    option(WF_BUILD_32 "Enable 32bit builds" OFF)
    if(WF_BUILD_32)
        set(CMAKE_CROSSCOMPILING)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -m32")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    endif()

    add_definitions(-D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wno-unused-function -fvisibility=hidden -Winvalid-pch")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -D_DEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing -ffast-math -funroll-loops -msse2 -O2")

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=500")

    # default to sdl2 build on non-windows platforms
    set(USE_SDL2 1)
    

    # linking flags can't be used for static libs
    if (!TARGET_IS_STATIC_LIB)
        # -Wl,--as-needed
        add_common_link_flags("-Wl,--as-needed")

        # -s in release link flags
        add_release_link_flags("-s")

        if (MINGW)
            add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_VISTA)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
            add_common_link_flags("-static -static-libgcc -static-libstdc++")
        endif()

    endif()
endif()

# Linux-only specific settings
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if (CMAKE_CROSSCOMPILING)
        set(CMAKE_LIBRARY_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
        link_directories("${CMAKE_FIND_ROOT_PATH}/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    endif()

    # Remove compilating flags from link options
    # string(REPLACE "<FLAGS>" "" CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE}")
    # string(REPLACE "<LANGUAGE_COMPILE_FLAGS>" "" CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")
    # string(REPLACE "<LANGUAGE_COMPILE_FLAGS>" "" CMAKE_C_CREATE_SHARED_MODULE "${CMAKE_C_CREATE_SHARED_MODULE}")

    # Remove soname link flags from library linking options
    string(REPLACE "<SONAME_FLAG><TARGET_SONAME>" "" CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY}")
    string(REPLACE "<SONAME_FLAG><TARGET_SONAME>" "" CMAKE_C_CREATE_SHARED_MODULE "${CMAKE_C_CREATE_SHARED_MODULE}")

    # Remove -rdynamic from link options
    string(REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
endif()

if (NOT GAME_MODULES_ONLY)
    find_package(Ogg REQUIRED)
    find_package(Vorbis REQUIRED)
    find_package(CURL REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(OpenAL REQUIRED)

    if (USE_SDL2)
        set(USE_SDL2 1)
        find_package(SDL2 REQUIRED)
        add_definitions(-DUSE_SDL2)
    endif()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR MINGW)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        STRING(TOLOWER "win32_${QFUSION_WINDOWS_ARCH}.tar.gz" TAR_GZ_NAME)
    else()
        STRING(TOLOWER "${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}.tar.gz" TAR_GZ_NAME)
    endif()
    if (NOT QFUSION_TAR_GZ_OUTPUT_DIRECTORY)
        set(TAR_GZ_DIST "${QFUSION_CLIENT_NAME}_${TAR_GZ_NAME}")
    else()
        set(TAR_GZ_DIST "${QFUSION_TAR_GZ_OUTPUT_DIRECTORY}/${QFUSION_CLIENT_NAME}_${TAR_GZ_NAME}")
    endif()
    add_custom_target(tar_gz COMMAND ${CMAKE_COMMAND} -E tar "cfvz" ${TAR_GZ_DIST} "${CMAKE_HOME_DIRECTORY}/build/")
    add_dependencies(tar_gz ${QFUSION_CLIENT_NAME} game ${QFUSION_SERVER_NAME} ${QFUSION_TVSERVER_NAME})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_EXECUTABLE_SUFFIX "_${QFUSION_WINDOWS_ARCH}${CMAKE_EXECUTABLE_SUFFIX}")
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_LIBRARY_SUFFIX "_${QFUSION_WINDOWS_ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if (NOT GAME_MODULES_ONLY)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        find_windows_release_libs(FREETYPE_LIBRARIES)
        find_windows_release_libs(OGG_LIBRARY)
        find_windows_release_libs(VORBIS_LIBRARIES)
        find_windows_release_libs(OPENAL_LIBRARIES)
        find_windows_release_libs(CURL_LIBRARY)
        find_windows_release_libs(ANGELSCRIPT_LIBRARY)
        find_windows_release_libs(SDL2_LIBRARIES)
    endif()
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") # Do not add 'OR MINGW' here - we need to use suffix-prefix settings from Windows
    set(CURL_LIBRARY "")
    add_definitions(-DLIBCURL_RUNTIME)

    set(OGG_LIBRARY "")
    add_definitions(-DOGGLIB_RUNTIME)

    set(VORBIS_LIBRARIES "")
    add_definitions(-DVORBISLIB_RUNTIME)

    set(FREETYPE_LIBRARIES "")
    add_definitions(-DFREETYPELIB_RUNTIME)

    set(CMAKE_EXECUTABLE_SUFFIX ".${CMAKE_SYSTEM_PROCESSOR}${CMAKE_EXECUTABLE_SUFFIX}")
    set(CMAKE_SHARED_LIBRARY_SUFFIX "_${CMAKE_SYSTEM_PROCESSOR}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_SHARED_LIBRARY_SUFFIX "_mac${CMAKE_SHARED_LIBRARY_SUFFIX}")

    capitalize(QFUSION_CLIENT_NAME)
endif()

add_subdirectory(ref_base)
add_subdirectory(angelwrap)
add_subdirectory(cgame)
add_subdirectory(game)
add_subdirectory(steamshim)
add_subdirectory(gameshared/q_math)


if (NOT GAME_MODULES_ONLY)
    add_subdirectory(cin)
    add_subdirectory(ftlib)
    add_subdirectory(ref_gl)
    add_subdirectory(snd_openal)
    add_subdirectory(ui)
    add_subdirectory(server)
    add_subdirectory(tv_server)
    add_subdirectory(client)
endif()
