#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#define NRI_STATIC_LIBRARY 1
#include "NRI.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRISwapChain.h"

#include "r_renderer.h"

const static NriSwapChainFormat DefaultSwapchainFormat = NriSwapChainFormat_BT709_G22_8BIT; 

#define NRI_ABORT_ON_FAILURE(result) \
    if (result != NriResult_SUCCESS) { exit(0);}

#define COMMAND_BUFFER_COUNT 2 

static const char* NriResultToString[NriResult_MAX_NUM] = {
    [NriResult_SUCCESS]= "SUCESS",
    [NriResult_FAILURE]= "FAILURE",
    [NriResult_INVALID_ARGUMENT]= "INVALID_ARGUMENT",
    [NriResult_OUT_OF_MEMORY]= "OUT_OF_MEMORY",
    [NriResult_UNSUPPORTED]= "UNSUPPORTED",
    [NriResult_DEVICE_LOST] = "DEVICE_LOST",
    [NriResult_OUT_OF_DATE] = "OUT_OF_DATE"
};
typedef struct {
    NriDescriptor* colorAttachment;
    NriTexture* texture;
} nri_back_buffer_t;

typedef struct {
    NriCommandAllocator* alloc;
    NriCommandBuffer* cmd;
    
} nri_frame_t;

typedef struct {
	NriHelperInterface helperI;
	NriCoreInterface coreI;
	NriSwapChainInterface swapChainI;
	NriDevice *device;

    NriCommandQueue* graphicsQueue;

    
	NriWindow window;
	NriSwapChain *swapChain;
	
	uint32_t frameIdx;
    NriFence* frameFence;
	nri_back_buffer_t* backbuffers;
	nri_frame_t frames[COMMAND_BUFFER_COUNT]; 
} nri_t;

extern nri_t r_nri;

typedef struct {
  NriWindow window;
  uint16_t width;
  uint16_t height;
} nri_desc_t;

typedef struct {
} nri_swap_chain_desc_t;

bool nri_Init(nri_desc_t* desc);
void nri_resizeSwapChain(uint16_t width, uint16_t height);

#endif
