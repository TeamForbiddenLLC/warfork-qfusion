#include "r_nri.h"
#include "r_local.h"

#include "stb_ds.h"


static void nri_CallbackMessage(NriMessage msg, const char* file, uint32_t line, const char* message, void* userArg) {
  switch(msg) {
    case NriMessage_TYPE_INFO:
      Com_Printf(S_COLOR_BLACK "NRI: %s (%s:%d)", message, file, line);
      break;
    case NriMessage_TYPE_WARNING:
      Com_Printf(S_COLOR_YELLOW "NRI: %s (%s:%d)", message, file, line);
      break;
    case NriMessage_TYPE_ERROR:
      Com_Printf(S_COLOR_RED "NRI: %s (%s:%d)", message, file, line);
      break;
    default:
      assert(false);
      break;
  }
}

static void nri_BuildSwapchain( NriSwapChainDesc *swapChainDesc )
{
	assert(r_nri.swapChain == NULL);

	NRI_ABORT_ON_FAILURE( r_nri.swapChainI.CreateSwapChain( r_nri.device, swapChainDesc, &r_nri.swapChain ) );
	uint32_t textureNums = 0;
	NriTexture *const *swapChainTextures = r_nri.swapChainI.GetSwapChainTextures( r_nri.swapChain, &textureNums );
	const NriTextureDesc *swapChainTextureDesc = r_nri.coreI.GetTextureDesc( swapChainTextures[0] );

	for( size_t i = 0; i < textureNums; i++ ) {
		NriTexture2DViewDesc textureViewDesc = { swapChainTextures[i], NriTexture2DViewType_COLOR_ATTACHMENT, swapChainTextureDesc->format };
		NriDescriptor *colorAttachment;
		NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateTexture2DView( &textureViewDesc, &colorAttachment ) );
		nri_back_buffer_t backBuffer = { .colorAttachment = colorAttachment, .texture = swapChainTextures[i] };
		arrpush( r_nri.backbuffers, backBuffer );
	}
}

bool nri_Init( nri_desc_t *desc )
{
  memset(&r_nri, 0,sizeof(r_nri));

	NriDeviceCreationDesc deviceCreationDesc = {
	  .callbackInterface = {
	    .MessageCallback = nri_CallbackMessage 
	  },
	  .enableAPIValidation = true,
	  .enableNRIValidation = true
	};
	switch( r_backend_api ) {
		case BACKEND_NRI_VULKAN:
			deviceCreationDesc.graphicsAPI = NriGraphicsAPI_VULKAN;
			break;
		default:
			assert( false );
			break;
	}
	NriResult result;
	result = nriCreateDevice( &deviceCreationDesc, &r_nri.device );
	if( result != NriResult_SUCCESS ) {
		Com_Printf( "failed to initialize device %s", NriResultToString[result] );
		goto fail;
	}
	result = nriGetInterface( r_nri.device, NRI_INTERFACE( NriCoreInterface ), &r_nri.coreI );
	if( result != NriResult_SUCCESS ) {
		goto fail_free_device;
	}
	result = nriGetInterface( r_nri.device, NRI_INTERFACE( NriHelperInterface ), &r_nri.helperI );
	if( result != NriResult_SUCCESS ) {
		goto fail_free_device;
	}
	result = nriGetInterface( r_nri.device, NRI_INTERFACE( NriSwapChainInterface ), &r_nri.swapChainI );
	if( result != NriResult_SUCCESS ) {
		goto fail_free_device;
	}

	NRI_ABORT_ON_FAILURE( r_nri.coreI.GetCommandQueue( r_nri.device, NriCommandQueueType_GRAPHICS, &r_nri.graphicsQueue ) );
	r_nri.window = desc->window;

	NriSwapChainDesc swapChainDesc = {};
	swapChainDesc.window = desc->window;
	swapChainDesc.commandQueue = r_nri.graphicsQueue;
	swapChainDesc.format = DefaultSwapchainFormat;
	swapChainDesc.verticalSyncInterval = 0;
	swapChainDesc.width = desc->width;
	swapChainDesc.height = desc->height;
	swapChainDesc.textureNum = 3;
	nri_BuildSwapchain( &swapChainDesc );
	NRI_ABORT_ON_FAILURE( r_nri.coreI.CreateFence( r_nri.device, 0, &r_nri.frameFence ) );

	return true;
fail_free_device:
	nriDestroyDevice( r_nri.device );
fail:
	return false;
}

void nri_resizeSwapChain( uint16_t width, uint16_t height )
{
	NriCommandQueue *commandQueue;
	NRI_ABORT_ON_FAILURE( r_nri.coreI.GetCommandQueue( r_nri.device, NriCommandQueueType_GRAPHICS, &commandQueue ) );
	NRI_ABORT_ON_FAILURE( r_nri.helperI.WaitForIdle( commandQueue ) );
	for( size_t i = 0; i < arrlen( r_nri.backbuffers ); i++ ) {
		r_nri.coreI.DestroyDescriptor( r_nri.backbuffers[i].colorAttachment );
	}
	r_nri.swapChainI.DestroySwapChain( r_nri.swapChain );
	arrsetcap( r_nri.backbuffers, 0 );

	NriSwapChainDesc swapChainDesc = {};
	swapChainDesc.window = r_nri.window;
	swapChainDesc.commandQueue = r_nri.graphicsQueue;
	swapChainDesc.format = DefaultSwapchainFormat;
	swapChainDesc.verticalSyncInterval = 0;
	swapChainDesc.width = width;
	swapChainDesc.height = height;
	swapChainDesc.textureNum = 3;
	nri_BuildSwapchain( &swapChainDesc );
}
