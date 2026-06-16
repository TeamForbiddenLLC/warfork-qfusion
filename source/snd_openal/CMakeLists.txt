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

add_library(snd_openal SHARED ${SND_OPENAL_HEADERS} ${SND_OPENAL_SOURCES})
# OpenAL is loaded at runtime (OPENAL_RUNTIME, see gameshared/q_arch.h): QAL_Init resolves
# the entry points via dlopen/LoadLibrary, so we must NOT link it here (that would add a
# load-time dependency on the library and defeat the graceful "no sound" fallback when it
# is missing). We still need its public headers to compile, and we must ensure it is built
# so it can be deployed next to the executable.
target_link_libraries(snd_openal PRIVATE ogg vorbis vorbisfile ${SND_OPENAL_PLATFORM_LIBRARIES} Tracy::TracyClient)
target_include_directories(snd_openal PRIVATE $<TARGET_PROPERTY:OpenAL::OpenAL,INTERFACE_INCLUDE_DIRECTORIES>)
if (NOT USE_SYSTEM_OPENAL)
    add_dependencies(snd_openal OpenAL)
endif()
qf_set_output_dir(snd_openal libs)
