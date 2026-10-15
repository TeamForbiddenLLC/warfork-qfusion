# Emscripten / WebAssembly helpers for the Warfork client.
# Included from source/CMakeLists.txt when EMSCRIPTEN is true.
# Native Windows/Linux/macOS builds never load this file.

if (NOT EMSCRIPTEN)
    return()
endif()

message(STATUS "Qfusion: configuring WebAssembly (Emscripten) client")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(QFUSION_WASM ON CACHE BOOL "Building the WASM client" FORCE)
set(USE_SDL2 ON CACHE BOOL "SDL2 is required for the WASM client" FORCE)
set(USE_GRAPHICS_NRI OFF CACHE BOOL "NRI backends are not available in the browser" FORCE)
set(BUILD_STEAMLIB OFF CACHE BOOL "Steamworks is not available in the browser" FORCE)
set(BUILD_UNIT_TEST OFF CACHE BOOL "" FORCE)
set(USE_CRASHPAD OFF CACHE BOOL "" FORCE)
set(QFUSION_LIBRARY_TYPE STATIC)

add_compile_definitions(__WASM__=1)

# Static link: modules must not provide their own Com_Printf/Sys_Error
# wrappers (those call trap_Print -> engine Print -> Com_Printf forever).
add_compile_definitions(
	FTLIB_HARD_LINKED=1
	REF_HARD_LINKED=1
	SOUND_HARD_LINKED=1
	CIN_HARD_LINKED=1
	ANGELWRAP_HARD_LINKED=1
	GAME_HARD_LINKED=1
	CGAME_HARD_LINKED=1
	UI_HARD_LINKED=1
)

# Desktop SIMD flags from the Linux block must not leak into wasm32.
string(REPLACE "-msse2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
string(REPLACE "-msse2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

set(QFUSION_WASM_LINK_FLAGS
    "SHELL:-sUSE_SDL=2"
    "SHELL:-sUSE_WEBGL2=1"
    "SHELL:-sMIN_WEBGL_VERSION=2"
    "SHELL:-sMAX_WEBGL_VERSION=2"
    "SHELL:-sFULL_ES3=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sINITIAL_MEMORY=268435456"
    "SHELL:-sSTACK_SIZE=1048576"
    "SHELL:-sINVOKE_RUN=0"
    "SHELL:-sEXIT_RUNTIME=0"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=['callMain','ccall','cwrap','FS']"
    "SHELL:-sEXPORTED_FUNCTIONS=['_main','_malloc','_free','_WASM_TouchCmd','_WASM_TouchMove','_WASM_TouchKey','_WASM_AddMasterServer']"
    "SHELL:-sENVIRONMENT=web,worker"
    "SHELL:-sNO_DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:-lwebsocket.js"
    "SHELL:-lopenal"
    "SHELL:-sFETCH=1"
    "SHELL:-Wl,--allow-multiple-definition"
    "SHELL:--profiling-funcs"
)

set(QFUSION_WASM_COMPILE_FLAGS
    "SHELL:-sUSE_SDL=2"
    "SHELL:-sUSE_WEBGL2=1"
)

set(QFUSION_WASM_PRELOAD_DIR "${CMAKE_HOME_DIRECTORY}/../web/preload")
if (EXISTS "${QFUSION_WASM_PRELOAD_DIR}")
    list(APPEND QFUSION_WASM_LINK_FLAGS "SHELL:--preload-file ${QFUSION_WASM_PRELOAD_DIR}@/")
endif()

add_compile_options(${QFUSION_WASM_COMPILE_FLAGS})

# Shared engine .c files are compiled into every native .so. A static WASM
# link may only include them once (the client + game already provide them).
function(qf_wasm_strip_common_sources var)
    set(_srcs ${${var}})
    list(FILTER _srcs EXCLUDE REGEX "gameshared")
    list(FILTER _srcs EXCLUDE REGEX "qalgo")
    list(FILTER _srcs EXCLUDE REGEX "qcommon/bsp\\.c")
    list(FILTER _srcs EXCLUDE REGEX "qcommon/patch\\.c")
    set(${var} "${_srcs}" PARENT_SCOPE)
endfunction()

function(qf_apply_wasm_client_flags target)
    target_link_options(${target} PRIVATE ${QFUSION_WASM_LINK_FLAGS})
endfunction()
