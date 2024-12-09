project(cin)

include_directories(${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

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

add_library(cin SHARED ${CIN_SOURCES} ${CIN_HEADERS})
target_link_libraries(cin PRIVATE Ogg::ogg Vorbis::vorbis Vorbis::vorbisfile ${CIN_PLATFORM_LIBRARIES})
qf_set_output_dir(cin libs)
