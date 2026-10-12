if (NOT QFUSION_LIBRARY_TYPE)
	set(QFUSION_LIBRARY_TYPE SHARED)
endif()

project (angelwrap)

include_directories(${ANGELSCRIPT_INCLUDE_DIR})

file(GLOB angelwrap_HEADERS
    "*.h"
    "addon/*.h"
	"../gameshared/q_*.h"
)

file(GLOB angelwrap_SOURCES
    "*.cpp"
    "*.c"
    "addon/*.cpp"
    "../gameshared/q_*.c"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(ANGELWRAP_PLATFORM_LIBRARIES "pthread")
else ()
        set(ANGELWRAP_PLATFORM_LIBRARIES "")
endif()

if (EMSCRIPTEN)
	qf_wasm_strip_common_sources(angelwrap_SOURCES)
endif()

add_library(angelwrap ${QFUSION_LIBRARY_TYPE} ${angelwrap_SOURCES} ${angelwrap_HEADERS})
set_property(TARGET angelwrap PROPERTY CXX_STANDARD 11)
target_link_libraries(angelwrap PRIVATE angelscript ${ANGELWRAP_PLATFORM_LIBRARIES})
qf_set_output_dir(angelwrap libs)
