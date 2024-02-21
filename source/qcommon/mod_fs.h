#ifndef _FS_MODULE_H
#define _FS_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret ( *name##Fn )( __VA_ARGS__ );    \
	ret name( __VA_ARGS__ );

DECLARE_TYPEDEF_METHOD( const char *, FS_BaseGameDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_GameDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_WriteDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_CacheDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_SecureDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_MediaDirectory, fs_mediatype_t type );
DECLARE_TYPEDEF_METHOD( const char *, FS_DownloadsDirectory, void );
DECLARE_TYPEDEF_METHOD( const char *, FS_RuntimeDirectory, void );

DECLARE_TYPEDEF_METHOD( int, FS_GetGameDirectoryList, char *buf, size_t bufsize );
DECLARE_TYPEDEF_METHOD( int, FS_GetExplicitPurePakList, char ***paknames );
DECLARE_TYPEDEF_METHOD( bool, FS_IsExplicitPurePak, const char *pakname, bool *wrongver );

DECLARE_TYPEDEF_METHOD( int, FS_Read, void *buffer, size_t len, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Print, int file, const char *msg );
DECLARE_TYPEDEF_METHOD( int, FS_Printf, int file, const char *format, ... );
DECLARE_TYPEDEF_METHOD( int, FS_Write, const void *buffer, size_t len, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Tell, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Seek, int file, int offset, int whence );
DECLARE_TYPEDEF_METHOD( int, FS_Eof, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Flush, int file );
DECLARE_TYPEDEF_METHOD( bool, FS_IsUrl, const char *url );
DECLARE_TYPEDEF_METHOD( int, FS_FileNo, int file, size_t *offset );

// handling of absolute filenames
// only to be used if necessary (library not supporting custom file handling functions etc.)
DECLARE_TYPEDEF_METHOD( void, FS_CreateAbsolutePath, const char *path );
DECLARE_TYPEDEF_METHOD( const char *, FS_AbsoluteNameForFile, const char *filename );
DECLARE_TYPEDEF_METHOD( const char *, FS_AbsoluteNameForBaseFile, const char *filename );
DECLARE_TYPEDEF_METHOD( void, FS_AddExtraPK3Directory, const char *path );

DECLARE_TYPEDEF_METHOD( int, FS_FOpenFile, const char *filename, int *filenum, int mode );
DECLARE_TYPEDEF_METHOD( int, FS_FOpenBaseFile, const char *filename, int *filenum, int mode );
DECLARE_TYPEDEF_METHOD( int, FS_FOpenAbsoluteFile, const char *filename, int *filenum, int mode );
DECLARE_TYPEDEF_METHOD( void, FS_FCloseFile, int file );

// file loading
DECLARE_TYPEDEF_METHOD( int, FS_LoadFileExt, const char *path, int flags, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( int, FS_LoadBaseFileExt, const char *path, int flags, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, FS_FreeFile, void *buffer );
DECLARE_TYPEDEF_METHOD( void, FS_FreeBaseFile, void *buffer );
#define FS_LoadFile( path, buffer, stack, stacksize ) FS_LoadFileExt( path, 0, buffer, stack, stacksize, __FILE__, __LINE__ )
#define FS_LoadBaseFile( path, buffer, stack, stacksize ) FS_LoadBaseFileExt( path, 0, buffer, stack, stacksize, __FILE__, __LINE__ )
#define FS_LoadCacheFile( path, buffer, stack, stacksize ) FS_LoadFileExt( path, FS_CACHE, buffer, stack, stacksize, __FILE__, __LINE__ )

// util functions
DECLARE_TYPEDEF_METHOD( bool, FS_CopyFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_CopyBaseFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_ExtractFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_MoveFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_MoveBaseFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_MoveCacheFile, const char *src, const char *dst );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveFile, const char *filename );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveBaseFile, const char *filename );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveAbsoluteFile, const char *filename );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveDirectory, const char *dirname );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveBaseDirectory, const char *dirname );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveAbsoluteDirectory, const char *dirname );
DECLARE_TYPEDEF_METHOD( unsigned, FS_ChecksumAbsoluteFile, const char *filename );
DECLARE_TYPEDEF_METHOD( unsigned, FS_ChecksumBaseFile, const char *filename, bool ignorePakChecksum );
DECLARE_TYPEDEF_METHOD( bool, FS_CheckPakExtension, const char *filename );
DECLARE_TYPEDEF_METHOD( bool, FS_PakFileExists, const char *packfilename );

DECLARE_TYPEDEF_METHOD( time_t, FS_FileMTime, const char *filename );
DECLARE_TYPEDEF_METHOD( time_t, FS_BaseFileMTime, const char *filename );

// // only for game files
DECLARE_TYPEDEF_METHOD( const char *, FS_FirstExtension, const char *filename, const char *extensions[], int num_extensions );
DECLARE_TYPEDEF_METHOD( const char *, FS_PakNameForFile, const char *filename );
DECLARE_TYPEDEF_METHOD( bool, FS_IsPureFile, const char *pakname );
DECLARE_TYPEDEF_METHOD( const char *, FS_FileManifest, const char *filename );
DECLARE_TYPEDEF_METHOD( const char *, FS_BaseNameForFile, const char *filename );

DECLARE_TYPEDEF_METHOD( int, FS_GetFileList, const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end );
DECLARE_TYPEDEF_METHOD( int, FS_GetFileListExt, const char *dir, const char *extension, char *buf, size_t *bufsize, int start, int end );

// // only for base files
DECLARE_TYPEDEF_METHOD( bool, FS_IsPakValid, const char *filename, unsigned *checksum );
DECLARE_TYPEDEF_METHOD( bool, FS_AddPurePak, unsigned checksum );
DECLARE_TYPEDEF_METHOD( void, FS_RemovePurePaks, void );

DECLARE_TYPEDEF_METHOD( void, FS_AddFileToMedia, const char *filename );

#undef DECLARE_TYPEDEF_METHOD

struct fs_import_s {
	FS_GameDirectoryFn FS_GameDirectory;
	FS_BaseGameDirectoryFn FS_BaseGameDirectory;
	FS_WriteDirectoryFn FS_WriteDirectory;
	FS_CacheDirectoryFn FS_CacheDirectory;
	FS_SecureDirectoryFn FS_SecureDirectory;
	FS_MediaDirectoryFn FS_MediaDirectory;
	FS_DownloadsDirectoryFn FS_DownloadsDirectory;
	FS_RuntimeDirectoryFn FS_RuntimeDirectory;
	FS_GetGameDirectoryListFn FS_GetGameDirectoryList;
	FS_GetExplicitPurePakListFn FS_GetExplicitPurePakList;
	FS_IsExplicitPurePakFn FS_IsExplicitPurePak;
	FS_ReadFn FS_Read;
	FS_PrintFn FS_Print;
	FS_PrintfFn FS_Printf;
	FS_WriteFn FS_Write;
	FS_TellFn FS_Tell;
	FS_SeekFn FS_Seek;
	FS_EofFn FS_Eof;
	FS_FlushFn FS_Flush;
	FS_IsUrlFn FS_IsUrl;
	FS_FileNoFn FS_FileNo;
	FS_FOpenFileFn FS_FOpenFile;	
	FS_CreateAbsolutePathFn FS_CreateAbsolutePath;
	FS_AbsoluteNameForFileFn FS_AbsoluteNameForFile;
	FS_AbsoluteNameForBaseFileFn FS_AbsoluteNameForBaseFile;
	FS_AddExtraPK3DirectoryFn FS_AddExtraPK3Directory;
	FS_LoadFileExtFn FS_LoadFileExt;
	FS_LoadBaseFileExtFn FS_LoadBaseFileExt;
	FS_FreeFileFn FS_FreeFile;
	FS_FreeBaseFileFn FS_FreeBaseFile;
	FS_CopyFileFn FS_CopyFile;
	FS_CopyBaseFileFn FS_CopyBaseFile;
	FS_ExtractFileFn FS_ExtractFile;
	FS_MoveFileFn FS_MoveFile;
	FS_MoveBaseFileFn FS_MoveBaseFile;
	FS_MoveCacheFileFn FS_MoveCacheFile;
	FS_RemoveFileFn FS_RemoveFile;
	FS_RemoveBaseFileFn FS_RemoveBaseFile;
	FS_RemoveAbsoluteFileFn FS_RemoveAbsoluteFile;
	FS_RemoveDirectoryFn FS_RemoveDirectory;
	FS_RemoveBaseDirectoryFn FS_RemoveBaseDirectory;
	FS_RemoveAbsoluteDirectoryFn FS_RemoveAbsoluteDirectory;
	FS_ChecksumAbsoluteFileFn FS_ChecksumAbsoluteFile;
	FS_ChecksumBaseFileFn FS_ChecksumBaseFile;
	FS_CheckPakExtensionFn FS_CheckPakExtension;
	FS_PakFileExistsFn FS_PakFileExists;
	FS_FileMTimeFn FS_FileMTime;
	FS_BaseFileMTimeFn FS_BaseFileMTime;
	FS_FirstExtensionFn FS_FirstExtension;
	FS_PakNameForFileFn FS_PakNameForFile;
	FS_IsPureFileFn FS_IsPureFile;
	FS_FileManifestFn FS_FileManifest;
	FS_BaseNameForFileFn FS_BaseNameForFile;
	FS_GetFileListFn FS_GetFileList;
	FS_GetFileListExtFn FS_GetFileListExt;
	FS_IsPakValidFn FS_IsPakValid;
	FS_AddPurePakFn FS_AddPurePak;
	FS_RemovePurePaksFn FS_RemovePurePaks;
	FS_AddFileToMediaFn FS_AddFileToMedia;
	FS_FOpenAbsoluteFileFn FS_FOpenAbsoluteFile;
	FS_FCloseFileFn FS_FCloseFile;
	FS_FOpenBaseFileFn FS_FOpenBaseFile;
};
static const struct fs_import_s default_fs_imports_s = {
	.FS_FOpenBaseFile = FS_FOpenBaseFile,
	.FS_FCloseFile = FS_FCloseFile,
	.FS_FOpenFile = FS_FOpenFile,
	.FS_FOpenAbsoluteFile = FS_FOpenAbsoluteFile,
	.FS_GameDirectory = FS_GameDirectory,
	.FS_BaseGameDirectory = FS_BaseGameDirectory,
	.FS_WriteDirectory = FS_WriteDirectory,
	.FS_CacheDirectory = FS_CacheDirectory,
	.FS_SecureDirectory = FS_SecureDirectory,
	.FS_MediaDirectory = FS_MediaDirectory,
	.FS_DownloadsDirectory = FS_DownloadsDirectory,
	.FS_RuntimeDirectory = FS_RuntimeDirectory,
	.FS_GetGameDirectoryList = FS_GetGameDirectoryList,
	.FS_GetExplicitPurePakList = FS_GetExplicitPurePakList,
	.FS_IsExplicitPurePak = FS_IsExplicitPurePak,
	.FS_Read = FS_Read,
	.FS_Print = FS_Print,
	.FS_Printf = FS_Printf,
	.FS_Write = FS_Write,
	.FS_Tell = FS_Tell,
	.FS_Seek = FS_Seek,
	.FS_Eof = FS_Eof,
	.FS_Flush = FS_Flush,
	.FS_IsUrl = FS_IsUrl,
	.FS_FileNo = FS_FileNo,
	.FS_CreateAbsolutePath = FS_CreateAbsolutePath,
	.FS_AbsoluteNameForFile = FS_AbsoluteNameForFile,
	.FS_AbsoluteNameForBaseFile = FS_AbsoluteNameForBaseFile,
	.FS_AddExtraPK3Directory = FS_AddExtraPK3Directory,
	.FS_LoadFileExt = FS_LoadFileExt,
	.FS_LoadBaseFileExt = FS_LoadBaseFileExt,
	.FS_FreeFile = FS_FreeFile,
	.FS_FreeBaseFile = FS_FreeBaseFile,
	.FS_CopyFile = FS_CopyFile,
	.FS_CopyBaseFile = FS_CopyBaseFile,
	.FS_ExtractFile = FS_ExtractFile,
	.FS_MoveFile = FS_MoveFile,
	.FS_MoveBaseFile = FS_MoveBaseFile,
	.FS_MoveCacheFile = FS_MoveCacheFile,
	.FS_RemoveFile = FS_RemoveFile,
	.FS_RemoveBaseFile = FS_RemoveBaseFile,
	.FS_RemoveAbsoluteFile = FS_RemoveAbsoluteFile,
	.FS_RemoveDirectory = FS_RemoveDirectory,
	.FS_RemoveBaseDirectory = FS_RemoveBaseDirectory,
	.FS_RemoveAbsoluteDirectory = FS_RemoveAbsoluteDirectory,
	.FS_ChecksumAbsoluteFile = FS_ChecksumAbsoluteFile,
	.FS_ChecksumBaseFile = FS_ChecksumBaseFile,
	.FS_CheckPakExtension = FS_CheckPakExtension,
	.FS_PakFileExists = FS_PakFileExists,
	.FS_FileMTime = FS_FileMTime,
	.FS_BaseFileMTime = FS_BaseFileMTime,
	.FS_FirstExtension = FS_FirstExtension,
	.FS_PakNameForFile = FS_PakNameForFile,
	.FS_IsPureFile = FS_IsPureFile,
	.FS_FileManifest = FS_FileManifest,
	.FS_BaseNameForFile = FS_BaseNameForFile,
	.FS_GetFileList = FS_GetFileList,
	.FS_GetFileListExt = FS_GetFileListExt,
	.FS_IsPakValid = FS_IsPakValid,
	.FS_AddPurePak = FS_AddPurePak,
	.FS_RemovePurePaks = FS_RemovePurePaks,
	.FS_AddFileToMedia = FS_AddFileToMedia,
};

#if FS_DEFINE_INTERFACE_IMPL
extern struct fs_import_s fs_import;
const char * FS_GameDirectory(void){ return fs_import.FS_GameDirectory();}
const char * FS_BaseGameDirectory(void){ return fs_import.FS_BaseGameDirectory();}
const char * FS_WriteDirectory(void ){ return fs_import.FS_WriteDirectory();}
const char * FS_CacheDirectory(void){ return fs_import.FS_CacheDirectory();}
const char * FS_SecureDirectory(void){ return fs_import.FS_SecureDirectory();}
const char * FS_MediaDirectory(fs_mediatype_t type ){ return fs_import.FS_MediaDirectory(type);}
const char * FS_DownloadsDirectory(void ){ return fs_import.FS_DownloadsDirectory();}
const char * FS_RuntimeDirectory(void ){ return fs_import.FS_RuntimeDirectory();}
int FS_GetGameDirectoryList( char *buf, size_t bufsize ){ return fs_import.FS_GetGameDirectoryList(buf, bufsize);}
int FS_GetExplicitPurePakList( char ***paknames ){ return fs_import.FS_GetExplicitPurePakList(paknames);}
bool FS_IsExplicitPurePak( const char *pakname, bool *wrongver ){ return fs_import.FS_IsExplicitPurePak(pakname, wrongver);}
int FS_Read( void *buffer, size_t len, int file ){ return fs_import.FS_Read(buffer, len, file);}
int FS_Print( int file, const char *msg ){ return fs_import.FS_Print(file, msg);}
int FS_Printf( int file, const char *format, ...){ 
	int ret;
	va_list args;
	va_start(args, format);
	ret = fs_import.FS_Printf(file, format, args);
	va_end(args);
	return ret;
}
int FS_Write( const void *buffer, size_t len, int file ){ return fs_import.FS_Write(buffer, len, file);}
int FS_Tell( int file ){ return fs_import.FS_Tell(file);}
int FS_Seek( int file, int offset, int whence ){ return fs_import.FS_Seek(file, offset, whence);}
int FS_Eof( int file ){ return fs_import.FS_Eof(file);}
int FS_Flush( int file ){ return fs_import.FS_Flush(file);}
bool FS_IsUrl( const char *url ){ return fs_import.FS_IsUrl(url);}
int FS_FileNo( int file, size_t *offset ){ return fs_import.FS_FileNo(file, offset);}
void FS_CreateAbsolutePath( const char *path ){ fs_import.FS_CreateAbsolutePath(path);}
const char * FS_AbsoluteNameForFile(const char *filename ){ return fs_import.FS_AbsoluteNameForFile(filename);}
const char * FS_AbsoluteNameForBaseFile(const char *filename ){ return fs_import.FS_AbsoluteNameForBaseFile(filename);}
void FS_AddExtraPK3Directory(const char *path ){ fs_import.FS_AddExtraPK3Directory(path);}
int FS_FOpenFile(const char *filename, int *filenum, int mode ){ return fs_import.FS_FOpenFile(filename, filenum, mode);}
int FS_FOpenBaseFile(const char *filename, int *filenum, int mode ){ return fs_import.FS_FOpenBaseFile(filename, filenum, mode);}
int FS_FOpenAbsoluteFile(const char *filename, int *filenum, int mode ){ return fs_import.FS_FOpenAbsoluteFile(filename, filenum, mode);}
void FS_FCloseFile(int file ){ fs_import.FS_FCloseFile(file);}
int FS_LoadFileExt(const char *path, int flags, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline ){ return fs_import.FS_LoadFileExt(path, flags, buffer, stack, stackSize, filename, fileline);}
int FS_LoadBaseFileExt(const char *path, int flags, void **buffer, void *stack, size_t stackSize, const char *filename, int fileline ){ return fs_import.FS_LoadBaseFileExt(path, flags, buffer, stack, stackSize, filename, fileline);}
void FS_FreeFile(void *buffer ){ fs_import.FS_FreeFile(buffer);}
void FS_FreeBaseFile(void *buffer ){ fs_import.FS_FreeBaseFile(buffer);}
bool FS_CopyFile(const char *src, const char *dst ){ return fs_import.FS_CopyFile(src, dst);}
bool FS_CopyBaseFile(const char *src, const char *dst ){ return fs_import.FS_CopyBaseFile(src, dst);}
bool FS_ExtractFile(const char *src, const char *dst ){ return fs_import.FS_ExtractFile(src, dst);}
bool FS_MoveFile(const char *src, const char *dst ){ return fs_import.FS_MoveFile(src, dst);}
bool FS_MoveBaseFile(const char *src, const char *dst ){ return fs_import.FS_MoveBaseFile(src, dst);}
bool FS_MoveCacheFile(const char *src, const char *dst ){ return fs_import.FS_MoveCacheFile(src, dst);}
bool FS_RemoveFile(const char *filename ){ return fs_import.FS_RemoveFile(filename);}
bool FS_RemoveBaseFile(const char *filename ){ return fs_import.FS_RemoveBaseFile(filename);}
bool FS_RemoveAbsoluteFile(const char *filename ){ return fs_import.FS_RemoveAbsoluteFile(filename);}
bool FS_RemoveDirectory(const char *dirname ){ return fs_import.FS_RemoveDirectory(dirname);}
bool FS_RemoveBaseDirectory(const char *dirname ){ return fs_import.FS_RemoveBaseDirectory(dirname);}
bool FS_RemoveAbsoluteDirectory(const char *dirname ){ return fs_import.FS_RemoveAbsoluteDirectory(dirname);}
unsigned FS_ChecksumAbsoluteFile(const char *filename ){ return fs_import.FS_ChecksumAbsoluteFile(filename);}
unsigned FS_ChecksumBaseFile(const char *filename, bool ignorePakChecksum ){ return fs_import.FS_ChecksumBaseFile(filename, ignorePakChecksum);}
bool FS_CheckPakExtension(const char *filename ){ return fs_import.FS_CheckPakExtension(filename);}
bool FS_PakFileExists(const char *packfilename ){ return fs_import.FS_PakFileExists(packfilename);}
time_t FS_FileMTime(const char *filename ){ return fs_import.FS_FileMTime(filename);}
time_t FS_BaseFileMTime(const char *filename ){ return fs_import.FS_BaseFileMTime(filename);}
const char * FS_FirstExtension(const char *filename, const char *extensions[], int num_extensions ){ return fs_import.FS_FirstExtension(filename, extensions, num_extensions);}
const char * FS_PakNameForFile(const char *filename ){ return fs_import.FS_PakNameForFile(filename);}
bool FS_IsPureFile(const char *pakname ){ return fs_import.FS_IsPureFile(pakname);}
const char * FS_FileManifest(const char *filename ){ return fs_import.FS_FileManifest(filename);}
const char * FS_BaseNameForFile(const char *filename ){ return fs_import.FS_BaseNameForFile(filename);}
int FS_GetFileList(const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end ){ return fs_import.FS_GetFileList(dir, extension, buf, bufsize, start, end);}
int FS_GetFileListExt(const char *dir, const char *extension, char *buf, size_t *bufsize, int start, int end ){ return fs_import.FS_GetFileListExt(dir, extension, buf, bufsize, start, end);}
bool FS_IsPakValid(const char *filename, unsigned *checksum ){ return fs_import.FS_IsPakValid(filename, checksum);}
bool FS_AddPurePak(unsigned checksum ){ return fs_import.FS_AddPurePak(checksum);}
void FS_RemovePurePaks(void ){ fs_import.FS_RemovePurePaks();}
void FS_AddFileToMedia(const char *filename ){ fs_import.FS_AddFileToMedia(filename);}
#endif

#endif

