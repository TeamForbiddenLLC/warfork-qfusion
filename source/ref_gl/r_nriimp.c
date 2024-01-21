#include "r_nriimp.h"
#include "r_local.h"

static void NRIimp_CallbackMessage(NriMessage msg, const char* file, uint32_t line, const char* message, void* userArg) {
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

bool NRIimp_Init( nri_desc_t *desc )
{

	NriDeviceCreationDesc deviceCreationDesc = {
	  .callbackInterface = {
	    .MessageCallback = NRIimp_CallbackMessage 
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
	return true;
fail_free_device:
	nriDestroyDevice( r_nri.device );
fail:
	return false;
}
