#include "r_nri.h"
#include "../gameshared/q_arch.h"
#include "Extensions/NRIWrapperVK.h"
#include "NRIDescs.h"
#include "r_local.h"

#include "stb_ds.h"

#define VK_USE_PLATFORM_XLIB_KHR 1
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>
#include "vulkan/vulkan.h"
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_wayland.h>

#if defined( _WIN32 )

#include <windows.h>
static void *Sys_Library_Open( const char *name )
{
	return (void *)LoadLibrary( name );
}
static void *Sys_Library_ProcAddress( void *lib, const char *apifuncname )
{
	return (void *)GetProcAddress( (HINSTANCE)lib, apifuncname );
}

#else

#include <dlfcn.h>
static void *Sys_Library_Open( const char *name )
{
	return dlopen( name, RTLD_NOW );
}

static void *Sys_Library_ProcAddress( void *lib, const char *apifuncname )
{
	return (void *)dlsym( lib, apifuncname );
}

#endif

void R_NRI_CallbackMessage(NriMessage msg, const char* file, uint32_t line, const char* message, void* userArg) {
  switch(msg) {
    case NriMessage_TYPE_INFO:
      Com_Printf(S_COLOR_BLACK "%s", message);
      break;
    case NriMessage_TYPE_WARNING:
      Com_Printf(S_COLOR_YELLOW "%s", message);
      break;
    case NriMessage_TYPE_ERROR:
      Com_Printf(S_COLOR_RED "%s", message);
      break;
    default:
      assert(false);
      break;
  }
}

static const char* R_PhysicalDeviceTypeToString(VkPhysicalDeviceType deviceType) {
	switch( deviceType ) {
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			return "Other GPU";
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			return "CPU";
		default:
			break;
	}
  return "Unknown";
}

bool R_InitNriBackend(const nri_init_desc_t* init, nri_backend_t* backend) {

  backend->api = init->api;
  switch(init->api) {
	  case NriGraphicsAPI_VULKAN: {
#if defined( _WIN32 )
		  static const char *instanceExtensions[] = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME };
#else
		  static const char *instanceExtensions[] = { 
		      VK_KHR_SURFACE_EXTENSION_NAME, 
		      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, 
		      VK_KHR_XLIB_SURFACE_EXTENSION_NAME 
		  };
#endif
		  const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME};
		  const char *layers[] = { "VK_LAYER_KHRONOS_validation" };

		  backend->vk.vulkanLoader = Sys_Library_Open( "libvulkan.so" );

		  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkGetInstanceProcAddr" );
		  PFN_vkCreateInstance vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr( VK_NULL_HANDLE, "vkCreateInstance" );
		  PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkEnumeratePhysicalDevices" );
		  PFN_vkCreateDevice vkCreateDevice = (PFN_vkCreateDevice)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkCreateDevice" );
		  PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkGetPhysicalDeviceFeatures2" );
		  PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkGetPhysicalDeviceProperties" );
		  PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)Sys_Library_ProcAddress( backend->vk.vulkanLoader, "vkGetPhysicalDeviceQueueFamilyProperties" );

		  VkApplicationInfo applicationInfo = {};
		  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		  applicationInfo.apiVersion = VK_API_VERSION_1_3;

		  VkInstanceCreateInfo instanceCreateInfo = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
													  .pApplicationInfo = &applicationInfo,
													  .ppEnabledExtensionNames = instanceExtensions,
													  .enabledExtensionCount = Q_ARRAY_COUNT( instanceExtensions ),
													  .ppEnabledLayerNames = layers,
													  .enabledLayerCount = init->enableApiValidation ? 1 : 0 };

		  VkResult vkResult = vkCreateInstance( &instanceCreateInfo, NULL, &backend->vk.instance );
		  if( vkResult != VK_SUCCESS ) {
			  ri.Com_Error( ERR_FATAL, "Failed to create device: %d", vkResult );
		  }

		  uint32_t physicalDeviceNum = 0;
		  vkEnumeratePhysicalDevices( backend->vk.instance, &physicalDeviceNum, NULL );
		  if( physicalDeviceNum == 0 ) {
			  ri.Com_Error( ERR_FATAL, "failed to find physical device" );
		  }

		  assert( physicalDeviceNum != 0 );
		  VkPhysicalDevice *physicalDevices = NULL;
		  arrsetlen( physicalDevices, physicalDeviceNum );
		  vkEnumeratePhysicalDevices( backend->vk.instance, &physicalDeviceNum, physicalDevices );

		  for( size_t i = 0; i < arrlen( physicalDevices ); i++ ) {
			  VkPhysicalDeviceProperties properties;
			  vkGetPhysicalDeviceProperties( physicalDevices[i], &properties );
			  ri.Com_Printf( "GPU[%d]: device: %s type: %s \n", i, properties.deviceName, R_PhysicalDeviceTypeToString( properties.deviceType ) );
		  }
		  ri.Com_Printf("selectin GPU: 0 \n");
		  const VkPhysicalDevice physicalDevice = physicalDevices[0];
		  
		  uint32_t queueFamilyCount = 0;
		  vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, NULL);

		  VkQueueFamilyProperties *familyProperites = NULL;
		  arrsetlen( familyProperites, queueFamilyCount );
		  vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, familyProperites);

      //
		  const float priority = 1.0f;
		  uint32_t queueFamilyIndecies[1] = {};
		  VkDeviceQueueCreateInfo queueCreateInfo = {
		      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		      .pQueuePriorities = &priority,
		      .queueCount = 1,
		      .queueFamilyIndex = queueFamilyIndecies[0],
		  };
		  
		  for( size_t i = 0; i < queueFamilyCount; i++ ) {
        const bool graphics = familyProperites[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        const bool compute = familyProperites[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        const bool copy = familyProperites[i].queueFlags & VK_QUEUE_TRANSFER_BIT;

		    ri.Com_Printf( "Queue[%d]:%s%s%s \n", i, graphics ? " VK_QUEUE_GRAPHICS_BIT": "", compute ? " VK_QUEUE_COMPUTE_BIT":"", copy ? " VK_QUEUE_TRANSFER_BIT": "" );
		  }
		  arrfree(familyProperites); 


		  VkPhysicalDeviceVulkan11Features featuresVulkan11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		  VkPhysicalDeviceVulkan12Features featuresVulkan12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		  VkPhysicalDeviceVulkan13Features featuresVulkan13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

		  VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		  deviceFeatures2.pNext = &featuresVulkan11;
		  featuresVulkan11.pNext = &featuresVulkan12;
		  featuresVulkan12.pNext = &featuresVulkan13;

		  vkGetPhysicalDeviceFeatures2( physicalDevice, &deviceFeatures2 );

		  VkDeviceCreateInfo deviceCreateInfo = {};
		  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		  deviceCreateInfo.pNext = &deviceFeatures2;
		  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		  deviceCreateInfo.queueCreateInfoCount = 1;
		  deviceCreateInfo.enabledExtensionCount = Q_ARRAY_COUNT( deviceExtensions );
		  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

		  vkResult = vkCreateDevice( physicalDevice, &deviceCreateInfo, NULL, &backend->vk.device );
		  if(vkResult  != VK_SUCCESS) {
        ri.Com_Error(ERR_FATAL, "Failed to create device: %d", vkResult);
		  }

		  NriDeviceCreationVKDesc deviceDesc = {
			  .vkInstance = (NRIVkInstance)backend->vk.instance,
			  .vkPhysicalDevices = (NRIVkPhysicalDevice)&physicalDevice,
			  .deviceGroupSize = 1,
			  .queueFamilyIndices = queueFamilyIndecies,
			  .queueFamilyIndexNum = Q_ARRAY_COUNT(queueFamilyIndecies),
			  .vkDevice = (NRIVkDevice)backend->vk.device,
			  .enableNRIValidation = true,
			  .spirvBindingOffsets = DefaultBindingOffset,
			  .callbackInterface = {
			    .MessageCallback = R_NRI_CallbackMessage
			  }
		  };
		  NriResult nriResult = nriCreateDeviceFromVkDevice( &deviceDesc, &backend->device );
		  if(nriResult != NriResult_SUCCESS) {
        ri.Com_Error(ERR_FATAL, "Failed to create NRI device");
		  }
		  arrfree(physicalDevices);
		  break;
	  }
	  default:
		  assert( 0 );
		  break;
  }

  NriResult nriResult = nriGetInterface( backend->device, NRI_INTERFACE( NriCoreInterface ), &backend->coreI );
  if( nriResult != NriResult_SUCCESS ) {
	  return false;
  }
  nriResult = nriGetInterface( backend->device, NRI_INTERFACE( NriHelperInterface ), &backend->helperI );
  if( nriResult != NriResult_SUCCESS ) {
	  return false;
  }
  nriResult = nriGetInterface( backend->device, NRI_INTERFACE( NriSwapChainInterface ), &backend->swapChainI );
  if( nriResult != NriResult_SUCCESS ) {
	  return false;
  }
  nriResult = nriGetInterface( backend->device, NRI_INTERFACE( NriWrapperVKInterface ), &backend->wrapperVKI );
  if( nriResult != NriResult_SUCCESS ) {
	  return false;
  }
  nriResult = backend->coreI.GetCommandQueue( backend->device, NriCommandQueueType_GRAPHICS, &backend->graphicsCommandQueue) ;
  if( nriResult != NriResult_SUCCESS ) {
	  return false;
  }

  return true;
}
