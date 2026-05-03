#ifndef G_GAME_MODULE_H
#define G_GAME_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_collision.h"
#include "../gameshared/q_comref.h"
#include "../gameshared/q_cvar.h"
#include "../gameshared/q_dynvar.h"

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

// console / errors
DECLARE_TYPEDEF_METHOD( void, Print, const char *msg );
DECLARE_TYPEDEF_METHOD( void, Error, const char *msg );

// server -> client commands
DECLARE_TYPEDEF_METHOD( void, GameCmd, struct edict_s *ent, const char *cmd );
DECLARE_TYPEDEF_METHOD( void, ServerCmd, struct edict_s *ent, const char *cmd );

// configstrings / asset indices
DECLARE_TYPEDEF_METHOD( void, ConfigString, int num, const char *string );
DECLARE_TYPEDEF_METHOD( const char *, GetConfigString, int num );
DECLARE_TYPEDEF_METHOD( void, PureSound, const char *name );
DECLARE_TYPEDEF_METHOD( void, PureModel, const char *name );
DECLARE_TYPEDEF_METHOD( int, ModelIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, SoundIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, ImageIndex, const char *name );
DECLARE_TYPEDEF_METHOD( int, SkinIndex, const char *name );

// clock
DECLARE_TYPEDEF_METHOD( unsigned int, Milliseconds, void );

// pvs / collision
DECLARE_TYPEDEF_METHOD( bool, inPVS, const vec3_t p1, const vec3_t p2 );
DECLARE_TYPEDEF_METHOD( int, CM_TransformedPointContents, vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles );
DECLARE_TYPEDEF_METHOD( void, CM_TransformedBoxTrace, trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles );
DECLARE_TYPEDEF_METHOD( void, CM_RoundUpToHullSize, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel );
DECLARE_TYPEDEF_METHOD( int, CM_NumInlineModels, void );
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_InlineModel, int num );
DECLARE_TYPEDEF_METHOD( void, CM_InlineModelBounds, struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs );
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_ModelForBBox, vec3_t mins, vec3_t maxs );
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_OctagonModelForBBox, vec3_t mins, vec3_t maxs );
DECLARE_TYPEDEF_METHOD( void, CM_SetAreaPortalState, int area, int otherarea, bool open );
DECLARE_TYPEDEF_METHOD( bool, CM_AreasConnected, int area1, int area2 );
DECLARE_TYPEDEF_METHOD( int, CM_BoxLeafnums, vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode );
DECLARE_TYPEDEF_METHOD( int, CM_LeafCluster, int leafnum );
DECLARE_TYPEDEF_METHOD( int, CM_LeafArea, int leafnum );

// memory
DECLARE_TYPEDEF_METHOD( void *, MemAlloc, size_t size, const char *filename, int fileline );
DECLARE_TYPEDEF_METHOD( void, MemFree, void *data, const char *filename, int fileline );

// dynvars
DECLARE_TYPEDEF_METHOD( dynvar_t *, Dynvar_Create, const char *name, bool console, dynvar_getter_f getter, dynvar_setter_f setter );
DECLARE_TYPEDEF_METHOD( void, Dynvar_Destroy, dynvar_t *dynvar );
DECLARE_TYPEDEF_METHOD( dynvar_t *, Dynvar_Lookup, const char *name );
DECLARE_TYPEDEF_METHOD( const char *, Dynvar_GetName, dynvar_t *dynvar );
DECLARE_TYPEDEF_METHOD( dynvar_get_status_t, Dynvar_GetValue, dynvar_t *dynvar, void **value );
DECLARE_TYPEDEF_METHOD( dynvar_set_status_t, Dynvar_SetValue, dynvar_t *dynvar, void *value );
DECLARE_TYPEDEF_METHOD( void, Dynvar_AddListener, dynvar_t *dynvar, dynvar_listener_f listener );
DECLARE_TYPEDEF_METHOD( void, Dynvar_RemoveListener, dynvar_t *dynvar, dynvar_listener_f listener );

// cvars
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_Get, const char *name, const char *value, int flags );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_Set, const char *name, const char *value );
DECLARE_TYPEDEF_METHOD( void, Cvar_SetValue, const char *name, float value );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_ForceSet, const char *name, const char *value );
DECLARE_TYPEDEF_METHOD( float, Cvar_Value, const char *name );
DECLARE_TYPEDEF_METHOD( const char *, Cvar_String, const char *name );

// commands
DECLARE_TYPEDEF_METHOD( int, Cmd_Argc, void );
DECLARE_TYPEDEF_METHOD( char *, Cmd_Argv, int arg );
DECLARE_TYPEDEF_METHOD( char *, Cmd_Args, void );
DECLARE_TYPEDEF_METHOD( void, Cmd_AddCommand, const char *name, void ( *cmd )( void ) );
DECLARE_TYPEDEF_METHOD( void, Cmd_RemoveCommand, const char *cmd_name );
DECLARE_TYPEDEF_METHOD( void, Cmd_ExecuteText, int exec_when, const char *text );
DECLARE_TYPEDEF_METHOD( void, Cbuf_Execute, void );

// filesystem
DECLARE_TYPEDEF_METHOD( int, FS_FOpenFile, const char *filename, int *filenum, int mode );
DECLARE_TYPEDEF_METHOD( int, FS_Read, void *buffer, size_t len, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Write, const void *buffer, size_t len, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Print, int file, const char *msg );
DECLARE_TYPEDEF_METHOD( int, FS_Tell, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Seek, int file, int offset, int whence );
DECLARE_TYPEDEF_METHOD( int, FS_Eof, int file );
DECLARE_TYPEDEF_METHOD( int, FS_Flush, int file );
DECLARE_TYPEDEF_METHOD( void, FS_FCloseFile, int file );
DECLARE_TYPEDEF_METHOD( bool, FS_RemoveFile, const char *filename );
DECLARE_TYPEDEF_METHOD( int, FS_GetFileList, const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end );
DECLARE_TYPEDEF_METHOD( const char *, FS_FirstExtension, const char *filename, const char *extensions[], int num_extensions );
DECLARE_TYPEDEF_METHOD( bool, FS_MoveFile, const char *src, const char *dst );

// maplist
DECLARE_TYPEDEF_METHOD( bool, ML_Update, void );
DECLARE_TYPEDEF_METHOD( bool, ML_FilenameExists, const char *filename );
DECLARE_TYPEDEF_METHOD( const char *, ML_GetFullname, const char *filename );
DECLARE_TYPEDEF_METHOD( size_t, ML_GetMapByNum, int num, char *out, size_t size );

// fake clients / clients
DECLARE_TYPEDEF_METHOD( int, FakeClientConnect, char *fakeUserinfo, char *fakeSocketType, const char *fakeIP );
DECLARE_TYPEDEF_METHOD( int, GetClientState, int numClient );
DECLARE_TYPEDEF_METHOD( void, ExecuteClientThinks, int clientNum );
DECLARE_TYPEDEF_METHOD( void, DropClient, struct edict_s *ent, int type, const char *message );
DECLARE_TYPEDEF_METHOD( void, LocateEntities, struct edict_s *edicts, int edict_size, int num_edicts, int max_edicts );

// angelscript
DECLARE_TYPEDEF_METHOD( struct angelwrap_api_s *, asGetAngelExport, void );

// matchmaking
DECLARE_TYPEDEF_METHOD( struct stat_query_api_s *, GetStatQueryAPI, void );
DECLARE_TYPEDEF_METHOD( void, MM_SendQuery, struct stat_query_s *query );
DECLARE_TYPEDEF_METHOD( void, MM_GameState, bool state );

#undef DECLARE_TYPEDEF_METHOD

struct game_import_s {
	PrintFn Print;
	ErrorFn Error;
	GameCmdFn GameCmd;
	ServerCmdFn ServerCmd;
	ConfigStringFn ConfigString;
	GetConfigStringFn GetConfigString;
	PureSoundFn PureSound;
	PureModelFn PureModel;
	ModelIndexFn ModelIndex;
	SoundIndexFn SoundIndex;
	ImageIndexFn ImageIndex;
	SkinIndexFn SkinIndex;
	MillisecondsFn Milliseconds;
	inPVSFn inPVS;
	CM_TransformedPointContentsFn CM_TransformedPointContents;
	CM_TransformedBoxTraceFn CM_TransformedBoxTrace;
	CM_RoundUpToHullSizeFn CM_RoundUpToHullSize;
	CM_NumInlineModelsFn CM_NumInlineModels;
	CM_InlineModelFn CM_InlineModel;
	CM_InlineModelBoundsFn CM_InlineModelBounds;
	CM_ModelForBBoxFn CM_ModelForBBox;
	CM_OctagonModelForBBoxFn CM_OctagonModelForBBox;
	CM_SetAreaPortalStateFn CM_SetAreaPortalState;
	CM_AreasConnectedFn CM_AreasConnected;
	CM_BoxLeafnumsFn CM_BoxLeafnums;
	CM_LeafClusterFn CM_LeafCluster;
	CM_LeafAreaFn CM_LeafArea;
	MemAllocFn MemAlloc;
	MemFreeFn MemFree;
	Dynvar_CreateFn Dynvar_Create;
	Dynvar_DestroyFn Dynvar_Destroy;
	Dynvar_LookupFn Dynvar_Lookup;
	Dynvar_GetNameFn Dynvar_GetName;
	Dynvar_GetValueFn Dynvar_GetValue;
	Dynvar_SetValueFn Dynvar_SetValue;
	Dynvar_AddListenerFn Dynvar_AddListener;
	Dynvar_RemoveListenerFn Dynvar_RemoveListener;
	Cvar_GetFn Cvar_Get;
	Cvar_SetFn Cvar_Set;
	Cvar_SetValueFn Cvar_SetValue;
	Cvar_ForceSetFn Cvar_ForceSet;
	Cvar_ValueFn Cvar_Value;
	Cvar_StringFn Cvar_String;
	Cmd_ArgcFn Cmd_Argc;
	Cmd_ArgvFn Cmd_Argv;
	Cmd_ArgsFn Cmd_Args;
	Cmd_AddCommandFn Cmd_AddCommand;
	Cmd_RemoveCommandFn Cmd_RemoveCommand;
	Cmd_ExecuteTextFn Cmd_ExecuteText;
	Cbuf_ExecuteFn Cbuf_Execute;
	FS_FOpenFileFn FS_FOpenFile;
	FS_ReadFn FS_Read;
	FS_WriteFn FS_Write;
	FS_PrintFn FS_Print;
	FS_TellFn FS_Tell;
	FS_SeekFn FS_Seek;
	FS_EofFn FS_Eof;
	FS_FlushFn FS_Flush;
	FS_FCloseFileFn FS_FCloseFile;
	FS_RemoveFileFn FS_RemoveFile;
	FS_GetFileListFn FS_GetFileList;
	FS_FirstExtensionFn FS_FirstExtension;
	FS_MoveFileFn FS_MoveFile;
	ML_UpdateFn ML_Update;
	ML_FilenameExistsFn ML_FilenameExists;
	ML_GetFullnameFn ML_GetFullname;
	ML_GetMapByNumFn ML_GetMapByNum;
	FakeClientConnectFn FakeClientConnect;
	GetClientStateFn GetClientState;
	ExecuteClientThinksFn ExecuteClientThinks;
	DropClientFn DropClient;
	LocateEntitiesFn LocateEntities;
	asGetAngelExportFn asGetAngelExport;
	GetStatQueryAPIFn GetStatQueryAPI;
	MM_SendQueryFn MM_SendQuery;
	MM_GameStateFn MM_GameState;
};

#if GAME_DEFINE_INTERFACE_IMPL
static struct game_import_s game_import;

void Print( const char *msg ) { game_import.Print( msg ); }
void Error( const char *msg ) { game_import.Error( msg ); }
void GameCmd( struct edict_s *ent, const char *cmd ) { game_import.GameCmd( ent, cmd ); }
void ServerCmd( struct edict_s *ent, const char *cmd ) { game_import.ServerCmd( ent, cmd ); }
void ConfigString( int num, const char *string ) { game_import.ConfigString( num, string ); }
const char *GetConfigString( int num ) { return game_import.GetConfigString( num ); }
void PureSound( const char *name ) { game_import.PureSound( name ); }
void PureModel( const char *name ) { game_import.PureModel( name ); }
int ModelIndex( const char *name ) { return game_import.ModelIndex( name ); }
int SoundIndex( const char *name ) { return game_import.SoundIndex( name ); }
int ImageIndex( const char *name ) { return game_import.ImageIndex( name ); }
int SkinIndex( const char *name ) { return game_import.SkinIndex( name ); }
unsigned int Milliseconds( void ) { return game_import.Milliseconds(); }
bool inPVS( const vec3_t p1, const vec3_t p2 ) { return game_import.inPVS( p1, p2 ); }
int CM_TransformedPointContents( vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles ) { return game_import.CM_TransformedPointContents( p, cmodel, origin, angles ); }
void CM_TransformedBoxTrace( trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles ) { game_import.CM_TransformedBoxTrace( tr, start, end, mins, maxs, cmodel, brushmask, origin, angles ); }
void CM_RoundUpToHullSize( vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel ) { game_import.CM_RoundUpToHullSize( mins, maxs, cmodel ); }
int CM_NumInlineModels( void ) { return game_import.CM_NumInlineModels(); }
struct cmodel_s *CM_InlineModel( int num ) { return game_import.CM_InlineModel( num ); }
void CM_InlineModelBounds( struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs ) { game_import.CM_InlineModelBounds( cmodel, mins, maxs ); }
struct cmodel_s *CM_ModelForBBox( vec3_t mins, vec3_t maxs ) { return game_import.CM_ModelForBBox( mins, maxs ); }
struct cmodel_s *CM_OctagonModelForBBox( vec3_t mins, vec3_t maxs ) { return game_import.CM_OctagonModelForBBox( mins, maxs ); }
void CM_SetAreaPortalState( int area, int otherarea, bool open ) { game_import.CM_SetAreaPortalState( area, otherarea, open ); }
bool CM_AreasConnected( int area1, int area2 ) { return game_import.CM_AreasConnected( area1, area2 ); }
int CM_BoxLeafnums( vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode ) { return game_import.CM_BoxLeafnums( mins, maxs, list, listsize, topnode ); }
int CM_LeafCluster( int leafnum ) { return game_import.CM_LeafCluster( leafnum ); }
int CM_LeafArea( int leafnum ) { return game_import.CM_LeafArea( leafnum ); }
void *MemAlloc( size_t size, const char *filename, int fileline ) { return game_import.MemAlloc( size, filename, fileline ); }
void MemFree( void *data, const char *filename, int fileline ) { game_import.MemFree( data, filename, fileline ); }
dynvar_t *Dynvar_Create( const char *name, bool console, dynvar_getter_f getter, dynvar_setter_f setter ) { return game_import.Dynvar_Create( name, console, getter, setter ); }
void Dynvar_Destroy( dynvar_t *dynvar ) { game_import.Dynvar_Destroy( dynvar ); }
dynvar_t *Dynvar_Lookup( const char *name ) { return game_import.Dynvar_Lookup( name ); }
const char *Dynvar_GetName( dynvar_t *dynvar ) { return game_import.Dynvar_GetName( dynvar ); }
dynvar_get_status_t Dynvar_GetValue( dynvar_t *dynvar, void **value ) { return game_import.Dynvar_GetValue( dynvar, value ); }
dynvar_set_status_t Dynvar_SetValue( dynvar_t *dynvar, void *value ) { return game_import.Dynvar_SetValue( dynvar, value ); }
void Dynvar_AddListener( dynvar_t *dynvar, dynvar_listener_f listener ) { game_import.Dynvar_AddListener( dynvar, listener ); }
void Dynvar_RemoveListener( dynvar_t *dynvar, dynvar_listener_f listener ) { game_import.Dynvar_RemoveListener( dynvar, listener ); }
cvar_t *Cvar_Get( const char *name, const char *value, int flags ) { return game_import.Cvar_Get( name, value, flags ); }
cvar_t *Cvar_Set( const char *name, const char *value ) { return game_import.Cvar_Set( name, value ); }
void Cvar_SetValue( const char *name, float value ) { game_import.Cvar_SetValue( name, value ); }
cvar_t *Cvar_ForceSet( const char *name, const char *value ) { return game_import.Cvar_ForceSet( name, value ); }
float Cvar_Value( const char *name ) { return game_import.Cvar_Value( name ); }
const char *Cvar_String( const char *name ) { return game_import.Cvar_String( name ); }
int Cmd_Argc( void ) { return game_import.Cmd_Argc(); }
char *Cmd_Argv( int arg ) { return game_import.Cmd_Argv( arg ); }
char *Cmd_Args( void ) { return game_import.Cmd_Args(); }
void Cmd_AddCommand( const char *name, void ( *cmd )( void ) ) { game_import.Cmd_AddCommand( name, cmd ); }
void Cmd_RemoveCommand( const char *cmd_name ) { game_import.Cmd_RemoveCommand( cmd_name ); }
void Cmd_ExecuteText( int exec_when, const char *text ) { game_import.Cmd_ExecuteText( exec_when, text ); }
void Cbuf_Execute( void ) { game_import.Cbuf_Execute(); }
int FS_FOpenFile( const char *filename, int *filenum, int mode ) { return game_import.FS_FOpenFile( filename, filenum, mode ); }
int FS_Read( void *buffer, size_t len, int file ) { return game_import.FS_Read( buffer, len, file ); }
int FS_Write( const void *buffer, size_t len, int file ) { return game_import.FS_Write( buffer, len, file ); }
int FS_Print( int file, const char *msg ) { return game_import.FS_Print( file, msg ); }
int FS_Tell( int file ) { return game_import.FS_Tell( file ); }
int FS_Seek( int file, int offset, int whence ) { return game_import.FS_Seek( file, offset, whence ); }
int FS_Eof( int file ) { return game_import.FS_Eof( file ); }
int FS_Flush( int file ) { return game_import.FS_Flush( file ); }
void FS_FCloseFile( int file ) { game_import.FS_FCloseFile( file ); }
bool FS_RemoveFile( const char *filename ) { return game_import.FS_RemoveFile( filename ); }
int FS_GetFileList( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end ) { return game_import.FS_GetFileList( dir, extension, buf, bufsize, start, end ); }
const char *FS_FirstExtension( const char *filename, const char *extensions[], int num_extensions ) { return game_import.FS_FirstExtension( filename, extensions, num_extensions ); }
bool FS_MoveFile( const char *src, const char *dst ) { return game_import.FS_MoveFile( src, dst ); }
bool ML_Update( void ) { return game_import.ML_Update(); }
bool ML_FilenameExists( const char *filename ) { return game_import.ML_FilenameExists( filename ); }
const char *ML_GetFullname( const char *filename ) { return game_import.ML_GetFullname( filename ); }
size_t ML_GetMapByNum( int num, char *out, size_t size ) { return game_import.ML_GetMapByNum( num, out, size ); }
int FakeClientConnect( char *fakeUserinfo, char *fakeSocketType, const char *fakeIP ) { return game_import.FakeClientConnect( fakeUserinfo, fakeSocketType, fakeIP ); }
int GetClientState( int numClient ) { return game_import.GetClientState( numClient ); }
void ExecuteClientThinks( int clientNum ) { game_import.ExecuteClientThinks( clientNum ); }
void DropClient( struct edict_s *ent, int type, const char *message ) { game_import.DropClient( ent, type, message ); }
void LocateEntities( struct edict_s *edicts, int edict_size, int num_edicts, int max_edicts ) { game_import.LocateEntities( edicts, edict_size, num_edicts, max_edicts ); }
struct angelwrap_api_s *asGetAngelExport( void ) { return game_import.asGetAngelExport(); }
struct stat_query_api_s *GetStatQueryAPI( void ) { return game_import.GetStatQueryAPI(); }
void MM_SendQuery( struct stat_query_s *query ) { game_import.MM_SendQuery( query ); }
void MM_GameState( bool state ) { game_import.MM_GameState( state ); }

static inline void Q_ImportGameModule( struct game_import_s *mod ) {
	game_import = *mod;
}

#endif

#endif
