#include "r_renderer.h"
#include "r_local.h"

rserr_t initNRIRenderer( const renderer_desc_t *desc, renderer_t *rend )
{
	NriDeviceCreationDesc deviceCreationDesc = {};
	switch( desc->backend ) {
		case BACKEND_NRI_VULKAN:
			deviceCreationDesc.graphicsAPI = NriGraphicsAPI_VULKAN;
			break;
		default:
			assert( false );
			break;
	}
  NriResult result;
  result = nriCreateDevice( &deviceCreationDesc, &rend->nri.device); 
	if(result != NriResult_SUCCESS) {
	  Com_Printf("failed to initialize device %s", NriResultToString[result]);
		goto fail;
	}
	result = nriGetInterface( rend->nri.device, NRI_INTERFACE( NriCoreInterface ), &rend->nri.coreI );
	if (result != NriResult_SUCCESS) { goto fail_free_device; }
	result = nriGetInterface( rend->nri.device, NRI_INTERFACE( NriHelperInterface ), &rend->nri.helperI); 
	if (result != NriResult_SUCCESS) { goto fail_free_device; }
	result = nriGetInterface( rend->nri.device, NRI_INTERFACE( NriSwapChainInterface ), &rend->nri.swapChainI );
	if (result != NriResult_SUCCESS) { goto fail_free_device; }
	return true;
fail_free_device:
	nriDestroyDevice( rend->nri.device );
fail:
	return false;
}
