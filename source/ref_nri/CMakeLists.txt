project(ref_nri)

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
	"*.cpp"
	"../ref_base/*.c"
	"../ref_base/*.cpp"
	"../gameshared/q_*.c"
	"../qalgo/*.c"
	"../qcommon/bsp.c"
	"../qcommon/patch.c"
	)

file(GLOB STB_HEADERS
    "${STB_INCLUDE_DIR}/stb_image.h"
    "${STB_INCLUDE_DIR}/stb_image_write.h"
)

add_library(ref_nri SHARED ${REF_NRI_HEADERS} ${REF_NRI_COMMON_SOURCES} ${REF_NRI_PLATFORM_SOURCES})
target_include_directories(ref_nri PRIVATE ${STB_INCLUDE_DIR} "../ref_base" "${NRI_DIR}/External/vulkan/include")
target_link_libraries(ref_nri PRIVATE volk)
target_link_libraries(ref_nri PRIVATE VulkanMemoryAllocator)
target_link_libraries(ref_nri PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper qcore volk Tracy::TracyClient)
target_include_directories(ref_nri PRIVATE ${MINIZ_INCLUDE_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_compile_definitions (ref_nri PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_compile_definitions (ref_nri PRIVATE VK_USE_PLATFORM_METAL_EXT VK_ENABLE_BETA_EXTENSIONS)
else ()
endif ()
qf_set_output_dir(ref_nri libs)

