project(ref_nri)

if (USE_SDL2)
	include_directories(${SDL2_INCLUDE_DIR})
endif()

file(GLOB REF_NRI_HEADERS
	"*.h"
	"../gameshared/q_*.h"
	"../gameshared/anorms.h"
	"../gameshared/config.h"
	"../qcommon/bsp.h"
	"../qcommon/patch.h"
	"../qcommon/iqm.h"
	"../qcommon/qfiles.h"
	"../qalgo/*.h"
	"${REF_BASE_DIR}/*.h"
	)

file(GLOB REF_NRI_COMMON_SOURCES
	"*.c"
	"../ref_base/*.c"
	"../ref_base/*.cpp"
	"*.cpp"
	"../gameshared/q_*.c"
	"../qalgo/*.c"
	"../qcommon/bsp.c"
	"../qcommon/patch.c"
	)

file(GLOB STB_HEADERS
    "${STB_INCLUDE_DIR}/stb_image.h"
    "${STB_INCLUDE_DIR}/stb_image_write.h"
)

if (USE_SDL2)
	file(GLOB REF_NRI_PLATFORM_SOURCES
		"../sdl/sdl_glw.c"
		"../sdl/sdl_glw_icon.c"
		"../sdl/sdl_qgl.c"
		"../sdl/sdl_win.c"
		)

	set(REF_NRI_PLATFORM_LIBRARIES ${SDL2_LIBRARY})
else()
	if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		file(GLOB REF_NRI_PLATFORM_SOURCES
			"../win32/win_glw2.c"
			"../win32/win_glw.c"
			"../win32/win_qgl.c"
			)

		set(REF_NRI_PLATFORM_LIBRARIES "")
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		file(GLOB REF_NRI_PLATFORM_SOURCES
			"../unix/unix_glw.c"
			"../unix/unix_qgl.c"
			"../unix/unix_xpm.c"
			)

		set(REF_NRI_PLATFORM_LIBRARIES "X11" "Xext" "Xinerama" "Xrandr" "Xxf86vm" "m")
	endif()
endif()

add_library(ref_nri SHARED ${REF_NRI_HEADERS} ${REF_NRI_COMMON_SOURCES} ${REF_NRI_PLATFORM_SOURCES})
target_include_directories(ref_nri PRIVATE ${STB_INCLUDE_DIR} "../ref_base" "${NRI_DIR}/External/vulkan/include")
target_link_libraries(ref_nri PRIVATE NRI)
target_link_libraries(ref_nri PRIVATE OSDependent glslang MachineIndependent GenericCodeGen glslang-default-resource-limits SPVRemapper SPIRV qcore)
target_link_libraries(ref_nri PRIVATE ${REF_NRI_PLATFORM_LIBRARIES})
target_include_directories(ref_nri PRIVATE ${NRI_INCLUDE_DIR})
target_include_directories(ref_nri PRIVATE ${MINIZ_INCLUDE_DIR})
qf_set_output_dir(ref_nri libs)

