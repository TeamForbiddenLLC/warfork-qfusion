/*
Copyright (C) 2016 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_frame_cmd_buffer.h"
#include "r_image.h"
#include "r_local.h"
#include "r_frontend.h"
#include "r_capture.h"

#include "ri_resource_upload.h"
#include "ri_types.h"
#include "ri_conversion.h"
#include "ri_swapchain.h"
#include "ri_renderer.h"
#include "ri_vk.h"

#include "stb_ds.h"
#include "tracy/TracyC.h"

static ref_frontend_t rrf;

static void __ShutdownSwapchainTexture();

static void __R_InitVolatileAssets( void )
{
	// init volatile data
	R_InitSkeletalCache();
	R_InitCoronas();
	R_InitCustomColors();

	rsh.envShader = R_LoadShader( "$environment", SHADER_TYPE_OPAQUE_ENV, true );
	rsh.skyShader = R_LoadShader( "$skybox", SHADER_TYPE_SKYBOX, true );
	rsh.whiteShader = R_LoadShader( "$whiteimage", SHADER_TYPE_2D, true );
	rsh.emptyFogShader = R_LoadShader( "$emptyfog", SHADER_TYPE_FOG, true );

	if( !rsh.nullVBO ) {
		rsh.nullVBO = R_InitNullModelVBO();
	}
	else {
		R_TouchMeshVBO( rsh.nullVBO );
	}

	if( !rsh.postProcessingVBO ) {
		rsh.postProcessingVBO = R_InitPostProcessingVBO();
	}
	else {
		R_TouchMeshVBO( rsh.postProcessingVBO );
	}
}

static void __ShutdownSwapchainTexture() {
	if( IsRISwapchainValid( &rsh.riSwapchain ) ) {
		for( uint32_t i = 0; i < rsh.riSwapchain.imageCount; i++ ) {
			for( uint32_t p = 0; p < 2; p++ ) {
				FreeRITexture(&rsh.device, &rsh.pogoTextures[(i * 2) + p]);
				FreeRIDescriptor( &rsh.device, &rsh.pogoBuffer[i].pogoAttachment[p] );
#if ( DEVICE_IMPL_VULKAN )
				vmaFreeMemory( rsh.device.vk.vmaAllocator, rsh.vk.pogoAlloc[( i * 2 ) + p] );
#endif
			}
			FreeRIDescriptor(&rsh.device, &rsh.colorAttachment[i]);
			FreeRIDescriptor(&rsh.device, &rsh.depthAttachment[i]);
			
			FreeRITexture(&rsh.device, &rsh.depthTextures[i]);
#if ( DEVICE_IMPL_VULKAN )
			vmaFreeMemory( rsh.device.vk.vmaAllocator, rsh.vk.depthAlloc[i] );
#endif
		}
		FreeRISwapchain( &rsh.device, &rsh.riSwapchain );

		rsh.vk.swapchainIndex = 0;
	}
}

#if ( DEVICE_IMPL_VULKAN )

static int __VK_OrderPhysicaAdapter( const void *a, const void *b )
{
	const struct RIPhysicalAdapter_s *a1 = a;
	const struct RIPhysicalAdapter_s *b2 = b;
	if( a1->type > b2->type )
		return 1;
	if( a1->type < b2->type )
		return -1;
	if( a1->presetLevel > b2->presetLevel )
		return 1;
	if( a1->presetLevel < b2->presetLevel )
		return -1;
	return a1->videoMemorySize > b2->videoMemorySize;
}

#endif

rserr_t RF_Init( const char *applicationName, const char *screenshotPrefix, int startupColor,
	int iconResource, const int *iconXPM,
	void *hinstance, void *wndproc, void *parenthWnd, 
	bool verbose )
{
	memset( &rsh, 0, sizeof( rsh ) );
	memset( &rf, 0, sizeof( rf ) );
	memset( &rrf, 0, sizeof( rrf ) );
	
	rsh.registrationSequence = 1;
	rsh.registrationOpen = false;

	rsh.worldModelSequence = 1;

	rf.swapInterval = -1;
	rf.speedsMsgLock = ri.Mutex_Create();
	rf.debugSurfaceLock = ri.Mutex_Create();

	r_mempool = R_AllocPool( NULL, "Rendering Frontend" );
	rserr_t err = R_Init( applicationName, screenshotPrefix, startupColor,
		iconResource, iconXPM, hinstance, wndproc, parenthWnd, verbose );
	
	if( !applicationName ) applicationName = "Qfusion";
	if( !screenshotPrefix ) screenshotPrefix = "";
	
	R_WIN_Init(applicationName, hinstance, wndproc, parenthWnd, iconResource, iconXPM);

	struct RIBackendInit_s backendInit = { 0 };
	backendInit.api = RI_DEVICE_API_VK;
	backendInit.applicationName = applicationName;
#ifndef NDEBUG
	backendInit.vk.enableValidationLayer = true;
#else
	backendInit.vk.enableValidationLayer = false;
#endif

	if(InitRIRenderer(&backendInit, &rsh.renderer) != RI_SUCCESS) {
		return rserr_unknown;
	}

	uint32_t numAdapters = 0;
	if( EnumerateRIAdapters( &rsh.renderer, NULL, &numAdapters ) != RI_SUCCESS ) {
		return rserr_unknown;
	}
	assert(numAdapters > 0);
	struct RIPhysicalAdapter_s* physicalAdapters = alloca(sizeof(struct RIPhysicalAdapter_s) * numAdapters);
	if(EnumerateRIAdapters(&rsh.renderer, physicalAdapters, &numAdapters) != RI_SUCCESS) {
		return rserr_unknown;
	}
	//qsort( phyiscalAdapters, sizeof( struct RIPhysicalAdapter_s ), numAdapters, __VK_OrderPhysicaAdapter );
	uint32_t selectedAdapterIdx = 0;
	for( size_t i = 1; i < numAdapters; i++ ) {
		if( physicalAdapters[i].type > physicalAdapters[selectedAdapterIdx].type )
			selectedAdapterIdx = i;
		if( physicalAdapters[i].type < physicalAdapters[selectedAdapterIdx].type )
			continue;

		if( physicalAdapters[i].presetLevel > physicalAdapters[selectedAdapterIdx].presetLevel ) 
			selectedAdapterIdx = i;
		if( physicalAdapters[i].presetLevel < physicalAdapters[selectedAdapterIdx].presetLevel )
			continue;
		
		if(physicalAdapters[i].videoMemorySize > physicalAdapters[selectedAdapterIdx].videoMemorySize) 
			selectedAdapterIdx = i;
	}
	struct RIDeviceDesc_s deviceInit = { 0 };
	deviceInit.physicalAdapter = &physicalAdapters[selectedAdapterIdx];
	InitRIDevice(&rsh.renderer, &deviceInit, &rsh.device );

	rf.applicationName = R_CopyString( applicationName );
	rf.screenshotPrefix = R_CopyString( screenshotPrefix );
	rf.startupColor = startupColor;

	// create vulkan window
	win_init_t winInit = { 
		.backend = VID_WINDOW_VULKAN, 
		.x = vid_xpos->integer, 
		.y = vid_ypos->integer, 
		.width = vid_width->integer,
		.height = vid_height->integer,
	};
	if(!R_WIN_InitWindow(&winInit)) {
		ri.Com_Error(ERR_DROP, "failed to create window" );
		return rserr_unknown;
	}

	rsh.shadowSamplerDescriptor = R_ResolveSamplerDescriptor( IT_DEPTHCOMPARE | IT_SPECIAL | IT_DEPTH );
#if ( DEVICE_IMPL_VULKAN )
	struct RIQueue_s *graphicsQueue = &rsh.device.queues[RI_QUEUE_GRAPHICS];
	{
		VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
		semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
		VK_WrapResult( vkCreateSemaphore( rsh.device.vk.device, &semaphoreCreateInfo, NULL, &rsh.vk.frameSemaphore ) );
	}

	for( size_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++ ) {
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			cmdPoolCreateInfo.queueFamilyIndex = graphicsQueue->vk.queueFamilyIdx;
			cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_WrapResult( vkCreateCommandPool( rsh.device.vk.device, &cmdPoolCreateInfo, NULL, &rsh.frameSets[i].vk.pool ) );
		}
		{
			VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			cmdAllocInfo.commandPool = rsh.frameSets[i].vk.pool;
			cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdAllocInfo.commandBufferCount = 1;
			rsh.frameSets[i].primaryCmd.vk.pool = rsh.frameSets[i].vk.pool;
			VK_WrapResult( vkAllocateCommandBuffers( rsh.device.vk.device, &cmdAllocInfo, &rsh.frameSets[i].primaryCmd.vk.cmd ) );
		}
		{
			VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			cmdAllocInfo.commandPool = rsh.frameSets[i].vk.pool;
			cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
			cmdAllocInfo.commandBufferCount = 1;
			rsh.frameSets[i].backBufferCmd.vk.pool = rsh.frameSets[i].vk.pool;
			VK_WrapResult( vkAllocateCommandBuffers( rsh.device.vk.device, &cmdAllocInfo, &rsh.frameSets[i].backBufferCmd.vk.cmd ) );
		}
	}
#endif
	struct RIScratchAllocDesc_s scratchDesc = { .blockSize = UBOBlockerBufferSize, .alignmentReq = UBOBlockerBufferAlignmentReq, .alloc = RIUniformScratchAllocHandler };
	for( size_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++ ) {
		InitRIScratchAlloc( &rsh.device, &rsh.frameSets[i].uboScratchAlloc, &scratchDesc );
	}

	if( err != rserr_ok ) {
		return err;
	}

	RI_InitResourceUploader( &rsh.device, &rsh.uploader );

	RP_Init();

	R_InitVBO();

	R_InitImages();

	R_InitShaders();

	R_InitCinematics();

	R_InitSkinFiles();

	R_InitModels();

	R_ClearScene();

	__R_InitVolatileAssets();

	R_ClearRefInstStack();

	R_InitDrawLists();

	R_InitShaders();

	return rserr_ok;
}

rserr_t RF_SetMode( int x, int y, int width, int height, int displayFrequency, bool fullScreen, bool stereo )
{
	WaitRIQueueIdle(&rsh.device, &rsh.device.queues[RI_QUEUE_GRAPHICS]);
	TracyCZone(ctx, 1);

	if( fullScreen ) {
		if( !R_WIN_SetFullscreen( displayFrequency, width, height ) ) {
			fullScreen = false;
		}
	}
	if( !fullScreen )
		R_WIN_SetWindowed( x, y, width, height );

	win_handle_t handle = {0};
	if(!R_WIN_GetWindowHandle( &handle ) ) {
		ri.Com_Error(ERR_DROP, "failed to resolve window handle" );
		return rserr_unknown;
	}
	{
		struct RIWindowHandle_s windowHandle = { 0 };
		
		switch( handle.winType ) {
			case VID_WINDOW_WAYLAND:
				windowHandle.type = RI_WINDOW_WAYLAND;
				windowHandle.wayland.surface = handle.window.wayland.surface; 
				windowHandle.wayland.display = handle.window.wayland.display; 
				break;
			case VID_WINDOW_TYPE_X11:
				windowHandle.type = RI_WINDOW_X11;
				windowHandle.x11.window = handle.window.x11.window; 
				windowHandle.x11.dpy = handle.window.x11.dpy; 
				break;
			case VID_WINDOW_WIN32:
				windowHandle.type = RI_WINDOW_WIN32;
				windowHandle.windows.hwnd = handle.window.win.hwnd; 
				break;
			default:
				assert( false );
				break;
		}
		
		__ShutdownSwapchainTexture();
		struct RISwapchainDesc_s swapchainInit = { 0 };
		swapchainInit.windowHandle = &windowHandle;
		swapchainInit.imageCount = 3;
		swapchainInit.queue = &rsh.device.queues[RI_QUEUE_GRAPHICS];
		swapchainInit.width = width;
		swapchainInit.height = height;
		swapchainInit.format = RI_SWAPCHAIN_BT709_G22_8BIT;
		InitRISwapchain(&rsh.device, &swapchainInit, &rsh.riSwapchain);
		rsh.postProcessingSampler = R_ResolveSamplerDescriptor( IT_NOFILTERING );

#if ( DEVICE_IMPL_VULKAN )
		{
			uint32_t queueFamilies[RI_QUEUE_LEN] = { 0 };

			assert( rsh.riSwapchain.imageCount > 0 );
			for( uint32_t i = 0; i < rsh.riSwapchain.imageCount; i++ ) {
				VmaAllocationCreateInfo mem_reqs = { 0 };
				mem_reqs.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
				{
					VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
					info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
					info.imageType = VK_IMAGE_TYPE_2D;
					info.extent.width = rsh.riSwapchain.width;
					info.extent.height = rsh.riSwapchain.height;
					info.extent.depth = 1;
					info.mipLevels = 1;
					info.arrayLayers = 1;
					info.samples = 1;
					info.tiling = VK_IMAGE_TILING_OPTIMAL;
					info.pQueueFamilyIndices = queueFamilies;
					VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
					info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					info.format = RIFormatToVK( POGO_BUFFER_TEXTURE_FORMAT );
					VK_WrapResult( vmaCreateImage( rsh.device.vk.vmaAllocator, &info, &mem_reqs, &rsh.pogoTextures[( i * 2 )].vk.image, &rsh.vk.pogoAlloc[( i * 2 )], NULL ) );
					VK_WrapResult( vmaCreateImage( rsh.device.vk.vmaAllocator, &info, &mem_reqs, &rsh.pogoTextures[( i * 2 ) + 1].vk.image, &rsh.vk.pogoAlloc[( i * 2 ) + 1], NULL ) );
				}
				{
					VkImageViewUsageCreateInfo usageInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
					VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
					createInfo.pNext = &usageInfo;
					createInfo.subresourceRange = (VkImageSubresourceRange){
						VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
					};
					usageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					createInfo.image = rsh.riSwapchain.vk.images[i];
					createInfo.format = RIFormatToVK( rsh.riSwapchain.format );
					createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					rsh.colorAttachment[i].flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
					rsh.colorAttachment[i].texture = &rsh.riSwapchain.textures[i];
					rsh.colorAttachment[i].vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					rsh.colorAttachment[i].vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &rsh.colorAttachment[i].vk.image.imageView ) );
					UpdateRIDescriptor( &rsh.device, &rsh.colorAttachment[i] );

					for( size_t p = 0; p < 2; p++ ) {
						usageInfo.usage = (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
						createInfo.image = rsh.pogoTextures[(i * 2) + p].vk.image;
						createInfo.format = RIFormatToVK( POGO_BUFFER_TEXTURE_FORMAT );
						createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
						( rsh.pogoBuffer + i )->pogoAttachment[p].flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
						( rsh.pogoBuffer + i )->pogoAttachment[p].texture = &rsh.pogoTextures[(i * 2) + p];
						( rsh.pogoBuffer + i )->pogoAttachment[p].vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
						( rsh.pogoBuffer + i )->pogoAttachment[p].vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &( rsh.pogoBuffer + i )->pogoAttachment[p].vk.image.imageView ) );
						UpdateRIDescriptor( &rsh.device, &( rsh.pogoBuffer + i )->pogoAttachment[p] );
					}

				}

				{
					VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
					info.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
					info.imageType = VK_IMAGE_TYPE_2D;
					info.extent.width = rsh.riSwapchain.width;
					info.extent.height = rsh.riSwapchain.height;
					info.extent.depth = 1;
					info.mipLevels = 1;
					info.arrayLayers = 1;
					info.samples = 1;
					info.tiling = VK_IMAGE_TILING_OPTIMAL;
					info.pQueueFamilyIndices = queueFamilies;
					VK_ConfigureImageQueueFamilies( &info, rsh.device.queues, RI_QUEUE_LEN, queueFamilies, RI_QUEUE_LEN );
					info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					info.format = RIFormatToVK( RI_FORMAT_D32_SFLOAT );
					info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					VK_WrapResult( vmaCreateImage( rsh.device.vk.vmaAllocator, &info, &mem_reqs, &rsh.depthTextures[i].vk.image, &rsh.vk.depthAlloc[i], NULL ) );
				}
				{
					VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
					createInfo.format = RIFormatToVK( RI_FORMAT_D32_SFLOAT );
					createInfo.subresourceRange = (VkImageSubresourceRange){
						VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1,
					};
					createInfo.image = rsh.depthTextures[i].vk.image;
					createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; //| VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					
					rsh.depthAttachment[i].flags |= RI_VK_DESC_OWN_IMAGE_VIEW;
					rsh.depthAttachment[i].texture = &rsh.depthTextures[i];
					rsh.depthAttachment[i].vk.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					rsh.depthAttachment[i].vk.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					VK_WrapResult( vkCreateImageView( rsh.device.vk.device, &createInfo, NULL, &rsh.depthAttachment[i].vk.image.imageView ) );
					UpdateRIDescriptor( &rsh.device, &rsh.colorAttachment[i] );
					//RI_VK_InitImageView( &rsh.device, &createInfo, rsh.depthAttachment + i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE );
				}
			}
		}
#endif
	}

	memset( rrf.customColors, 0, sizeof( rrf.customColors ) );

	RB_Init();

  TracyCZoneEnd(ctx);
	return rserr_ok;	
}

rserr_t RF_SetWindow( void *hinstance, void *wndproc, void *parenthWnd )
{
 assert( false );
 return rserr_ok;
}

void RF_AppActivate( bool active, bool destroy )
{
}

void RF_Shutdown( bool verbose )
{
	//RF_AdapterShutdown( &rrf.adapter );
	memset( &rrf, 0, sizeof( rrf ) );
	WaitRIQueueIdle(&rsh.device, &rsh.device.queues[RI_QUEUE_GRAPHICS]);
	WaitRIQueueIdle(&rsh.device, &rsh.device.queues[RI_QUEUE_COPY]);

	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "envshot" );
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "gfxinfo" );
	Cmd_RemoveCommand( "shaderdump" );
	Cmd_RemoveCommand( "shaderlist" );
	Cmd_RemoveCommand( "glslprogramlist" );
	Cmd_RemoveCommand( "cinlist" );

	// free shaders, models, etc.

	// kill volatile data
	RI_FreeResourceUploader(&rsh.device, &rsh.uploader);
	
	R_DestroyVolatileAssets();

	R_ShutdownModels();

	R_ShutdownSkinFiles();

	R_ShutdownVBO();

	R_ShutdownShaders();

	R_ShutdownCinematics();

	R_ShutdownImages();

	R_ShutdownPortals();
	
	R_ShutdownShadows();
	
	RP_Shutdown();

	RB_Shutdown();

	//R_ExitResourceUpload();

	R_DisposeScene(&rsc);


	for( size_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++ ) {
		struct r_frame_set_s *frameSet = &rsh.frameSets[i];
		FreeRIScratchAlloc( &rsh.device, &frameSet->uboScratchAlloc );
		for( size_t i = 0; i < arrlen( frameSet->freeList ); i++ ) {
			FreeRIFree( &rsh.device, &frameSet->freeList[i] );
		}
		for( size_t i = 0; i < arrlen( frameSet->secondaryCmd ); i++ ) {
			FreeRICmd( &rsh.device, &frameSet->secondaryCmd[i] );
		}
		FreeRICmd( &rsh.device, &frameSet->primaryCmd );
		FreeRICmd( &rsh.device, &frameSet->backBufferCmd );

#if ( DEVICE_IMPL_VULKAN )
		vkDestroyCommandPool( rsh.device.vk.device, frameSet->vk.pool, NULL );
#endif
		arrfree( frameSet->freeList );
		arrfree( frameSet->secondaryCmd );
		memset(frameSet, 0, sizeof(struct r_frame_set_s));
	}
#if ( DEVICE_IMPL_VULKAN )
		vkDestroySemaphore( rsh.device.vk.device, rsh.vk.frameSemaphore, NULL );
#endif


	__ShutdownSwapchainTexture();

	ri.Mutex_Destroy( &rf.speedsMsgLock );
	ri.Mutex_Destroy( &rf.debugSurfaceLock );

	R_FreePool( &r_mempool );

	R_WIN_Shutdown();
	FreeRIDevice(&rsh.device);
	ShutdownRIRenderer(&rsh.renderer);
	//R_FreeNriBackend(&rsh.nri);

}

static void RF_CheckCvars( void )
{
	TracyCZone(ctx, 1);
	// disallow bogus r_maxfps values, reset to default value instead
	if( r_maxfps->modified ) {
		if( r_maxfps->integer <= 0 ) {
			Cvar_ForceSet( r_maxfps->name, r_maxfps->dvalue );
		}
		r_maxfps->modified = false;
	}

	// update gamma
 // if( r_gamma->modified )
 // {
 // 	r_gamma->modified = false;
 // 	rrf.adapter.cmdPipe->SetGamma( rrf.adapter.cmdPipe, r_gamma->value );
 // }
 // 
	if( r_texturefilter->modified )
	{
		r_texturefilter->modified = false;
		R_AnisotropicFilter(r_texturefilter->integer);
		//rrf.adapter.cmdPipe->SetTextureFilter( rrf.adapter.cmdPipe, r_texturefilter->integer );
	}

	if( r_wallcolor->modified || r_floorcolor->modified ) {
		vec3_t wallColor, floorColor;
		
		sscanf( r_wallcolor->string,  "%3f %3f %3f", &wallColor[0], &wallColor[1], &wallColor[2] );
		sscanf( r_floorcolor->string, "%3f %3f %3f", &floorColor[0], &floorColor[1], &floorColor[2] );
		
		r_wallcolor->modified = r_floorcolor->modified = false;
		
		for( size_t i = 0; i < 3; i++ ) {
			rsh.wallColor[i] = bound( 0, floor( wallColor[i] ) / 255.0, 1.0 );
			rsh.floorColor[i] = bound( 0, floor( floorColor[i] ) / 255.0, 1.0 );
		}
		//rrf.adapter.cmdPipe->SetWallFloorColors( rrf.adapter.cmdPipe, wallColor, floorColor );		
	}
	
	// texturemode stuff
	if( r_texturemode->modified )
	{
		r_texturemode->modified = false;
		R_TextureMode(r_texturemode->string);
	}
	
	// keep r_outlines_cutoff value in sane bounds to prevent wallhacking
	if( r_outlines_scale->modified ) {
		if( r_outlines_scale->value < 0 ) {
			Cvar_ForceSet( r_outlines_scale->name, "0" );
		}
		else if( r_outlines_scale->value > 3 ) {
			Cvar_ForceSet( r_outlines_scale->name, "3" );
		}
		r_outlines_scale->modified = false;
	}
  TracyCZoneEnd(ctx);
}

void RF_BeginFrame( float cameraSeparation, bool forceClear, bool forceVsync )
{
	TracyCZone(ctx, 1);
	RF_CheckCvars();

	// run cinematic passes on shaders
	R_RunAllCinematics();
	struct r_frame_set_s* activeSet = R_GetActiveFrameSet();

	memset( &rsh.frame, 0, sizeof( rsh.frame ) ); // reset the primary cmd buffer
#if ( DEVICE_IMPL_VULKAN )
	{
		if( rsh.frameSetCount >= NUMBER_FRAMES_FLIGHT ) {
			const uint64_t waitValue = 1 + rsh.frameSetCount - NUMBER_FRAMES_FLIGHT;
			VkSemaphoreWaitInfo semaphoreWaitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
			semaphoreWaitInfo.semaphoreCount = 1;
			semaphoreWaitInfo.pSemaphores = &rsh.vk.frameSemaphore;
			semaphoreWaitInfo.pValues = &waitValue;
			VK_WrapResult( vkWaitSemaphores( rsh.device.vk.device, &semaphoreWaitInfo, 5000 * 1000000ull ) );
			VK_WrapResult( vkResetCommandPool( rsh.device.vk.device, activeSet->vk.pool, 0 ) );
			for( size_t i = 0; i < arrlen( activeSet->secondaryCmd ); i++ ) {
				vkFreeCommandBuffers( rsh.device.vk.device, activeSet->vk.pool, 1, &activeSet->secondaryCmd[i].vk.cmd );
			}
			arrsetlen( activeSet->secondaryCmd, 0 );
		}
		for( size_t i = 0; i < arrlen( activeSet->freeList ); i++ ) {
			FreeRIFree( &rsh.device, &activeSet->freeList[i] );
		}
		RIResetScratchAlloc( &rsh.device, &activeSet->uboScratchAlloc );
		arrsetlen( activeSet->freeList, 0 );
		{
			rsh.vk.swapchainIndex = RISwapchainAcquireNextTexture( &rsh.device, &rsh.riSwapchain );
			rsh.frame.handle.vk.cmd = activeSet->backBufferCmd.vk.cmd;
			{
				VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkBeginCommandBuffer( activeSet->primaryCmd.vk.cmd, &info );
			}
			{
				VkCommandBufferInheritanceInfo inheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
				VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				info.pInheritanceInfo = &inheritanceInfo;
				info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkBeginCommandBuffer( rsh.frame.handle.vk.cmd, &info );
			}
			VkImageMemoryBarrier2 imageBarriers[4] = { 0 };
			imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			imageBarriers[0].srcAccessMask = VK_ACCESS_2_NONE;
			imageBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			imageBarriers[0].dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[0].image = rsh.colorAttachment[rsh.vk.swapchainIndex].texture->vk.image;
			imageBarriers[0].subresourceRange = (VkImageSubresourceRange){
				VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
			};

			imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarriers[1].srcStageMask = VK_PIPELINE_STAGE_2_NONE;
			imageBarriers[1].srcAccessMask = VK_ACCESS_2_NONE;
			imageBarriers[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			imageBarriers[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarriers[1].image = rsh.depthAttachment[rsh.vk.swapchainIndex].texture->vk.image;
			imageBarriers[1].subresourceRange = (VkImageSubresourceRange){
				VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
			};

			struct RI_PogoBuffer *pogoBuffer = rsh.pogoBuffer + rsh.vk.swapchainIndex;
			imageBarriers[2] = VK_RI_PogoAttachmentMemoryBarrier2( pogoBuffer->pogoAttachment[pogoBuffer->attachmentIndex].texture->vk.image, true );
			imageBarriers[3] = VK_RI_PogoShaderMemoryBarrier2( pogoBuffer->pogoAttachment[( pogoBuffer->attachmentIndex + 1 ) % 2].texture->vk.image, true );
			
			VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			dependencyInfo.imageMemoryBarrierCount = Q_ARRAY_COUNT( imageBarriers );
			dependencyInfo.pImageMemoryBarriers = imageBarriers;
			vkCmdPipelineBarrier2( rsh.frame.handle.vk.cmd, &dependencyInfo );
		}
		{
			VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			RI_VK_FillColorAttachment( &colorAttachment, &rsh.colorAttachment[rsh.vk.swapchainIndex], true );

			VkRenderingAttachmentInfo depthStencil = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			RI_VK_FillDepthAttachment( &depthStencil, &rsh.depthAttachment[rsh.vk.swapchainIndex], true );
			enum RI_Format_e attachments[] = {rsh.riSwapchain.format};
			FR_ConfigurePipelineAttachment(&rsh.frame.pipeline, attachments, Q_ARRAY_COUNT(attachments), RI_FORMAT_D32_SFLOAT);

			struct RIViewport_s viewport = { 0 };
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = rsh.riSwapchain.width;
			viewport.height = rsh.riSwapchain.height;
			viewport.depthMax = 1.0f;
			viewport.originBottomLeft = true;
			FR_CmdSetViewport( &rsh.frame, viewport );
			
			struct RIRect_s rect = { 0 };
			rect.width = viewport.width;
			rect.height = viewport.height;
			FR_CmdSetScissor( &rsh.frame, rect );

			VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
			renderingInfo.flags = 0;
			renderingInfo.renderArea = RIViewportToRect2D( &rsh.frame.viewport );
			renderingInfo.layerCount = 1;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.pColorAttachments = &colorAttachment;
			renderingInfo.pDepthAttachment = &depthStencil;
			renderingInfo.pStencilAttachment = NULL;
			vkCmdBeginRendering( rsh.frame.handle.vk.cmd, &renderingInfo );
		}
	}
#endif
	rrf.cameraSeparation = cameraSeparation;

	memset( &rf.stats, 0, sizeof( rf.stats ) );

	// update fps meter
	// copy in changes from R_BeginFrame
	const unsigned int time = ri.Sys_Milliseconds();
	rf.fps.count++;
	rf.fps.time = time;
	if( rf.fps.time - rf.fps.oldTime >= 250 ) {
		rf.fps.average = time - rf.fps.oldTime;
		rf.fps.average = 1000.0f * (rf.fps.count - rf.fps.oldCount) / (float)rf.fps.average + 0.5f;
		rf.fps.oldTime = time;
		rf.fps.oldCount = rf.fps.count;
	}
	rf.width2D = -1;
	rf.height2D = -1;
	R_Set2DMode( &rsh.frame, true );
	TracyCZoneEnd( ctx );
}

static inline void __R_PolyBlendPostPass(struct FrameState_s* frame) {
	if( !r_polyblend->integer )
		return;
	if( rsc.refdef.blend[3] < 0.01f )
		return;

	R_Set2DMode( frame, true );
	R_DrawStretchPic(frame, 0, 0, 
									rsh.riSwapchain.width, 
									rsh.riSwapchain.height, 0, 0, 1, 1, rsc.refdef.blend, rsh.whiteShader );
	RB_FlushDynamicMeshes( frame );
}

static inline void __R_ApplyBrightnessBlend(struct FrameState_s* frame) {

	float c = r_brightness->value;
	if( c < 0.005 )
		return;
	else if( c > 1.0 )
		c = 1.0;

	vec4_t color;
	color[0] = color[1] = color[2] = c;
	color[3] = 1;

	
	R_Set2DMode( frame, true );
	R_DrawStretchQuick(frame, 0, 0, rsh.riSwapchain.width, rsh.riSwapchain.height, 0, 0, 1, 1,
		color, GLSL_PROGRAM_TYPE_NONE, rsh.whiteTexture, GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE );
}

void RF_EndFrame( void )
{
	TracyCZone(ctx, 1);
	// render previously batched 2D geometry, if any
	RB_FlushDynamicMeshes(&rsh.frame);

	__R_PolyBlendPostPass(&rsh.frame);

	__R_ApplyBrightnessBlend(&rsh.frame);
	struct r_frame_set_s *activeSet = R_GetActiveFrameSet();

#if ( DEVICE_IMPL_VULKAN )
	{
		vkCmdEndRendering( rsh.frame.handle.vk.cmd );
		VkImageMemoryBarrier2 imageBarriers[1] = { 0 };
		imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		imageBarriers[0].srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		imageBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_NONE;
		imageBarriers[0].dstAccessMask = VK_ACCESS_2_NONE;
		imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarriers[0].image = rsh.colorAttachment[rsh.vk.swapchainIndex].texture->vk.image;
		imageBarriers[0].subresourceRange = (VkImageSubresourceRange){
			VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS,
		};
		VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = imageBarriers;
		vkCmdPipelineBarrier2( rsh.frame.handle.vk.cmd, &dependencyInfo );

		vkEndCommandBuffer( rsh.frame.handle.vk.cmd );

		RI_InsertTransitionBarriers( &rsh.device, &rsh.uploader, &activeSet->primaryCmd );
		for(size_t i = 0; i < arrlen(activeSet->secondaryCmd); i++) {
			vkCmdExecuteCommands( activeSet->primaryCmd.vk.cmd, 1, &activeSet->secondaryCmd[i].vk.cmd);
		}

		vkCmdExecuteCommands( activeSet->primaryCmd.vk.cmd, 1, &rsh.frame.handle.vk.cmd );
		vkEndCommandBuffer( activeSet->primaryCmd.vk.cmd );

		struct RIQueue_s *graphicsQueue = &rsh.device.queues[RI_QUEUE_GRAPHICS];
		VkCommandBufferSubmitInfo cmdSubmitInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
		cmdSubmitInfo.commandBuffer = activeSet->primaryCmd.vk.cmd;

		VkSemaphoreSubmitInfo waitSem = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
		waitSem.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		waitSem.value = rsh.uploader.syncIndex;
		waitSem.semaphore = rsh.uploader.vk.uploadSem;

		VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
		submitInfo.pCommandBufferInfos = &cmdSubmitInfo;
		submitInfo.commandBufferInfoCount = 1;

		RI_ResourceSubmit(&rsh.device, &rsh.uploader);
		VK_WrapResult(vkQueueSubmit2( graphicsQueue->vk.queue, 1, &submitInfo, VK_NULL_HANDLE ));
		RISwapchainPresent( &rsh.device, &rsh.riSwapchain );

		{
			VkSemaphoreSubmitInfo signalSem = { VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
			signalSem.stageMask = VK_PIPELINE_STAGE_2_NONE;
			signalSem.value = 1 + rsh.frameSetCount;
			signalSem.semaphore = rsh.vk.frameSemaphore;
			VkSubmitInfo2 submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
			submitInfo.pSignalSemaphoreInfos = &signalSem;
			submitInfo.signalSemaphoreInfoCount = 1;
			
			VK_WrapResult( vkQueueSubmit2( graphicsQueue->vk.queue, 1, &submitInfo, VK_NULL_HANDLE ) );
		}
	}
#endif

	rsh.frameSetCount++;
  	TracyCZoneEnd(ctx);
}

void RF_BeginRegistration( void )
{
	// sync to the backend thread to ensure it's not using old assets for drawing
	//RF_AdapterWait( &rrf.adapter );
	R_BeginRegistration();
	RB_BeginRegistration();	

//rrf.adapter.cmdPipe->BeginRegistration( rrf.adapter.cmdPipe );
	//RF_AdapterWait( &rrf.adapter );
}

void RF_EndRegistration( void )
{
	R_EndRegistration();
	RB_EndRegistration();
	//RFB_FreeUnusedObjects(); // todo: redo fbo logic
	// reset the cache of custom colors, otherwise RF_SetCustomColor might fail to do anything
	memset( rrf.customColors, 0, sizeof( rrf.customColors ) );
}

void RF_RegisterWorldModel( const char *model, const dvis_t *pvsData )
{
	R_RegisterWorldModel( model, pvsData );
}

void RF_ClearScene( void )
{
	R_ClearScene();	
}

void RF_AddEntityToScene( const entity_t *ent )
{
	R_AddEntityToScene(ent);
}

void RF_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b )
{
	R_AddLightToScene( org, intensity, r, g, b );
}

void RF_AddPolyToScene( const poly_t *poly )
{
	R_AddPolyToScene( poly );
}

void RF_AddLightStyleToScene( int style, float r, float g, float b )
{
	R_AddLightStyleToScene( style, r, g, b );
}

void RF_RenderScene( const refdef_t *fd )
{
	R_RenderScene( fd );
}

void RF_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, const vec4_t color, const shader_t *shader )
{
	R_DrawRotatedStretchPic( &rsh.frame, x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

void RF_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, const vec4_t color, const shader_t *shader )
{
	R_DrawRotatedStretchPic( &rsh.frame, x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

void RF_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, float s1, float t1, float s2, float t2, uint8_t *data )
{
	assert(false); // eeh doesn't need to implement this yet
	//if( !cols || !rows )
	//	return;

	//if( data )
	//	R_UploadRawPic( rsh.rawTexture, cols, rows, data );

	//rrf.frame->DrawStretchRaw( rrf.frame, x, y, w, h, s1, t1, s2, t2 );
}

void RF_DrawStretchRawYUV( int x, int y, int w, int h, 
	float s1, float t1, float s2, float t2, ref_img_plane_t *yuv )
{
	//if( yuv )
	//	R_UploadRawYUVPic( rsh.rawYUVTextures, yuv );

	//rrf.frame->DrawStretchRawYUV( rrf.frame, x, y, w, h, s1, t1, s2, t2 );
}

void RF_DrawStretchPoly( const poly_t *poly, float x_offset, float y_offset )
{
	R_DrawStretchPoly( &rsh.frame, poly, x_offset, y_offset );
}

void RF_SetScissor( int x, int y, int w, int h )
{
	struct RIRect_s rect;
	rect.x = max(0,x);
	rect.y = max(0,y);
	rect.width = w;
	rect.height = h;
	FR_CmdSetScissor(&rsh.frame, rect);
	Vector4Set( rrf.scissor, x, y, w, h );
}

void R_InitSubpass( struct FrameState_s *parent, struct FrameState_s *child)
{
	struct r_frame_set_s *active = R_GetActiveFrameSet();
	if( parent ) {
		( *child) = *parent;
	} else {
		memset( child, 0, sizeof( struct FrameState_s ) );
	}
	child->parent = parent;
#if ( DEVICE_IMPL_VULKAN )
	{
		VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		cmdAllocInfo.commandPool = active->vk.pool;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdAllocInfo.commandBufferCount = 1;
		struct RICmd_s cmd = { 0 };
		cmd.vk.pool = active->vk.pool;
		VK_WrapResult( vkAllocateCommandBuffers( rsh.device.vk.device, &cmdAllocInfo, &cmd.vk.cmd ) );
		assert(IsRICmdValid(&rsh.renderer, &cmd));
		arrpush( active->secondaryCmd, cmd );
		child->handle = cmd;
	}
#else
#error unhandled
#endif
}

void RF_GetScissor( int *x, int *y, int *w, int *h )
{

	if( x )
		*x = rrf.scissor[0];
	if( y )
		*y = rrf.scissor[1];
	if( w )
		*w = rrf.scissor[2];
	if( h )
	 	*h = rrf.scissor[3];
}

void RF_ResetScissor( void )
{
	//struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	struct RIRect_s rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = rsh.riSwapchain.width;
	rect.height = rsh.riSwapchain.height;
	FR_CmdSetScissor(&rsh.frame, rect);
	Vector4Set( rrf.scissor, 0, 0, rsh.riSwapchain.width, rsh.riSwapchain.height );
}

void RF_SetCustomColor( int num, int r, int g, int b )
{
	if( num < 0 || num >= NUM_CUSTOMCOLORS )
		return;
	Vector4Set( rsh.customColors[num], (uint8_t)r, (uint8_t)g, (uint8_t)b, 255 );
 	// *(int *)rrf.customColors[num] = *(int *)rgba;

 // byte_vec4_t rgba;

 // Vector4Set( rgba, r, g, b, 255 );
 // 
 // if( *(int *)rgba != *(int *)rrf.customColors[num] ) {
 // 	rrf.adapter.cmdPipe->SetCustomColor( rrf.adapter.cmdPipe, num, r, g, b );
 // 	*(int *)rrf.customColors[num] = *(int *)rgba;
 // }
}

static struct tm *__Localtime( const time_t time, struct tm* _tm )
{
#ifdef _WIN32
	struct tm* __tm = localtime( &time );
	*_tm = *__tm;
#else
	localtime_r( &time, _tm );
#endif
	return _tm;
}



void RF_ScreenShot( const char *path, const char *name, const char *fmtstring, bool silent )
{
	if(rsh.screenshot.state != CAPTURE_STATE_NONE) {
		return;
	}

	const char *extension;
	char *checkname = NULL;
	size_t checkname_size = 0;
	
	switch(r_screenshot_format->integer) 
	{
		case 2:
			extension = ".jpg";
			break;
		case 3:
			extension = ".tga";
			break;
		default:
			extension = ".png";
			break;
	}
	
	if( name && name[0] && Q_stricmp(name, "*") )
	{
		if( !COM_ValidateRelativeFilename( name) )
		{
			Com_Printf( "Invalid filename\n" );
			return;
		}
		
		const size_t checkname_size =  strlen(path) + strlen( name) + strlen( extension ) + 1;
		checkname = alloca( checkname_size );
		Q_snprintfz( checkname, checkname_size, "%s%s", path, name);
		COM_DefaultExtension( checkname, extension, checkname_size );
	}

  if( !checkname )
	{
		const int maxFiles = 100000;
		static int lastIndex = 0;
		bool addIndex = false;
		char timestampString[MAX_QPATH];
		static char lastFmtString[MAX_QPATH];
		struct tm newtime;
		
		__Localtime( time( NULL ), &newtime );
		strftime( timestampString, sizeof( timestampString ), fmtstring, &newtime );

		checkname_size = strlen(path) + strlen( timestampString ) + 5 + 1 + strlen( extension );
		checkname = alloca( checkname_size );
		
		// if the string format is a constant or file already exists then iterate
		if( !*fmtstring || !strcmp( timestampString, fmtstring  ) )
		{
			addIndex = true;
			
			// force a rescan in case some vars have changed..
			if( strcmp( lastFmtString, fmtstring) )
			{
				lastIndex = 0;
				Q_strncpyz( lastFmtString, fmtstring, sizeof( lastFmtString ) );
				r_screenshot_fmtstr->modified = false;
			}
	
		}
		else
		{
			Q_snprintfz( checkname, checkname_size, "%s%s%s", path, timestampString, extension );
			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) != -1 )
			{
				lastIndex = 0;
				addIndex = true;
			}
		}
		
		for( ; addIndex && lastIndex < maxFiles; lastIndex++ )
		{
			Q_snprintfz( checkname, checkname_size, "%s%s%05i%s",path, timestampString, lastIndex, extension );
			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) == -1 )
				break; // file doesn't exist
		}
		
		if( lastIndex == maxFiles )
		{
			Com_Printf( "Couldn't create a file\n" );
			return;
		}
		lastIndex++;
	}

	rsh.screenshot.single.path = sdsnew(checkname);
	rsh.screenshot.state = CAPTURE_STATE_RECORD_SCREENSHOT;
}

void RF_EnvShot( const char *path, const char *name, unsigned pixels )
{
	if( RF_RenderingEnabled() ) {
		R_TakeEnvShot( &rsh.frame, path, name, pixels );
	}
		// rrf.adapter.cmdPipe->EnvShot( rrf.adapter.cmdPipe, path, name, pixels );
}

bool RF_RenderingEnabled( void )
{
	return true;
	//return GLimp_RenderingEnabled();
}

const char *RF_GetSpeedsMessage( char *out, size_t size )
{
	ri.Mutex_Lock( rf.speedsMsgLock );
	Q_strncpyz( out, rf.speedsMsg, size );
	ri.Mutex_Unlock( rf.speedsMsgLock );
	return out;
}

int RF_GetAverageFramerate( void )
{
	return rf.fps.average;
}

void RF_ReplaceRawSubPic( shader_t *shader, int x, int y, int width, int height, uint8_t *data )
{
	R_ReplaceRawSubPic( shader, x, y, width, height, data );
}

void RF_BeginAviDemo( void )
{
	assert(false);
	//RF_AdapterWait( &rrf.adapter );
}

void RF_WriteAviFrame( int frame, bool scissor )
{
	int x, y, w, h;
	const char *writedir, *gamedir;
	size_t path_size;
	char *path;
	char name[32];
	
	if( !R_IsRenderingToScreen() )
		return;

	if( scissor )
	{
		x = rsc.refdef.x;
		
		y = rsh.riSwapchain.height - rsc.refdef.height - rsc.refdef.y;
		w = rsc.refdef.width;
		h = rsc.refdef.height;
	}
	else
	{
		x = 0;
		y = 0;
		w = rsh.riSwapchain.width;
		h = rsh.riSwapchain.height;
	}
	
	writedir = FS_WriteDirectory();
	gamedir = FS_GameDirectory();
	path_size = strlen( writedir ) + 1 + strlen( gamedir ) + strlen( "/avi/" ) + 1;
	path = alloca( path_size );
	Q_snprintfz( path, path_size, "%s/%s/avi/", writedir, gamedir );
	Q_snprintfz( name, sizeof( name ), "%06i", frame );
	
//	RF_AdapterWait( &rrf.adapter );
	
	//rrf.adapter.cmdPipe->AviShot( rrf.adapter.cmdPipe, path, name, x, y, w, h );
}

void RF_StopAviDemo( void )
{
//	RF_AdapterWait( &rrf.adapter );
}

void RF_TransformVectorToScreen( const refdef_t *rd, const vec3_t in, vec2_t out )
{
	mat4_t p, m;
	vec4_t temp, temp2;
 	
	if( !rd || !in || !out )
		return;
 	
	temp[0] = in[0];
	temp[1] = in[1];
	temp[2] = in[2];
	temp[3] = 1.0f;
 	
	if( rd->rdflags & RDF_USEORTHO ) {
		Matrix4_OrthogonalProjection( rd->ortho_x, rd->ortho_x, rd->ortho_y, rd->ortho_y,
			-4096.0f, 4096.0f, p );
	}
	else {
		Matrix4_InfinitePerspectiveProjection( rd->fov_x, rd->fov_y, Z_NEAR, rrf.cameraSeparation,
			p, DEPTH_EPSILON );
	}
 	
	if( rd->rdflags & RDF_FLIPPED ) {
		p[0] = -p[0];
	}
 	
	Matrix4_Modelview( rd->vieworg, rd->viewaxis, m );
 	
	Matrix4_Multiply_Vector( m, temp, temp2 );
	Matrix4_Multiply_Vector( p, temp2, temp );
 	
	if( !temp[3] )
 		return;

	out[0] = rd->x + ( temp[0] / temp[3] + 1.0f ) * rd->width * 0.5f;
	out[1] = rsh.riSwapchain.height - (rd->y + ( temp[1] / temp[3] + 1.0f ) * rd->height * 0.5f);
}

bool RF_LerpTag( orientation_t *orient, const model_t *mod, int oldframe, int frame, float lerpfrac, const char *name )
{
	if( !orient )
		return false;
 	
	VectorClear( orient->origin );
	Matrix3_Identity( orient->axis );
 	
	if( !name )
		return false;
 	
	if( mod->type == mod_alias )
		return R_AliasModelLerpTag( orient, (const maliasmodel_t *)mod->extradata, oldframe, frame, lerpfrac, name );
 	
	return false;
}

void RF_LightForOrigin( const vec3_t origin, vec3_t dir, vec4_t ambient, vec4_t diffuse, float radius )
{
	R_LightForOrigin( origin, dir, ambient, diffuse, radius, false );
}

/*
* RF_GetShaderForOrigin
*
* Trace 64 units in all axial directions to find the closest surface
*/
shader_t *RF_GetShaderForOrigin( const vec3_t origin )
{
	int i, j;
	vec3_t dir, end;
	rtrace_t tr;
	shader_t *best = NULL;
	float best_frac = 1000.0f;

	for( i = 0; i < 3; i++ ) {
		VectorClear( dir );

		for( j = -1; j <= 1; j += 2 ) {
			dir[i] = j;
			VectorMA( origin, 64, dir, end );

			R_TraceLine( &tr, origin, end, 0 );
			if( !tr.shader ) {
				continue;
			}

			if( tr.fraction < best_frac ) {
				best = tr.shader;
				best_frac = tr.fraction;
			}
		}
	}

	return best;
}

struct cinematics_s *RF_GetShaderCinematic( shader_t *shader )
{
	if( !shader ) {
		return NULL;
	}
	return R_GetCinematicById( shader->cin );
}
