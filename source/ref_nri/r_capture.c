#include "r_capture.h"
#include "r_texture_buf.h"
#include "ri_renderer.h"
#include "ri_swapchain.h"
#include "ri_vk.h"
#include "stb_image_write.h"

// The readback aliases the mapped buffer through a texture_buf_s described by the swapchain's format.
// RI_Format_e and texture_format_e are separately declared but value-identical enums, so the cast in
// __ScreenshotFormatDef is only sound while they stay in lockstep.
Q_COMPILE_ASSERT_MSG( (int)RI_FORMAT_BGRA8_UNORM == (int)R_FORMAT_BGRA8_UNORM, "RI_Format_e and texture_format_e have diverged" );
Q_COMPILE_ASSERT_MSG( (int)RI_FORMAT_RGBA8_UNORM == (int)R_FORMAT_RGBA8_UNORM, "RI_Format_e and texture_format_e have diverged" );

static const struct base_format_def_s *__ScreenshotFormatDef( void )
{
	return R_BaseFormatDef( (enum texture_format_e)rsh.swapchain.format );
}

void R_SaveScreenshotBuffer(struct texture_buf_s *pic, const char* path, int image_type )
{
	const enum texture_logical_channel_e expectBGR[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED };
	const enum texture_logical_channel_e expectBGRA[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED, R_LOGICAL_C_ALPHA };
	const bool isBGRTexture = RT_ExpectChannelsMatch( pic->def, expectBGR, Q_ARRAY_COUNT( expectBGR ) ) ||
	                          RT_ExpectChannelsMatch( pic->def, expectBGRA, Q_ARRAY_COUNT( expectBGRA ) );
	enum texture_logical_channel_e swizzleChannel[R_LOGICAL_C_MAX] = { 0 };
	if( isBGRTexture ) {
		const size_t numberChannels = RT_NumberChannels( pic->def );
		assert( numberChannels >= 3 && numberChannels <= Q_ARRAY_COUNT( swizzleChannel ) );
		memcpy( swizzleChannel, RT_Channels( pic->def ), numberChannels );
		swizzleChannel[0] = R_LOGICAL_C_RED;
		swizzleChannel[1] = R_LOGICAL_C_GREEN;
		swizzleChannel[2] = R_LOGICAL_C_BLUE;
		T_SwizzleInplace( pic, swizzleChannel );
	}

  // update alpha to max
  const size_t destBlockSize = RT_BlockSize(pic->def);
	for( size_t slice = 0; slice < pic->height; slice++ ) {
		const size_t dstRowStart = pic->rowPitch * slice;
		for( size_t column = 0; column < pic->width; column++ ) {
		  pic->buffer[dstRowStart + ( destBlockSize * column ) + 3] = 255;
		}
	}

	int file;
	if( FS_FOpenAbsoluteFile(path, &file, FS_WRITE ) == -1 ) {
		Com_Printf( "WriteScreenShot: Couldn't create %s\n", path);
		return;
	}
	FS_FCloseFile( file );

	int res = 0;
	switch( image_type) {
		case 2:
			res = stbi_write_jpg( path, pic->width, pic->height, RT_NumberChannels( pic->def ), pic->buffer, 100 );
			break;
		case 3:
			res = stbi_write_tga( path, pic->width, pic->height, RT_NumberChannels( pic->def ), pic->buffer );
			break;
		default:
			res = stbi_write_png( path, pic->width, pic->height, RT_NumberChannels( pic->def ), pic->buffer, 0 );
			break;
	}
	if( res == 0 ) {
		Com_Printf( "WriteScreenShot: Couldn't create %s\n", path);
	}
}

static void __R_CaptureAbortScreenshot( void )
{
	qStrFree( &rsh.screenshot.single.path );
	rsh.screenshot.state = CAPTURE_STATE_NONE;
}

bool R_CaptureRecordScreenshot( struct RICmd_s *cmd )
{
	if( rsh.screenshot.state != CAPTURE_STATE_RECORD_SCREENSHOT )
		return false;

#if ( DEVICE_IMPL_VULKAN )
	// the acquire failed, so this frame's image was never rendered and won't be presented
	if( rsh.swapchain.vk.acquireFailed ) {
		__R_CaptureAbortScreenshot();
		return false;
	}

	const struct base_format_def_s *def = __ScreenshotFormatDef();
	if( !def ) {
		Com_Printf( "screenshot: unsupported swapchain format %u\n", rsh.swapchain.format );
		__R_CaptureAbortScreenshot();
		return false;
	}

	const uint32_t width = rsh.swapchain.width;
	const uint32_t height = rsh.swapchain.height;
	const size_t size = (size_t)width * (size_t)height * RT_BlockSize( def );
	struct RIBuffer_s *readback = &rsh.screenshot.single.readback;

	VkBufferCreateInfo bufInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufInfo.size = size;
	bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = { 0 };
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	// HOST_ACCESS_RANDOM asks for HOST_CACHED memory. The upload paths use SEQUENTIAL_WRITE, which lands
	// in write-combined memory that is correct but pathologically slow to read back on the CPU.
	allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

	if( vmaCreateBuffer( rsh.device.vk.vmaAllocator, &bufInfo, &allocInfo, &readback->vk.buffer, &readback->vk.allocation, NULL ) != VK_SUCCESS ) {
		Com_Printf( "screenshot: failed to allocate a %zu byte readback buffer\n", size );
		__R_CaptureAbortScreenshot();
		return false;
	}
	rsh.screenshot.single.readbackSize = size;
	rsh.screenshot.single.textureBuferDesc = ( struct texture_buf_desc_s ){
		.width = width,
		.height = height,
		// tightly packed, matching the copy below
		.alignment = 1,
		.def = def,
	};

	const struct RITexture_s backbuffer = RISwapchainGetTexture( &rsh.swapchain, rsh.swapchainIndex );

	// The frame just finished rendering into the backbuffer, so take it COLOR_ATTACHMENT -> COPY_SRC,
	// pull it into the readback buffer, then hand it to the presenter. This replaces the frame's usual
	// RENDER_TARGET -> PRESENT transition, which RF_EndFrame skips when we return true.
	RICmdImageBarrier( &rsh.device, cmd,
					   &( struct RIImageBarrier_s ){
						   .texture = &backbuffer,
						   .before = RI_RESOURCE_STATE_RENDER_TARGET,
						   .after = RI_RESOURCE_STATE_COPY_SRC,
						   .aspect = RI_BARRIER_ASPECT_COLOR,
					   } );

	RICmdCopyTextureToBuffer( &rsh.device, cmd,
							  &( struct RICopyTextureToBufferDesc_s ){
								  .src = &backbuffer,
								  .dst = readback,
								  .aspect = RI_BARRIER_ASPECT_COLOR,
								  .width = width,
								  .height = height,
							  } );

	// Make the copy visible to the host. The frame fence alone would order it, but spelling it as a
	// barrier is what keeps the map in R_CaptureFinishScreenshot correct under validation.
	RICmdBufferBarrier( &rsh.device, cmd,
						&( struct RIBufferBarrier_s ){
							.buffer = readback,
							.before = RI_RESOURCE_STATE_COPY_DST,
							.after = RI_RESOURCE_STATE_HOST_READ,
						} );

	RICmdImageBarrier( &rsh.device, cmd,
					   &( struct RIImageBarrier_s ){
						   .texture = &backbuffer,
						   .before = RI_RESOURCE_STATE_COPY_SRC,
						   .after = RI_RESOURCE_STATE_PRESENT,
						   .aspect = RI_BARRIER_ASPECT_COLOR,
					   } );

	return true;
#else
	__R_CaptureAbortScreenshot();
	return false;
#endif
}

void R_CaptureFinishScreenshot( void )
{
#if ( DEVICE_IMPL_VULKAN )
	struct RIBuffer_s *readback = &rsh.screenshot.single.readback;
	const char *path = rsh.screenshot.single.path.buf;

	// the copy is only visible to the host after an invalidate on non-coherent (HOST_CACHED) memory
	VK_WrapResult( vmaInvalidateAllocation( rsh.device.vk.vmaAllocator, readback->vk.allocation, 0, rsh.screenshot.single.readbackSize ) );

	void *mapped = NULL;
	if( vmaMapMemory( rsh.device.vk.vmaAllocator, readback->vk.allocation, &mapped ) == VK_SUCCESS ) {
		struct texture_buf_s pic = { 0 };
		T_AliasTextureBuf( &pic, &rsh.screenshot.single.textureBuferDesc, mapped, rsh.screenshot.single.readbackSize );
		R_SaveScreenshotBuffer( &pic, path, r_screenshot_format->integer );
		vmaUnmapMemory( rsh.device.vk.vmaAllocator, readback->vk.allocation );

		FS_AddFileToMedia( path );
		if( !rsh.screenshot.single.silent )
			Com_Printf( "Wrote %s\n", path );
	} else {
		Com_Printf( "screenshot: failed to map the readback buffer\n" );
	}

	// the frame timeline has passed the copy, so the buffer is free to release immediately
	vmaDestroyBuffer( rsh.device.vk.vmaAllocator, readback->vk.buffer, readback->vk.allocation );
	memset( readback, 0, sizeof( *readback ) );
#endif
	__R_CaptureAbortScreenshot();
}
