if (NOT QFUSION_LIBRARY_TYPE)
	set(QFUSION_LIBRARY_TYPE SHARED)
endif()

project(snd_openal)

include_directories("../snd_common" ".")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include_directories("../win32/include") # For al.h
endif()

file(GLOB SND_OPENAL_HEADERS
    "*.h"
	"../gameshared/q_*.h"
	"../client/snd_public.h"
)

file(GLOB SND_OPENAL_SOURCES
    "*.c"
    "../snd_common/*.c"
    "../gameshared/q_*.c"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(SND_OPENAL_PLATFORM_LIBRARIES "")
else ()
        set(SND_OPENAL_PLATFORM_LIBRARIES "")
endif()

if (EMSCRIPTEN)
	qf_wasm_strip_common_sources(SND_OPENAL_SOURCES)
endif()

add_library(snd_openal ${QFUSION_LIBRARY_TYPE} ${SND_OPENAL_HEADERS} ${SND_OPENAL_SOURCES})
target_link_libraries(snd_openal PRIVATE ogg vorbis vorbisfile OpenAL::OpenAL ${SND_OPENAL_PLATFORM_LIBRARIES} Tracy::TracyClient)
qf_set_output_dir(snd_openal libs)
