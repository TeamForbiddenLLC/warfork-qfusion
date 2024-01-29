#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#define NRI_STATIC_LIBRARY 1
#include "NRI.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRISwapChain.h"
#include "Extensions/NRIWrapperVK.h"

#include "vulkan/vulkan.h"

const static NriSwapChainFormat DefaultSwapchainFormat = NriSwapChainFormat_BT709_G22_8BIT;
const static NriSPIRVBindingOffsets DefaultBindingOffset = {100, 200, 300, 400}; // just ShaderMake defaults for simplicity

#define R_VK_ABORT_ON_FAILURE(result) \
	if(result != VK_SUCCESS) { \
		Com_Printf("invalid result: %d", result); \
		exit(0); \
	}

#define NRI_ABORT_ON_FAILURE( result )  \
	if( result != NriResult_SUCCESS ) { \
		exit( 0 );                      \
	}

#define COMMAND_BUFFER_COUNT 2

static const char *NriResultToString[NriResult_MAX_NUM] = { [NriResult_SUCCESS] = "SUCESS",
															[NriResult_FAILURE] = "FAILURE",
															[NriResult_INVALID_ARGUMENT] = "INVALID_ARGUMENT",
															[NriResult_OUT_OF_MEMORY] = "OUT_OF_MEMORY",
															[NriResult_UNSUPPORTED] = "UNSUPPORTED",
															[NriResult_DEVICE_LOST] = "DEVICE_LOST",
															[NriResult_OUT_OF_DATE] = "OUT_OF_DATE" };

typedef struct {
	NriGraphicsAPI api;
	NriHelperInterface helperI;
	NriCoreInterface coreI;
	NriSwapChainInterface swapChainI;
	NriWrapperVKInterface wrapperVKI;
	NriDevice *device;
  
	NriCommandQueue* graphicsCommandQueue;

  union {
  	struct {
  		void* vulkanLoader;
  		VkDevice device;
  		VkInstance instance;
  	} vk;
  };  
} nri_backend_t;

typedef struct {
	NriGraphicsAPI api;
	bool enableApiValidation;
	bool enableNriValidation;
} nri_init_desc_t; 

bool R_InitNriBackend(const nri_init_desc_t* init, nri_backend_t* backend);
void R_NRI_CallbackMessage(NriMessage msg, const char* file, uint32_t line, const char* message, void* userArg);
#endif
