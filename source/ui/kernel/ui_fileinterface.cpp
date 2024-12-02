/*
 * UI_FileInterface.cpp
 *
 *  Created on: 25.6.2011
 *      Author: hc
 */

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_fileinterface.h"
#include "../qcommon/mod_fs.h"

namespace WSWUI
{

UI_FileInterface::UI_FileInterface() : Rml::Core::FileInterface()
{
	fileSizeMap.clear();

	// TODO Auto-generated constructor stub
}

UI_FileInterface::~UI_FileInterface()
{
}

Rml::FileHandle UI_FileInterface::Open(const Rml::String & path)
{
	int filenum = 0;
	int length = -1;
	Rml::Core::URL url( path );
	Rml::String protocol = url.GetProtocol();
	bool cache = protocol == "cache";

	// local
	if( protocol.Empty() || protocol == "file" || cache ) {
		Rml::String path2( url.GetHost() + "/" + url.GetPathedFileName() );
		while( path2[0] == '/' ) {
			path2.Erase( 0, 1 );
		}
		length = FS_FOpenFile( path2.CString(), &filenum, FS_READ | (cache ? FS_CACHE : 0) );
	}
	else if( protocol == "http" ) {
		// allow blocking download of remote resources
		length = FS_FOpenFile( path.CString(), &filenum, FS_READ );
	}

	if( length == -1 )
		return 0;

	// cache file length
	fileSizeMap[filenum] = length;

	// Com_Printf("UI_FileInterface opened %s\n", path2.CString() );
	return static_cast<Rml::FileHandle>( filenum );
}

void UI_FileInterface::Close(Rml::FileHandle file)
{
	if( file != 0 ) {
		int filenum = static_cast<int>( file );

		fileSizeMap.erase( filenum );
		FS_FCloseFile( filenum );
	}
}

size_t UI_FileInterface::Read(void *buffer, size_t size, Rml::FileHandle file)
{
	return FS_Read( buffer, size, static_cast<int>( file ) );
}

bool UI_FileInterface::Seek(Rml::FileHandle file, long  offset, int origin)
{
	if( origin == SEEK_SET )
		origin = FS_SEEK_SET;
	else if( origin == SEEK_END )
		origin = FS_SEEK_END;
	else if( origin == SEEK_CUR )
		origin = FS_SEEK_CUR;
	else
		return false;

	return ( FS_Seek( static_cast<int>( file ), offset, origin ) != -1 );
}

size_t UI_FileInterface::Tell(Rml::FileHandle file)
{
	return FS_Tell( static_cast<int>( file ) );
}

size_t UI_FileInterface::Length(Rml::FileHandle file)
{
	int filenum = static_cast<int>( file );
	fileSizeMap_t::iterator it = fileSizeMap.find( filenum );

	// assertion failure here means that Length was called without preceeding Open call
	assert( it != fileSizeMap.end() );
	if( it == fileSizeMap.end() ) {
		return 0;
	}
	return fileSizeMap[filenum];
}

}
