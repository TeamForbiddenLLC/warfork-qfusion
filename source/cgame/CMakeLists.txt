if (NOT QFUSION_LIBRARY_TYPE)
	set(QFUSION_LIBRARY_TYPE SHARED)
endif()

project (cgame)

file(GLOB CGAME_HEADERS
    "*.h"
	"../gameshared/*.h"

)

file(GLOB CGAME_SOURCES
    "*.cpp"
    "../gameshared/*.c"
   	"../qalgo/rng.cpp"

)

if (EMSCRIPTEN)
	qf_wasm_strip_common_sources(CGAME_SOURCES)
	list(APPEND CGAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../qalgo/rng.cpp")
	# Own copy of gs_*.c so cgame does not overwrite the game module's gs/Pmove.
	file(GLOB CGAME_GS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../gameshared/gs_*.c")
	list(APPEND CGAME_SOURCES ${CGAME_GS_SOURCES})
endif()

add_library(cgame ${QFUSION_LIBRARY_TYPE} ${CGAME_SOURCES} ${CGAME_HEADERS})
set_property(TARGET cgame PROPERTY CXX_STANDARD 11)
if (EMSCRIPTEN)
	target_compile_options(cgame PRIVATE "-include" "${CMAKE_CURRENT_SOURCE_DIR}/cg_wasm_gs_prefix.h")
endif()
qf_set_output_dir(cgame ${QFUSION_GAME_DIR})