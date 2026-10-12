if (NOT QFUSION_LIBRARY_TYPE)
	set(QFUSION_LIBRARY_TYPE SHARED)
endif()

project (game)

include_directories(${ANGELSCRIPT_INCLUDE_DIR})

file(GLOB GAME_HEADERS
    "*.h"
    "ai/*.h"
	"../gameshared/*.h"
)

file(GLOB GAME_SOURCES
    "*.cpp"
    "ai/*.cpp"
    "../gameshared/*.c"
    "../matchmaker/mm_rating.c"
)

if (EMSCRIPTEN)
	list(FILTER GAME_SOURCES EXCLUDE REGEX "matchmaker")
	list(FILTER GAME_SOURCES EXCLUDE REGEX "gameshared/q_")
endif()

add_library(game ${QFUSION_LIBRARY_TYPE} ${GAME_SOURCES} ${GAME_HEADERS})
target_link_libraries(game PRIVATE ${ANGELSCRIPT_LIBRARY})
add_dependencies(game angelwrap)
set_property(TARGET game PROPERTY CXX_STANDARD 11)
qf_set_output_dir(game ${QFUSION_GAME_DIR})