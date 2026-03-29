#include "r_capture.h"
#include "r_texture_buf.h"
#include "stb_image_write.h"

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

//static struct tm *R_Localtime( const time_t time, struct tm* _tm )
//{
//#ifdef _WIN32
//	struct tm* __tm = localtime( &time );
//	*_tm = *__tm;
//#else
//	localtime_r( &time, _tm );
//#endif
//	return _tm;
//}
//
//
//struct screenshot_cb_handler {
//  NriMemory* memory;
//  NriBuffer* buffer;
//  const NriTextureDesc* textureDesc;
//  NriTextureDataLayoutDesc dstLayoutDesc;
//  const char* path;
//};
//
//static void __RF_Screensot_CB(void* self,struct frame_cmd_buffer_s* cmd) {
//  struct screenshot_cb_handler* handler = (struct screenshot_cb_handler*) self;
//  void* buffer = rsh.nri.coreI.MapBuffer(handler->buffer, 0, handler->dstLayoutDesc.rowPitch * handler->dstLayoutDesc.slicePitch);
//	
//	struct texture_buf_desc_s desc = {
//		.alignment = 1,
//		.width = handler->textureDesc->width,
//		.height = handler->textureDesc->height,
//    .def = R_BaseFormatDef(R_FromNRIFormat(handler->textureDesc->format))
//  };
//	struct texture_buf_s pic = {0};
//	T_AliasTextureBuf(&pic, &desc, buffer, handler->dstLayoutDesc.slicePitch * handler->dstLayoutDesc.rowPitch);
//
//		
//	const enum texture_logical_channel_e expectBGR[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED };
//	const enum texture_logical_channel_e expectBGRA[] = { R_LOGICAL_C_BLUE, R_LOGICAL_C_GREEN, R_LOGICAL_C_RED, R_LOGICAL_C_ALPHA };
//	const bool isBGRTexture = RT_ExpectChannelsMatch( pic.def, expectBGR, Q_ARRAY_COUNT( expectBGR ) ) || RT_ExpectChannelsMatch( pic.def, expectBGRA, Q_ARRAY_COUNT( expectBGRA ) );
//	enum texture_logical_channel_e swizzleChannel[R_LOGICAL_C_MAX] = { 0 };
//	if( isBGRTexture ) {
//		const size_t numberChannels = RT_NumberChannels( pic.def);
//		assert( numberChannels >= 3 && numberChannels <= Q_ARRAY_COUNT( swizzleChannel ) );
//		memcpy( swizzleChannel, RT_Channels( pic.def ), numberChannels );
//		swizzleChannel[0] = R_LOGICAL_C_RED;
//		swizzleChannel[1] = R_LOGICAL_C_GREEN;
//		swizzleChannel[2] = R_LOGICAL_C_BLUE;
//		T_SwizzleInplace( &pic, swizzleChannel );
//	}
//
//	
//	int file;
//	if( FS_FOpenAbsoluteFile( handler->path, &file, FS_WRITE ) == -1 ) {
//		Com_Printf( "WriteScreenShot: Couldn't create %s\n", handler->path);
//	  goto cleanup;
//	}
//	FS_FCloseFile( file );
//
//  int res = 0;
//  switch(r_screenshot_format->integer) {
//		case 2:
//		  res = stbi_write_jpg(handler->path, pic.width, pic.height, RT_NumberChannels(pic.def), pic.buffer, 100);	
//			break;
//		case 3:
//			res = stbi_write_tga( handler->path, pic.width, pic.height, RT_NumberChannels(pic.def), pic.buffer );
//			break;
//		default:
//			res = stbi_write_png( handler->path, pic.width, pic.height, RT_NumberChannels(pic.def), pic.buffer , 0 ) ;
//			break;
//
//  }
//  if(res == 0) {
//		Com_Printf( "WriteScreenShot: Couldn't create %s\n", handler->path);
//  }
//
//cleanup:
//	rsh.nri.coreI.DestroyBuffer(handler->buffer);
//	rsh.nri.coreI.FreeMemory(handler->memory);
//  free(handler);
//}
//
//
//void R_ScreenShot_2(struct frame_cmd_buffer_s *cmd,const char *path, const char *name, const char *fmtstring, bool silent) {
//
//	const char *extension;
//	char *checkname = NULL;
//	size_t checkname_size = 0;
//	
//	switch(r_screenshot_format->integer) 
//	{
//		case 2:
//			extension = ".jpg";
//			break;
//		case 3:
//			extension = ".tga";
//			break;
//		default:
//			extension = ".png";
//			break;
//	}
//
//	if( name && name[0] && Q_stricmp(name, "*") )
//	{
//		if( !COM_ValidateRelativeFilename( name) )
//		{
//			Com_Printf( "Invalid filename\n" );
//			return;
//		}
//		
//		const size_t checkname_size =  strlen(path) + strlen( name) + strlen( extension ) + 1;
//		checkname = alloca( checkname_size );
//		Q_snprintfz( checkname, checkname_size, "%s%s", path, name);
//		COM_DefaultExtension( checkname, extension, checkname_size );
//	}
//
//  if( !checkname )
//	{
//		const int maxFiles = 100000;
//		static int lastIndex = 0;
//		bool addIndex = false;
//		char timestampString[MAX_QPATH];
//		static char lastFmtString[MAX_QPATH];
//		struct tm newtime;
//		
//		R_Localtime( time( NULL ), &newtime );
//		strftime( timestampString, sizeof( timestampString ), fmtstring, &newtime );
//
//		checkname_size = strlen(path) + strlen( timestampString ) + 5 + 1 + strlen( extension );
//		checkname = alloca( checkname_size );
//		
//		// if the string format is a constant or file already exists then iterate
//		if( !*fmtstring || !strcmp( timestampString, fmtstring  ) )
//		{
//			addIndex = true;
//			
//			// force a rescan in case some vars have changed..
//			if( strcmp( lastFmtString, fmtstring) )
//			{
//				lastIndex = 0;
//				Q_strncpyz( lastFmtString, fmtstring, sizeof( lastFmtString ) );
//				r_screenshot_fmtstr->modified = false;
//			}
//	
//		}
//		else
//		{
//			Q_snprintfz( checkname, checkname_size, "%s%s%s", path, timestampString, extension );
//			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) != -1 )
//			{
//				lastIndex = 0;
//				addIndex = true;
//			}
//		}
//		
//		for( ; addIndex && lastIndex < maxFiles; lastIndex++ )
//		{
//			Q_snprintfz( checkname, checkname_size, "%s%s%05i%s",path, timestampString, lastIndex, extension );
//			if( FS_FOpenAbsoluteFile( checkname, NULL, FS_READ ) == -1 )
//				break; // file doesn't exist
//		}
//		
//		if( lastIndex == maxFiles )
//		{
//			Com_Printf( "Couldn't create a file\n" );
//			return;
//		}
//		lastIndex++;
//	}
//
//  struct screenshot_cb_handler* handler = malloc(sizeof(struct screenshot_cb_handler) + strlen(checkname) + 1);
//  char* cb_path = (char*)(((uint8_t*)handler) + sizeof(struct screenshot_cb_handler));
//  handler->path = path;
//  strcpy(cb_path , checkname);
//
//	NriMemoryDesc memoryDesc = {0};
//	const NriTextureDesc* textureDesc = rsh.nri.coreI.GetTextureDesc(cmd->textureBuffers.colorTexture);
//	rsh.nri.coreI.GetTextureMemoryDesc(rsh.nri.device, textureDesc, NriMemoryLocation_HOST_READBACK, &memoryDesc);
//
//  const struct base_format_def_s* formatDef = R_BaseFormatDef(R_FromNRIFormat(textureDesc->format));
//	struct post_frame_handler_s* frameHandler = &cmd->postFrameHandlers[cmd->numPostFrameHandlers++];
//	NriBufferDesc bufferDesc = { .size = memoryDesc.size };
//	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.CreateBuffer( rsh.nri.device, &bufferDesc, &handler->buffer) );
//	
//	NriAllocateMemoryDesc allocateMemoryDesc = {0};
//	allocateMemoryDesc.size = memoryDesc.size;
//	allocateMemoryDesc.type = memoryDesc.type;
//	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.AllocateMemory( rsh.nri.device, &allocateMemoryDesc, &handler->memory) );
//	NriBufferMemoryBindingDesc bindBufferDesc = {
//		.memory = handler->memory,
//		.buffer = handler->buffer,
//	};
//	frameHandler->handler = __RF_Screensot_CB;
//	frameHandler->self = handler;
//
//	NRI_ABORT_ON_FAILURE( rsh.nri.coreI.BindBufferMemory( rsh.nri.device, &bindBufferDesc, 1 ) );
//
//	const uint64_t rowPitch = textureDesc->width * RT_BlockSize(formatDef);
//	const NriTextureDataLayoutDesc dstLayoutDesc = {
//		.offset = 0,
//		.rowPitch = rowPitch,
//		.slicePitch = rowPitch * textureDesc->height 
//	};
//	handler->dstLayoutDesc = dstLayoutDesc;
//	handler->textureDesc = textureDesc;
//	const NriTextureRegionDesc textureRegionDesc = {
//		.x = 0,
//		.y = 0,
//		.z = 0,
//		.width = textureDesc->width,
//		.height = textureDesc->height,
//		.depth = textureDesc->depth,
//		.mipOffset = 0,
//		.layerOffset = 0
//	};
//	{	
//		NriTextureBarrierDesc transitionBarriers = { 0 };
//		transitionBarriers.texture = cmd->textureBuffers.colorTexture;
//		transitionBarriers.before = (NriAccessLayoutStage){	
//			NriAccessBits_COLOR_ATTACHMENT, 
//			NriLayout_COLOR_ATTACHMENT 
//		};
//		transitionBarriers.after = (NriAccessLayoutStage){	
//			.layout = NriLayout_COPY_SOURCE, 
//			.access = NriAccessBits_COPY_SOURCE, 
//			.stages = NriStageBits_COPY 
//		};
//
//		NriBarrierGroupDesc barrierGroupDesc = { 0 };
//		barrierGroupDesc.textureNum = 1;
//		barrierGroupDesc.textures = &transitionBarriers;
//		rsh.nri.coreI.CmdBarrier( cmd->cmd, &barrierGroupDesc );
//	}
//	rsh.nri.coreI.CmdReadbackTextureToBuffer(cmd->cmd, handler->buffer, &dstLayoutDesc, cmd->textureBuffers.colorTexture, &textureRegionDesc);
//	{	
//		NriTextureBarrierDesc transitionBarriers = { 0 };
//		transitionBarriers.texture = cmd->textureBuffers.colorTexture;
//		transitionBarriers.before = (NriAccessLayoutStage){	
//			.layout = NriLayout_COPY_SOURCE, 
//			.access = NriAccessBits_COPY_SOURCE, 
//			.stages = NriStageBits_COPY 
//		};
//		transitionBarriers.after = (NriAccessLayoutStage){	
//			NriAccessBits_COLOR_ATTACHMENT, 
//			NriLayout_COLOR_ATTACHMENT 
//		};
//
//		NriBarrierGroupDesc barrierGroupDesc = { 0 };
//		barrierGroupDesc.textureNum = 1;
//		barrierGroupDesc.textures = &transitionBarriers;
//		rsh.nri.coreI.CmdBarrier( cmd->cmd, &barrierGroupDesc );
//	}
//
//}
//
//#endif 
