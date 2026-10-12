if (NOT QFUSION_LIBRARY_TYPE)
	set(QFUSION_LIBRARY_TYPE SHARED)
endif()

project(cin)

file(GLOB CIN_HEADERS
    "*.h"
	"../gameshared/q_*.h"
)

file(GLOB CIN_SOURCES
    "*.c"
    "../gameshared/q_*.c"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CIN_PLATFORM_LIBRARIES "m")
else ()
        set(CIN_PLATFORM_LIBRARIES "")
endif()

if (EMSCRIPTEN)
	qf_wasm_strip_common_sources(CIN_SOURCES)
endif()

add_library(cin ${QFUSION_LIBRARY_TYPE} ${CIN_SOURCES} ${CIN_HEADERS})
target_link_libraries(cin PRIVATE ogg vorbis vorbisfile ${CIN_PLATFORM_LIBRARIES})
qf_set_output_dir(cin libs)
