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

#include "NRIDescs.h"
#include "r_frame_cmd_buffer.h"
#include "r_image.h"
#include "r_local.h"
#include "r_nri.h"
#include "r_resource_upload.h"
#include "r_frontend.h"

#include "stb_ds.h"
#include "r_capture.h"
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
	if(rsh.swapchain) {
		for( size_t i = 0; i < arrlen( rsh.backBuffers ); i++ ) {
			struct frame_tex_buffers_s *backBuffer = &rsh.backBuffers[i];
			assert( backBuffer->colorAttachment );
			assert( backBuffer->colorTexture );
			assert( backBuffer->depthAttachment );
			assert( backBuffer->depthTexture );

			for( size_t pogoIdx = 0; pogoIdx < Q_ARRAY_COUNT( rsh.backBuffers->pogoBuffers ); pogoIdx++ ) {
				rsh.nri.coreI.DestroyDescriptor( backBuffer->pogoBuffers[pogoIdx].colorAttachment );
				rsh.nri.coreI.DestroyDescriptor( backBuffer->pogoBuffers[pogoIdx].shaderDescriptor.descriptor );
				rsh.nri.coreI.DestroyTexture( backBuffer->pogoBuffers[pogoIdx].colorTexture );
			}
			rsh.nri.coreI.DestroyDescriptor( backBuffer->colorAttachment );
			rsh.nri.coreI.DestroyDescriptor( backBuffer->depthAttachment );
			rsh.nri.coreI.DestroyTexture( backBuffer->depthTexture );

			for( size_t mIdx = 0; mIdx < backBuffer->memoryLen; mIdx++ ) {
				assert( backBuffer->memory[mIdx] );
				rsh.nri.coreI.FreeMemory( backBuffer->memory[mIdx] );
			}
		}
		rsh.nri.swapChainI.DestroySwapChain( rsh.swapchain );
		arrfree(rsh.backBuffers);
	}
	if(rsh.frameFence) {
		rsh.nri.coreI.DestroyFence(rsh.frameFence);
	}
	rsh.frameFence = NULL;
	rsh.swapchainCount = 0;
	rsh.swapchain = NULL;
}

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
	nri_init_desc_t desc = {
		.enableApiValidation = true,
		.enableNriValidation = true,
		.api = NriGraphicsAPI_VK
	};		
	
	if(!R_InitNriBackend(&desc, &rsh.nri)) {
		return rserr_unknown;
	}

	rf.applicationName= R_CopyString( applicationName );
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

	rsh.shadowSamplerDescriptor = R_CreateDescriptorWrapper( &rsh.nri, R_ResolveSamplerDescriptor( IT_DEPTHCOMPARE | IT_SPECIAL | IT_DEPTH ) );
	assert(rsh.frameFence == NULL);
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateFence( rsh.nri.device, 0, &rsh.frameFence ) );
	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.GetCommandQueue(rsh.nri.device, NriCommandQueueType_GRAPHICS, &rsh.cmdQueue) )

	const struct block_buffer_pool_desc_s uboBlockBufferDesc = {
		.blockSize = UBOBlockerBufferSize,
		.alignmentReq = UBOBlockerBufferAlignmentReq,
		.usageBits = NriBufferUsageBits_CONSTANT_BUFFER | NriBufferUsageBits_SHADER_RESOURCE
	};

	for(size_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++) {
		rsh.frameCmds[i].frameCount = i;
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateCommandAllocator( rsh.cmdQueue, &rsh.frameCmds[i].allocator ) );
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateCommandBuffer( rsh.frameCmds[i].allocator, &rsh.frameCmds[i].cmd ) );
		InitBlockBufferPool( &rsh.nri, &rsh.frameCmds[i].uboBlockBuffer, &uboBlockBufferDesc );
	}


	if( err != rserr_ok ) {
		return err;
	}
	R_InitResourceUpload();

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
	TracyCZone(ctx, 1);
	rsh.nri.helperI.WaitForIdle( rsh.cmdQueue );

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
		NriWindow nriWindow = { 0 };
		switch( handle.winType ) {
			case VID_WINDOW_WAYLAND:
				nriWindow.wayland.surface = handle.window.wayland.surface;
				nriWindow.wayland.display = handle.window.wayland.display;
				break;
			case VID_WINDOW_TYPE_X11:
				nriWindow.x11.window = handle.window.x11.window;
				nriWindow.x11.dpy = handle.window.x11.dpy;
				break;
			case VID_WINDOW_WIN32:
				nriWindow.windows.hwnd = handle.window.win.hwnd;
				break;
			default:
				assert( false );
				break;
		}

		NriSwapChainDesc swapChainDesc = { .commandQueue = rsh.nri.graphicsCommandQueue, .width = width, .height = height, .format = DefaultSwapchainFormat, .textureNum = 3, .window = nriWindow };
		__ShutdownSwapchainTexture();

		assert(rsh.frameFence == NULL);
		NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateFence( rsh.nri.device, 0, &rsh.frameFence ) );
		NRI_ABORT_ON_FAILURE( rsh.nri.swapChainI.CreateSwapChain( rsh.nri.device, &swapChainDesc, &rsh.swapchain ) );

		uint32_t swapChainTextureNum = 0;
		NriTexture *const *swapChainTextures = rsh.nri.swapChainI.GetSwapChainTextures( rsh.swapchain, &swapChainTextureNum );
		arrsetlen( rsh.backBuffers, swapChainTextureNum );
		char debugName[1024];
		for( uint32_t i = 0; i < swapChainTextureNum; i++ ) {
			rsh.backBuffers[i].memoryLen = 0;
			rsh.backBuffers[i].screen = ( NriRect ){ .x = 0, .y = 0, .width = swapChainDesc.width, .height = swapChainDesc.height };

			const NriTextureDesc *swapChainDesc = rsh.nri.coreI.GetTextureDesc( swapChainTextures[i] );
			rsh.backBuffers[i].colorTexture = swapChainTextures[i];
			rsh.backBuffers[i].postProcessingSampler = R_ResolveSamplerDescriptor( IT_NOFILTERING );
			{
				NriTexture2DViewDesc textureViewDesc = { swapChainTextures[i], NriTexture2DViewType_COLOR_ATTACHMENT, swapChainDesc->format };
				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureViewDesc, &rsh.backBuffers[i].colorAttachment ) );
			}

			for( size_t pogoIdx = 0; pogoIdx < Q_ARRAY_COUNT( rsh.backBuffers->pogoBuffers ); pogoIdx++ ) {
				NriTextureDesc textureDesc = { .width = swapChainDesc->width,
											   .height = swapChainDesc->height,
											   .depth = 1,
											   .usage = NriTextureUsageBits_COLOR_ATTACHMENT | NriTextureUsageBits_SHADER_RESOURCE,
											   .layerNum = 1,
											   .format = POGO_BUFFER_TEXTURE_FORMAT,
											   .sampleNum = 1,
											   .type = NriTextureType_TEXTURE_2D,
											   .mipNum = 1 };
				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &rsh.backBuffers[i].pogoBuffers[pogoIdx].colorTexture ) );
				Q_snprintfz( debugName, sizeof( debugName ), "pogo[%d][%d]", i, pogoIdx );
				rsh.nri.coreI.SetTextureDebugName( rsh.backBuffers[i].pogoBuffers[pogoIdx].colorTexture, debugName );
				NriResourceGroupDesc resourceGroupDesc = {
					.textureNum = 1,
					.textures = &rsh.backBuffers[i].pogoBuffers[pogoIdx].colorTexture,
					.memoryLocation = NriMemoryLocation_DEVICE,
				};
				const size_t numAllocations = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
				assert( numAllocations + rsh.backBuffers[i].memoryLen <= Q_ARRAY_COUNT( rsh.backBuffers[i].memory ) );
				NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, rsh.backBuffers[i].memoryLen + rsh.backBuffers[i].memory ) )
				rsh.backBuffers[i].memoryLen += numAllocations;

				NriTexture2DViewDesc attachmentViewDesc = {
					.texture = rsh.backBuffers[i].pogoBuffers[pogoIdx].colorTexture, .viewType = NriTexture2DViewType_COLOR_ATTACHMENT, .format = textureDesc.format };
				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &attachmentViewDesc, &rsh.backBuffers[i].pogoBuffers[pogoIdx].colorAttachment ) );

				NriTexture2DViewDesc shaderViewDesc = {
					.texture = rsh.backBuffers[i].pogoBuffers[pogoIdx].colorTexture, .viewType = NriTexture2DViewType_SHADER_RESOURCE_2D, .format = textureDesc.format };
				NriDescriptor *descriptor;
				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &shaderViewDesc, &descriptor ) );

				rsh.backBuffers[i].pogoBuffers[pogoIdx].shaderDescriptor = R_CreateDescriptorWrapper( &rsh.nri, descriptor );
			}

			{
				NriTextureDesc textureDesc = { .width = swapChainDesc->width,
											   .height = swapChainDesc->height,
											   .depth = 1,
											   .usage = NriTextureUsageBits_DEPTH_STENCIL_ATTACHMENT,
											   .layerNum = 1,
											   .format = NriFormat_D32_SFLOAT,
											   .sampleNum = 1,
											   .type = NriTextureType_TEXTURE_2D,
											   .mipNum = 1 };
				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture( rsh.nri.device, &textureDesc, &rsh.backBuffers[i].depthTexture ) );
				Q_snprintfz( debugName, sizeof( debugName ), "backbuffer depth[%d]", i );
				rsh.nri.coreI.SetTextureDebugName( rsh.backBuffers[i].depthTexture, debugName );
				NriResourceGroupDesc resourceGroupDesc = {
					.textureNum = 1,
					.textures = &rsh.backBuffers[i].depthTexture,
					.memoryLocation = NriMemoryLocation_DEVICE,
				};

				const size_t numAllocations = rsh.nri.helperI.CalculateAllocationNumber( rsh.nri.device, &resourceGroupDesc );
				assert( numAllocations + rsh.backBuffers[i].memoryLen <= Q_ARRAY_COUNT( rsh.backBuffers[i].memory ) );
				NRI_ABORT_ON_FAILURE( rsh.nri.helperI.AllocateAndBindMemory( rsh.nri.device, &resourceGroupDesc, rsh.backBuffers[i].memoryLen + rsh.backBuffers[i].memory ) )
				rsh.backBuffers[i].memoryLen += numAllocations;

				NriTexture2DViewDesc textureViewDesc = { .texture = rsh.backBuffers[i].depthTexture, .viewType = NriTexture2DViewType_DEPTH_STENCIL_ATTACHMENT, .format = textureDesc.format };

				NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateTexture2DView( &textureViewDesc, &rsh.backBuffers[i].depthAttachment ) );
			}
		}
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
	rsh.nri.helperI.WaitForIdle( rsh.cmdQueue );

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

	R_ExitResourceUpload();

	R_DisposeScene(&rsc);

	for(size_t i = 0; i < NUMBER_FRAMES_FLIGHT; i++) {
		FrameCmdBufferFree(&rsh.frameCmds[i]);
	}

	__ShutdownSwapchainTexture();

	ri.Mutex_Destroy( &rf.speedsMsgLock );
	ri.Mutex_Destroy( &rf.debugSurfaceLock );

	R_FreePool( &r_mempool );

	R_WIN_Shutdown();

	R_FreeNriBackend(&rsh.nri);

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
		//rrf.adapter.cmdPipe->SetTextureMode( rrf.adapter.cmdPipe, r_texturemode->string );
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

	//rrf.adapter.maxfps = r_maxfps->integer;

	const uint32_t bufferedFrameIndex = rsh.swapchainCount % NUMBER_FRAMES_FLIGHT;
	struct frame_cmd_buffer_s *frame = &rsh.frameCmds[bufferedFrameIndex];

	if( rsh.swapchainCount >= NUMBER_FRAMES_FLIGHT ) {
		rsh.nri.coreI.Wait( rsh.frameFence, 1 + rsh.swapchainCount - NUMBER_FRAMES_FLIGHT );
		rsh.nri.coreI.ResetCommandAllocator( frame->allocator );
	}
	frame->frameCount = rsh.frameCount;
	ResetFrameCmdBuffer( &rsh.nri, frame );

	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.BeginCommandBuffer( frame->cmd, NULL ) );

	// set the cmdState to the backbuffer for rendering going forward
	{
		NriTextureBarrierDesc textureBarrierDescs[3] = {0};
		textureBarrierDescs[0].texture = frame->textureBuffers.colorTexture;
		textureBarrierDescs[0].after = ( NriAccessLayoutStage ){ 
			NriAccessBits_COLOR_ATTACHMENT, 
			NriLayout_COLOR_ATTACHMENT 
		};

		textureBarrierDescs[1].texture = frame->textureBuffers.pogoBuffers[0].colorTexture;
		textureBarrierDescs[1].after = ( NriAccessLayoutStage ){ 
			.layout = NriLayout_COLOR_ATTACHMENT, 
			.access = NriAccessBits_COLOR_ATTACHMENT, 
			.stages = NriStageBits_COLOR_ATTACHMENT 
		};
		frame->textureBuffers.pogoBuffers[0].isAttachment = true;
		
		textureBarrierDescs[2].texture = frame->textureBuffers.pogoBuffers[1].colorTexture;
		textureBarrierDescs[2].after = ( NriAccessLayoutStage ){ 
			.layout = NriLayout_COLOR_ATTACHMENT, 
			.access = NriAccessBits_COLOR_ATTACHMENT, 
			.stages = NriStageBits_COLOR_ATTACHMENT 
		};
		frame->textureBuffers.pogoBuffers[1].isAttachment = true;

		NriBarrierGroupDesc barrierGroupDesc = {0};
		barrierGroupDesc.textureNum = 3;
		barrierGroupDesc.textures = textureBarrierDescs;
		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
	}

	FR_CmdResetAttachmentToBackbuffer(frame);
	if(forceClear)
	{
		NriAttachmentsDesc attachmentsDesc = {0};
		attachmentsDesc.depthStencil = frame->state.depthAttachment;
		attachmentsDesc.colorNum = frame->state.numColorAttachments;
		attachmentsDesc.colors = frame->state.colorAttachment;
		rsh.nri.coreI.CmdBeginRendering( frame->cmd, &attachmentsDesc );
		{
			NriClearDesc clearDesc[MAX_COLOR_ATTACHMENTS + 1] = {0};
			size_t numClearDesc = 0;
			for(size_t i = 0; i < attachmentsDesc.colorNum; i++) {
				clearDesc[numClearDesc].value.color = ( NriColor ){ .f = { 0.0f, 0.0f, 0.0f, 1.0f } };
				clearDesc[numClearDesc].planes = NriPlaneBits_COLOR;
				numClearDesc++;
			}
			clearDesc[numClearDesc].value.depthStencil = ( NriDepthStencil ){ .depth = 1.0f, .stencil = 0.0f };
			clearDesc[numClearDesc].planes = NriPlaneBits_DEPTH;
			numClearDesc++;
			rsh.nri.coreI.CmdClearAttachments( frame->cmd, clearDesc, numClearDesc, NULL, 0 );
		}
		rsh.nri.coreI.CmdEndRendering( frame->cmd );
	}

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
	R_Set2DMode(frame, true );
  TracyCZoneEnd(ctx);
}

static inline void __R_PolyBlendPostPass(struct frame_cmd_buffer_s* frame) {
	if( !r_polyblend->integer )
		return;
	if( rsc.refdef.blend[3] < 0.01f )
		return;

	NriViewport* viewport = &frame->state.viewports[0];
	R_Set2DMode( frame, true );
	R_DrawStretchPic(frame, 0, 0, 
									viewport->width, 
									viewport->height, 0, 0, 1, 1, rsc.refdef.blend, rsh.whiteShader );
	RB_FlushDynamicMeshes( frame );
}

static inline void __R_ApplyBrightnessBlend(struct frame_cmd_buffer_s* frame) {

	float c = r_brightness->value;
	if( c < 0.005 )
		return;
	else if( c > 1.0 )
		c = 1.0;

	vec4_t color;
	color[0] = color[1] = color[2] = c;
	color[3] = 1;
	
	NriViewport* viewport = &frame->state.viewports[0];
	
	R_Set2DMode( frame, true );
	R_DrawStretchQuick(frame, 0, 0, viewport->width, viewport->height, 0, 0, 1, 1,
		color, GLSL_PROGRAM_TYPE_NONE, rsh.whiteTexture, GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE );
}

void RF_EndFrame( void )
{
	TracyCZone(ctx, 1);
	const uint32_t bufferedFrameIndex = rsh.swapchainCount % NUMBER_FRAMES_FLIGHT;
	struct frame_cmd_buffer_s *frame = &rsh.frameCmds[bufferedFrameIndex];

	assert(frame->stackCmdBeingRendered == 0);
	// render previously batched 2D geometry, if any
	RB_FlushDynamicMeshes(frame);

	__R_PolyBlendPostPass(frame);

	__R_ApplyBrightnessBlend(frame);

	R_ResourceSubmit();

	switch( rsh.screenshot.state ) {
		case CAPTURE_STATE_RECORD_SCREENSHOT: {
			NriMemoryDesc memoryDesc = { 0 };
			const NriTextureDesc *textureDesc = rsh.nri.coreI.GetTextureDesc( frame->textureBuffers.colorTexture );
			rsh.nri.coreI.GetTextureMemoryDesc( rsh.nri.device, textureDesc, NriMemoryLocation_HOST_READBACK, &memoryDesc );

			const struct base_format_def_s *formatDef = R_BaseFormatDef( R_FromNRIFormat( textureDesc->format ) );
			NriBufferDesc bufferDesc = { .size = memoryDesc.size };
			NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &rsh.screenshot.single.buffer ) );

			NriAllocateMemoryDesc allocateMemoryDesc = { 0 };
			allocateMemoryDesc.size = memoryDesc.size;
			allocateMemoryDesc.type = memoryDesc.type;
			NRI_ABORT_ON_FAILURE( rsh.nri.coreI.AllocateMemory( rsh.nri.device, &allocateMemoryDesc, &rsh.screenshot.single.memory ) );
			NriBufferMemoryBindingDesc bindBufferDesc = {
				.memory = rsh.screenshot.single.memory,
				.buffer = rsh.screenshot.single.buffer,
			};

			rsh.screenshot.single.textureBuferDesc = (struct texture_buf_desc_s) {
				.alignment = 1,
				.width = textureDesc->width, 
				.height = textureDesc->height, 
				.def = R_BaseFormatDef(R_FromNRIFormat(textureDesc->format))
			};

			NRI_ABORT_ON_FAILURE( rsh.nri.coreI.BindBufferMemory( rsh.nri.device, &bindBufferDesc, 1 ) );
			const uint64_t rowPitch = textureDesc->width * RT_BlockSize( formatDef );
			const NriTextureDataLayoutDesc dstLayoutDesc = { 
				.offset = 0, 
				.rowPitch = rowPitch, 
				.slicePitch = rowPitch * textureDesc->height 
			};
			const NriTextureRegionDesc textureRegionDesc = {
				.x = 0, 
				.y = 0, 
				.z = 0, 
				.width = textureDesc->width, 
				.height = textureDesc->height, 
				.depth = textureDesc->depth, 
				.mipOffset = 0, 
				.layerOffset = 0 
			};
			NriTextureBarrierDesc transitionBarriers = { 0 };
			transitionBarriers.texture = frame->textureBuffers.colorTexture;
			NriBarrierGroupDesc barrierGroupDesc = { 0 };
			barrierGroupDesc.textureNum = 1;
			barrierGroupDesc.textures = &transitionBarriers;

			transitionBarriers.before = ( NriAccessLayoutStage ){ NriAccessBits_COLOR_ATTACHMENT, NriLayout_COLOR_ATTACHMENT };
			transitionBarriers.after = ( NriAccessLayoutStage ){ .layout = NriLayout_COPY_SOURCE, .access = NriAccessBits_COPY_SOURCE, .stages = NriStageBits_COPY };
			rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
			rsh.nri.coreI.CmdReadbackTextureToBuffer( frame->cmd, rsh.screenshot.single.buffer, &dstLayoutDesc, frame->textureBuffers.colorTexture, &textureRegionDesc );
			transitionBarriers.before = ( NriAccessLayoutStage ){ .layout = NriLayout_COPY_SOURCE, .access = NriAccessBits_COPY_SOURCE, .stages = NriStageBits_COPY };
			transitionBarriers.after = ( NriAccessLayoutStage ){ NriAccessBits_COLOR_ATTACHMENT, NriLayout_COLOR_ATTACHMENT };
			rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );

			rsh.screenshot.single.frameCnt = frame->frameCount;
			rsh.screenshot.state = CAPTURE_STATE_FINISH_SCREENSHOT;
			break;
		}
		case CAPTURE_STATE_FINISH_SCREENSHOT: {
			if(rsh.screenshot.single.frameCnt + NUMBER_FRAMES_FLIGHT <  frame->frameCount ) {
  			void* buffer = rsh.nri.coreI.MapBuffer(rsh.screenshot.single.buffer, 0, NRI_WHOLE_SIZE);
				struct texture_buf_s pic = {0};
				T_AliasTextureBuf(&pic, &rsh.screenshot.single.textureBuferDesc, buffer, 0);
				R_SaveScreenshotBuffer(&pic, rsh.screenshot.single.path, r_screenshot_format->integer);

				rsh.nri.coreI.DestroyBuffer( rsh.screenshot.single.buffer );
				rsh.nri.coreI.FreeMemory( rsh.screenshot.single.memory );
				sdsfree( rsh.screenshot.single.path );
				rsh.screenshot.state = CAPTURE_STATE_NONE;
			}
			break;
		}
		default:
			break;
	}

	{
		NriTextureBarrierDesc textureBarrierDescs = { 0 };
		textureBarrierDescs.texture = frame->textureBuffers.colorTexture;
		textureBarrierDescs.before = ( NriAccessLayoutStage ){ NriAccessBits_COLOR_ATTACHMENT, NriLayout_COLOR_ATTACHMENT };
		textureBarrierDescs.after = ( NriAccessLayoutStage ){ NriAccessBits_UNKNOWN, NriLayout_PRESENT };
		textureBarrierDescs.layerNum = 1;
		textureBarrierDescs.mipNum = 1;

		NriBarrierGroupDesc barrierGroupDesc = { 0 };
		barrierGroupDesc.textureNum = 1;
		barrierGroupDesc.textures = &textureBarrierDescs;
		rsh.nri.coreI.CmdBarrier( frame->cmd, &barrierGroupDesc );
	}

	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.EndCommandBuffer( frame->cmd ) );

	{ // Submit
		const NriCommandBuffer *buffers[] = { frame->cmd };
		NriQueueSubmitDesc queueSubmitDesc = { 0 };
		queueSubmitDesc.commandBuffers = buffers;
		queueSubmitDesc.commandBufferNum = 1;
		rsh.nri.coreI.QueueSubmit( rsh.cmdQueue, &queueSubmitDesc );
	}

	// Present
	rsh.nri.swapChainI.QueuePresent( rsh.swapchain );
	{ // Signaling after "Present" improves D3D11 performance a bit
		NriFenceSubmitDesc signalFence = {0};
		signalFence.fence = rsh.frameFence;
		signalFence.value = 1 + rsh.swapchainCount;

		NriQueueSubmitDesc queueSubmitDesc = {0};
		queueSubmitDesc.signalFences = &signalFence;
		queueSubmitDesc.signalFenceNum = 1;

		rsh.nri.coreI.QueueSubmit( rsh.cmdQueue, &queueSubmitDesc );
	}

	rsh.swapchainCount++;
	rsh.frameCount++;

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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	R_RenderScene( cmd, fd );
}

void RF_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, 
	const vec4_t color, const shader_t *shader )
{
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	R_DrawRotatedStretchPic(cmd, x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

void RF_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, 
	const vec4_t color, const shader_t *shader )
{
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	R_DrawRotatedStretchPic(cmd, x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

void RF_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, 
	float s1, float t1, float s2, float t2, uint8_t *data )
{
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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	R_DrawStretchPoly( cmd, poly, x_offset, y_offset );
}

void RF_SetScissor( int x, int y, int w, int h )
{
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	NriRect rect;
	rect.x = max(0,x);
	rect.y = max(0,y);
	rect.width = w;
	rect.height = h;
	FR_CmdSetScissorAll(cmd, rect);
	Vector4Set( rrf.scissor, x, y, w, h );
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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	NriRect rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = cmd->textureBuffers.screen.width;
	rect.height = cmd->textureBuffers.screen.height;
	FR_CmdSetScissorAll(cmd, rect);
	Vector4Set( rrf.scissor, 0, 0, cmd->textureBuffers.screen.width, cmd->textureBuffers.screen.height );
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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
	if( RF_RenderingEnabled() ) {
		R_TakeEnvShot( cmd, path, name, pixels );
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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
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
		y = cmd->textureBuffers.screen.height - rsc.refdef.height - rsc.refdef.y;
		w = rsc.refdef.width;
		h = rsc.refdef.height;
	}
	else
	{
		x = 0;
		y = 0;
		w = cmd->textureBuffers.screen.width;
		h = cmd->textureBuffers.screen.height;
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
	struct frame_cmd_buffer_s *cmd = R_ActiveFrameCmd();
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
	out[1] = cmd->textureBuffers.screen.height - (rd->y + ( temp[1] / temp[3] + 1.0f ) * rd->height * 0.5f);
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
