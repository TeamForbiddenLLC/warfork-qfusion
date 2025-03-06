#define DEVICE_SUPPORT_VULKAN

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
#define DEVICE_IMPL_MUTLI 1
#else
#define DEVICE_IMPL_MUTLI 0
#endif


#if(DEVICE_IMPL_MUTLI)

#if(DEVICE_IMPL_VULKAN)
  #define GPU_VULKAN_SELECTED(backend) (backend->api == DEVICE_API_VK)
  #define GPU_VULKAN_BLOCK(backend, block) if(backend->api == DEVICE_API_VK) { Q_UNPAREN(block) }
#else
  #define GPU_VULKAN_SELECTED(backend) (false)
  #define GPU_VULKAN_BLOCK(backend, block) 
#endif

#if(DEVICE_IMPL_D3D11 )
  #define GPU_D3D11_BLOCK(backend,  block) if(backend->api == DEVICE_API_D3D11) { Q_UNPAREN(block) }
#else
  #define GPU_D3D11_BLOCK(backend,  block) 
#endif

#if(DEVICE_IMPL_D3D12 )
  #define GPU_D3D12_BLOCK(backend,  block) if(backend->api == DEVICE_API_D3D12) { Q_UNPAREN(block) }
#else
  #define GPU_D3D12_BLOCK(backend,  block) 
#endif

#if(DEVICE_IMPL_MTL )
  #define GPU_MTL_BLOCK(backend, block) if(backend->api == DEVICE_API_MTL) { Q_UNPAREN(block) }
#else
  #define GPU_MTL_BLOCK(backend, block) 
#endif

#else

#if(DEVICE_IMPL_VULKAN)
  #define GPU_VULKAN_SELECTED(backend) (true)
  #define GPU_VULKAN_BLOCK(backend, block)  { assert(backend->api == RI_DEVICE_API_VK);  Q_UNPAREN(block) }
#else
  #define GPU_VULKAN_SELECTED(backend) (false)
  #define GPU_VULKAN_BLOCK(backend, block) 
#endif

#if(DEVICE_IMPL_D3D11 )
  #define GPU_D3D11_BLOCK(backend, block)  { Q_UNPAREN(block) }
#else
  #define GPU_D3D11_BLOCK(backend, device, block) 
#endif

#if(DEVICE_IMPL_D3D12 )
  #define GPU_D3D12_BLOCK(backend, block) { Q_UNPAREN(block)}
#else
  #define GPU_D3D12_BLOCK(backend, device, block) 
#endif

#if(DEVICE_IMPL_MTL )
  #define GPU_MTL_BLOCK(backend, block) { Q_UNPAREN(block) }
#else
  #define GPU_MTL_BLOCK(backend, device, block) 
#endif

#endif

