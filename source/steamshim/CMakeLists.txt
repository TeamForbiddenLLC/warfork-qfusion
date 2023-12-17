project(wf_steam)

add_definitions(
    -DGAME_CLIENT_LAUNCH_NAME="${QFUSION_CLIENT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
    -DGAME_SERVER_LAUNCH_NAME="${QFUSION_SERVER_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
    -DGAME_TVSERVER_LAUNCH_NAME="${QFUSION_TVSERVER_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
    -DUNICODE
    -D_UNICODE
)

set(STEAMWORKS_SDK_DIR ${CMAKE_HOME_DIRECTORY}/../third-party/steamworks/sdk)
include_directories("${STEAMWORKS_SDK_DIR}/public")

set(STEAMWORKS_LIBRARY steam_api)
# Please note, this is not in the 'libs' directory!
set(STEAMWORKS_OUTPUT_PATH ${CMAKE_HOME_DIRECTORY}/build)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(STEAMWORKS_LIBRARY_DIR ${STEAMWORKS_SDK_DIR}/redistributable_bin/win64)
        set(STEAMWORKS_LIBRARY steam_api64)
    else()
        set(STEAMWORKS_LIBRARY_DIR ${STEAMWORKS_SDK_DIR}/redistributable_bin)
    endif()
    set(STEAMWORKS_LIBRARY_EXTENSION .dll)
    # This is not a typo, but using Visual Studio's 'Configuration' property!
    set(STEAMWORKS_OUTPUT_PATH "${CMAKE_HOME_DIRECTORY}/build/$(Configuration)")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(STEAMWORKS_SO_PREFIX lib)
    set(STEAMWORKS_LIBRARY_DIR ${STEAMWORKS_SDK_DIR}/redistributable_bin/osx)
    set(STEAMWORKS_LIBRARY_EXTENSION .dylib)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(STEAMWORKS_SO_PREFIX lib)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(STEAMWORKS_LIBRARY_DIR ${STEAMWORKS_SDK_DIR}/redistributable_bin/linux64)
    else()
        set(STEAMWORKS_LIBRARY_DIR ${STEAMWORKS_SDK_DIR}/redistributable_bin/linux32)
    endif()
    set(STEAMWORKS_LIBRARY_EXTENSION .so)
endif()

link_directories(${STEAMWORKS_LIBRARY_DIR})


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(STEAM_SERVER_BINARY_TYPE WIN32)
else()
    set(STEAM_SERVER_BINARY_TYPE "")
endif()


file(GLOB child_SRC
    "src/*.h"
    "src/*.cpp"
    "src/child/*.h"
    "src/child/*.cpp"
)

add_library(steamshim_child STATIC ${child_SRC})
qf_set_output_dir(steamshim_child libs)

file(GLOB parent_SRC
    "src/*.h"
    "src/*.cpp"
    "src/parent/*.h"
    "src/parent/*.cpp"
)
add_executable(wf_steam ${STEAM_SERVER_BINARY_TYPE} ${parent_SRC})
target_link_libraries(wf_steam ${STEAMWORKS_LIBRARY})

qf_set_output_dir(wf_steam "")

add_custom_command(TARGET wf_steam
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${STEAMWORKS_LIBRARY_DIR}/${STEAMWORKS_SO_PREFIX}${STEAMWORKS_LIBRARY}${STEAMWORKS_LIBRARY_EXTENSION}"
        "${STEAMWORKS_OUTPUT_PATH}"
)
add_custom_command(TARGET wf_steam
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E create_symlink
    "${STEAMWORKS_OUTPUT_PATH}/wf_steam${CMAKE_EXECUTABLE_SUFFIX}" "${STEAMWORKS_OUTPUT_PATH}/wf_server_steam${CMAKE_EXECUTABLE_SUFFIX}"
)
add_custom_command(TARGET wf_steam
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E create_symlink
    "${STEAMWORKS_OUTPUT_PATH}/wf_steam${CMAKE_EXECUTABLE_SUFFIX}" "${STEAMWORKS_OUTPUT_PATH}/warfork_steam${CMAKE_EXECUTABLE_SUFFIX}"
)
add_custom_command(TARGET wf_steam
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E create_symlink
    "${STEAMWORKS_OUTPUT_PATH}/wf_steam${CMAKE_EXECUTABLE_SUFFIX}" "${STEAMWORKS_OUTPUT_PATH}/wftv_server_steam${CMAKE_EXECUTABLE_SUFFIX}"
)
