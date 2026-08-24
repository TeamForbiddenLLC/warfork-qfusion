// Backend selection is compile-time and mutually exclusive (see the #error below). Pick a platform
// default only when the consumer has not forced one on the command line (e.g. -DDEVICE_SUPPORT_VULKAN
// to run MoltenVK on macOS for A/B testing). macOS renders natively on Metal; everything else Vulkan.
#if !defined( DEVICE_SUPPORT_VULKAN ) && !defined( DEVICE_SUPPORT_MTL ) && !defined( DEVICE_SUPPORT_D3D11 ) && !defined( DEVICE_SUPPORT_D3D12 )
	#if defined( __APPLE__ )
		#define DEVICE_SUPPORT_MTL
	#else
		#define DEVICE_SUPPORT_VULKAN
	#endif
#endif

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

#if ( ( DEVICE_IMPL_D3D12 + DEVICE_IMPL_D3D11 + DEVICE_IMPL_MTL + DEVICE_IMPL_VULKAN ) > 1 )
  #error MULTIPLE APIs UNSUPPORTED 
#else
#define DEVICE_IMPL_MUTLI 0
#endif


