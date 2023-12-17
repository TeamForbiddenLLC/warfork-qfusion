project(${QFUSION_CLIENT_NAME})

include_directories(${MINIZ_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${DISCORD_INCLUDE_DIR})

if (USE_SDL2 OR (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    include_directories(${SDL2_INCLUDE_DIR})
endif()

if (NOT QFUSION_MAC_ICON)
	set(QFUSION_MAC_ICON ../../icons/warfork.icns)
endif()
if (NOT QFUSION_MAC_INFO_PLIST)
	set(QFUSION_MAC_INFO_PLIST ../mac/Warfork-Info.plist)
endif()

file(GLOB CLIENT_HEADERS
    "*.h"
	"../gameshared/q_*.h"
	"../gameshared/anorms.h"
	"../gameshared/config.h"
	"../qcommon/*.h"
	"../qalgo/*.h"
	"../angelwrap/qas_public.h"
	"../ref_gl/r_public.h"
	"../cgame/cg_public.h"
	"../cgame/ref.h"
	"../game/g_public.h"
	"../ui/ui_public.h"
	"../ftlib/ftlib_public.h"
	"../steamlib/steamlib_public.h"
	"../cin/cin_public.h"
	"../matchmaker/*.h"
    "${MINIZ_INCLUDE_DIR}/miniz.h"
	"${DISCORD_INCLUDE_DIR}/*.h"
)

file(GLOB CLIENT_COMMON_SOURCES
    "../qcommon/*.c"
    "../server/*.c"
    "*.c"
    "../gameshared/q_*.c"
    "../qalgo/*.c"
    "../matchmaker/*.c"
    "${MINIZ_SOURCE_DIR}/miniz.c"
	"${DISCORD_INCLUDE_DIR}/discord_rpc.cpp"
	"${DISCORD_INCLUDE_DIR}/rpc_connection.cpp"
	"${DISCORD_INCLUDE_DIR}/serialization.cpp"
)
    
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	file(GLOB CLIENT_PLATFORM_HEADERS
		"../win32/resource.h"
		"../win32/conproc.h"
		"../win32/winquake.h"
		"../win32/win_input.h"
	)

    if (USE_SDL2)
        file(GLOB CLIENT_PLATFORM_SOURCES
            "../win32/win_fs.c"
            "../win32/win_net.c"
            "../win32/win_sys.c"
            "../win32/win_threads.c"
            "../null/sys_vfs_null.c"

            "../sdl/sdl_input.c"
            "../sdl/sdl_input_joy.c"
            "../sdl/sdl_input_win.c"
            "../sdl/sdl_vid.c"
            "../sdl/sdl_sys.c"
            "../sdl/sdl_clipboard.c"
            "../sdl/sdl_time.c"
            "../sdl/sdl_lib.c"
            "../sdl/sdl_client.c"

            "../win32/qfusion.rc"
            
            "${DISCORD_INCLUDE_DIR}/connection_win.cpp"	    
            "${DISCORD_INCLUDE_DIR}/discord_register_win.cpp"
        )

        set(CLIENT_PLATFORM_LIBRARIES ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY} "ws2_32.lib" "winmm.lib")
    else()
        file(GLOB CLIENT_PLATFORM_SOURCES
            "../win32/win_fs.c"
            "../win32/win_lib.c"
            "../win32/win_net.c"
            "../win32/win_sys.c"
            "../win32/win_console.c"
            "../win32/win_clipboard.c"
            "../win32/win_time.c"
            "../win32/win_threads.c"
            "../null/sys_vfs_null.c"

            "../win32/win_client.c"
            "../win32/win_input.c"
            "../win32/win_vid.c"
            "../win32/conproc.c"

            "../win32/qfusion.rc"
            
            "${DISCORD_INCLUDE_DIR}/connection_win.cpp"	    
            "${DISCORD_INCLUDE_DIR}/discord_register_win.cpp"
        )

        set(CLIENT_PLATFORM_LIBRARIES "ws2_32.lib" "winmm.lib")
    endif()

    set(CLIENT_BINARY_TYPE WIN32)

    set(BUNDLE_RESOURCES "")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CLIENT_PLATFORM_HEADERS "")
	
    file(GLOB CLIENT_PLATFORM_SOURCES
        "../unix/unix_fs.c"
        "../unix/unix_net.c"
        "../unix/unix_sys.c"
        "../unix/unix_threads.c"
        "../null/sys_vfs_null.c"

        "../sdl/sdl_input.c"
        "../sdl/sdl_input_joy.c"
        "../sdl/sdl_input_osx.m"
        "../sdl/sdl_vid.c"
        "../sdl/sdl_sys.c"
        "../sdl/sdl_clipboard.c"
        "../sdl/sdl_time.c"
        "../sdl/sdl_lib.c"
        "../sdl/sdl_client.c"

        "../mac/mac_sys.m"
        
        "${DISCORD_INCLUDE_DIR}/connection_unix.cpp"	    
        "${DISCORD_INCLUDE_DIR}/discord_register_osx.m"
	        
        "${QFUSION_MAC_INFO_PLIST}"
    )

    set(CLIENT_PLATFORM_LIBRARIES ${SDL2_LIBRARY} "-framework IOKit" "-framework Cocoa")
    set(CLIENT_BINARY_TYPE MACOSX_BUNDLE)

    set(BUNDLE_RESOURCES
        ../../libsrcs/lib/mac/Frameworks
        ${QFUSION_MAC_ICON}
    )
	if (QFUSION_DATA_DIR)
		list(APPEND BUNDLE_RESOURCES
			${QFUSION_DATA_DIR}/${QFUSION_GAME_DIR}
			${QFUSION_DATA_DIR}/docs
		)
	endif()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(CLIENT_PLATFORM_HEADERS "")

    if (USE_SDL2)
        file(GLOB CLIENT_PLATFORM_SOURCES
            "../unix/unix_fs.c"
            "../unix/unix_net.c"
            "../unix/unix_sys.c"
            "../unix/unix_threads.c"
            "../null/sys_vfs_null.c"

            "../sdl/sdl_input.c"
            "../sdl/sdl_input_joy.c"
            "../sdl/sdl_input_unix.c"
            "../sdl/sdl_vid.c"
            "../sdl/sdl_sys.c"
            "../sdl/sdl_clipboard.c"
            "../sdl/sdl_time.c"
            "../sdl/sdl_lib.c"
            "../sdl/sdl_client.c"

        	"${DISCORD_INCLUDE_DIR}/connection_unix.cpp"	    
            "${DISCORD_INCLUDE_DIR}/discord_register_linux.cpp"	
        )

        set(CLIENT_PLATFORM_LIBRARIES ${SDL2_LIBRARY} "pthread" "m" "dl" "rt")
    else()
        file(GLOB CLIENT_PLATFORM_SOURCES
            "../unix/unix_fs.c"
            "../unix/unix_lib.c"
            "../unix/unix_net.c"
            "../unix/unix_sys.c"
            "../unix/unix_console.c"
            "../unix/unix_clipboard.c"
            "../unix/unix_time.c"
            "../unix/unix_threads.c"
            "../null/sys_vfs_null.c"

            "../unix/keysym2ucs.c"
            "../unix/unix_client.c"
            "../unix/unix_input.c"
            "../unix/unix_vid.c"

            "../sdl/sdl_input_joy.c"

	        "${DISCORD_INCLUDE_DIR}/connection_unix.cpp"	    
	        "${DISCORD_INCLUDE_DIR}/discord_register_linux.cpp"	
        )
    
        set(CLIENT_PLATFORM_LIBRARIES ${SDL2_LIBRARY} "pthread" "dl" "m" "X11" "Xi" "Xext" "Xrandr" "rt")
    endif()
    set(CLIENT_BINARY_TYPE "")

    set(BUNDLE_RESOURCES "")
endif()

add_executable(${QFUSION_CLIENT_NAME} ${CLIENT_BINARY_TYPE} ${CLIENT_HEADERS} ${CLIENT_PLATFORM_HEADERS} ${CLIENT_COMMON_SOURCES} ${CLIENT_PLATFORM_SOURCES} ${BUNDLE_RESOURCES} )
add_dependencies(${QFUSION_CLIENT_NAME} angelwrap cgame cin ftlib game ref_gl snd_openal ui)
if (BUILD_STEAMLIB)
    set(STEAMSHIMCHILD_LIBRARY steamshim_child)
endif()


execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/.. OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
file(WRITE ${CMAKE_HOME_DIRECTORY}/build/commit.txt ${GIT_COMMIT_HASH})

file(WRITE ${CMAKE_HOME_DIRECTORY}/build/steam_appid.txt ${QFUSION_GAME_STEAMAPPID})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_target_properties(${QFUSION_CLIENT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${QFUSION_MAC_INFO_PLIST})
    set_target_properties(${QFUSION_CLIENT_NAME} PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/../Resources/base @executable_path/../Resources/libs")
    set_source_files_properties(${QFUSION_MAC_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set_source_files_properties(../../libsrcs/lib/mac/Frameworks                 PROPERTIES MACOSX_PACKAGE_LOCATION .)
	if (QFUSION_DATA_DIR)
	    set_source_files_properties(${QFUSION_DATA_DIR}/${QFUSION_GAME_DIR}    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		set_source_files_properties(${QFUSION_DATA_DIR}/docs    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
	endif()

    set(BUNDLE_PATH ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/${QFUSION_CLIENT_NAME}.app/Contents)
    set(POST_BUILD_CMDS
        cp -r ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/${QFUSION_GAME_DIR}/ ${BUNDLE_PATH}/Resources/${QFUSION_GAME_DIR}/ &&
        cp -r ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/libs/ ${BUNDLE_PATH}/Resources/libs/ &&
        cp ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/${QFUSION_SERVER_NAME} ${BUNDLE_PATH}/MacOS/ &&
        cp ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/${QFUSION_TVSERVER_NAME} ${BUNDLE_PATH}/MacOS/ &&
        cp ${CMAKE_HOME_DIRECTORY}/build/libsteam_api.dylib ${BUNDLE_PATH}/MacOS/ &&
        cp ${CMAKE_HOME_DIRECTORY}/build/commit.txt ${BUNDLE_PATH}/MacOS/ &&
        cp ${CMAKE_HOME_DIRECTORY}/build/steam_appid.txt ${BUNDLE_PATH}/MacOS/
    )
        
    add_custom_command(TARGET ${QFUSION_CLIENT_NAME} POST_BUILD COMMAND ${POST_BUILD_CMDS})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_custom_command(TARGET ${QFUSION_CLIENT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_HOME_DIRECTORY}/../libsrcs/src/openal-soft/bin/Win64/soft_oal.dll ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/soft_oal64.dll)
    else()
        add_custom_command(TARGET ${QFUSION_CLIENT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_HOME_DIRECTORY}/../libsrcs/src/openal-soft/bin/Win64/soft_oal.dll ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/soft_oal64.dll)
    endif()
    add_custom_command(TARGET ${QFUSION_CLIENT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_HOME_DIRECTORY}/build/commit.txt ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/)
    add_custom_command(TARGET ${QFUSION_CLIENT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_HOME_DIRECTORY}/build/steam_appid.txt ${CMAKE_HOME_DIRECTORY}/build/$(CONFIGURATION)/)
endif()

target_link_libraries(${QFUSION_CLIENT_NAME} PRIVATE ${CURL_LIBRARY} ${CLIENT_PLATFORM_LIBRARIES} ${DISCORD_LIBRARY} ${STEAMSHIMCHILD_LIBRARY})
qf_set_output_dir(${QFUSION_CLIENT_NAME} "")

if (BUILD_STEAMLIB)
    add_dependencies(wf_steam ${QFUSION_CLIENT_NAME})
endif()
