// Backend selection is compile-time. At least one DEVICE_SUPPORT_* macro must arrive from the build
// system: the top-level CMake RI_BACKEND cache variable (MTL on macOS, VULKAN elsewhere by default;
// override with e.g. -DRI_BACKEND=VULKAN to run MoltenVK on macOS). There is no header-side platform
// fallback -- see the #error check at the bottom. More than one backend sets DEVICE_IMPL_MUTLI.

#ifdef DEVICE_SUPPORT_VULKAN
#define VK_NO_PROPERTIES
#define DEVICE_IMPL_VULKAN 1
#else
#define DEVICE_IMPL_VULKAN 0
#endif

#ifdef DEVICE_SUPPORT_MTL
#define DEVICE_IMPL_MTL 1
#else
#define DEVICE_IMPL_MTL 0
#endif

#ifdef DEVICE_SUPPORT_D3D11
#define DEVICE_IMPL_D3D11 1
#else
#define DEVICE_IMPL_D3D11 0
#endif

#ifdef DEVICE_SUPPORT_D3D12
#define DEVICE_IMPL_D3D12 1
#else
#define DEVICE_IMPL_D3D12 0
#endif

#define DEVICE_IMPL_COUNT ( DEVICE_IMPL_D3D12 + DEVICE_IMPL_D3D11 + DEVICE_IMPL_MTL + DEVICE_IMPL_VULKAN )
#if DEVICE_IMPL_COUNT == 0
  #error NO GPU BACKEND SELECTED (set RI_BACKEND in CMake)
#endif

// 1 when more than one backend is compiled in. RIIsTargetSelected then dispatches on the runtime
// backend; with a single backend it folds to a compile-time constant.
#define DEVICE_IMPL_MUTLI ( DEVICE_IMPL_COUNT > 1 )
