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
	"../ref_base/*.h"
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
# "." last so shared ref_base/*.c sources can resolve this renderer's own r_local.h
target_include_directories(ref_nri PRIVATE ${STB_INCLUDE_DIR} "../ref_base" ".")
target_include_directories(ref_nri PRIVATE ${MINIZ_INCLUDE_DIR})
# glslang is the GLSL->SPIR-V front on every platform (SPIR-V is cross-compiled to MSL on macOS).
target_link_libraries(ref_nri PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper qcore Tracy::TracyClient)

if (APPLE)
  # Native Metal backend (DEVICE_SUPPORT_MTL, the macOS default in ri_defines.h). The vendored metal-c
  # bindings compile straight into ref_nri and pull in the Apple frameworks they wrap. No volk/VMA/MoltenVK.
  set(METAL_C_SOURCES
    metal-c/src/internal.c
    metal-c/src/foundation.c
    metal-c/src/metal.c
    metal-c/src/quartzcore.c)
  target_sources(ref_nri PRIVATE ${METAL_C_SOURCES})
  target_include_directories(ref_nri PRIVATE "metal-c/include" "metal-c/src")
  # metal-c's selector cache is a GNU statement expression; those TUs need GNU C extensions.
  set_source_files_properties(${METAL_C_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=gnu11")
  target_link_libraries(ref_nri PRIVATE "-framework Metal" "-framework QuartzCore" "-framework Foundation" objc)
  # SPIR-V -> MSL. r_program.c uses the C API (spirv_cross_c.h); spirv-cross-c pulls in core/glsl/msl.
  target_link_libraries(ref_nri PRIVATE spirv-cross-c)
  target_include_directories(ref_nri PRIVATE "../extern/SPIRV-Cross")
else ()
  target_include_directories(ref_nri PRIVATE "${NRI_DIR}/External/vulkan/include")
  target_link_libraries(ref_nri PRIVATE volk VulkanMemoryAllocator)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_compile_definitions (ref_nri PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif ()
qf_set_output_dir(ref_nri libs)

