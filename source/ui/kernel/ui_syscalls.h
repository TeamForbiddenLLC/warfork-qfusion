#ifndef __UI_SYSCALLS_H__
#define __UI_SYSCALLS_H__

#include "ui_public.h"
#include <cstdint>

// in ui_public.cpp
extern "C" QF_DLL_EXPORT ui_export_t *GetUIAPI( ui_import_t *import );

namespace WSWUI {
	extern ui_import_t UI_IMPORT;
}

#define UI_IMPORT_TEXTDRAWFLAGS ( TEXTDRAWFLAG_NO_COLORS | TEXTDRAWFLAG_KERNING )

namespace trap
{
		using WSWUI::UI_IMPORT;

		inline void Error( const char *str ) {
			UI_IMPORT.Error( str );
		}

		inline void Print( const char *str ) {
			UI_IMPORT.Print( str );
		}

		inline void Cmd_AddCommand( const char *name, void(*cmd)(void) ) {
			UI_IMPORT.Cmd_AddCommand( name, cmd );
		}

		inline void Cmd_RemoveCommand( const char *cmd_name ) {
			UI_IMPORT.Cmd_RemoveCommand( cmd_name );
		}

		inline void Cmd_ExecuteText( int exec_when, const char *text ) {
			UI_IMPORT.Cmd_ExecuteText( exec_when, text );
		}

		inline void Cmd_Execute( void ) {
			UI_IMPORT.Cmd_Execute ();
		}

		inline void R_RegisterWorldModel( const char *name ) {
			UI_IMPORT.R_RegisterWorldModel( name);
		}

		inline const char *ML_GetFullname( const char *name ) {
			return UI_IMPORT.ML_GetFullname( name );
		}
		inline const char *ML_GetFilename( const char *fullname ) {
			return UI_IMPORT.ML_GetFilename( fullname );
		}
		inline size_t ML_GetMapByNum( int num, char *out, size_t size ) {
				return UI_IMPORT.ML_GetMapByNum( num, out, size );
		}

		inline struct sfx_s *S_RegisterSound( const char *name ) {
			return UI_IMPORT.S_RegisterSound( name );
		}

		inline void S_StartLocalSound( const char *s ) {
			UI_IMPORT.S_StartLocalSound( s );
		}

		inline void S_StartBackgroundTrack( const char *intro, const char *loop, int mode ) {
			UI_IMPORT.S_StartBackgroundTrack( intro, loop, mode );
		}

		inline void S_StopBackgroundTrack( void ) {
			UI_IMPORT.S_StopBackgroundTrack ();
		}

		inline struct qfontface_s *SCR_RegisterFont( const char *name, qfontstyle_t style, unsigned int size ) {
			return UI_IMPORT.SCR_RegisterFont( name, style, size );
		}

		inline int SCR_DrawString( int x, int y, int align, const char *str, struct qfontface_s *font, vec4_t color ) {
			return UI_IMPORT.SCR_DrawString( x, y, align, str, font, color, UI_IMPORT_TEXTDRAWFLAGS );
		}

		inline size_t SCR_DrawStringWidth( int x, int y, int align, const char *str, size_t maxwidth, struct qfontface_s *font, vec4_t color ) {
			return UI_IMPORT.SCR_DrawStringWidth( x, y, align, str, maxwidth, font, color, UI_IMPORT_TEXTDRAWFLAGS );
		}

		inline void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct qfontface_s *font, vec4_t color ) {
			UI_IMPORT.SCR_DrawClampString( x, y, str, xmin, ymin, xmax, ymax, font, color, UI_IMPORT_TEXTDRAWFLAGS );
		}

		inline size_t SCR_FontSize( struct qfontface_s *font ) {
			return UI_IMPORT.SCR_FontSize( font );
		}

		inline size_t SCR_FontHeight( struct qfontface_s *font ) {
			return UI_IMPORT.SCR_FontHeight( font );
		}

		inline int SCR_FontUnderline( struct qfontface_s *font, int *thickness ) {
			return UI_IMPORT.SCR_FontUnderline( font, thickness );
		}

		inline size_t SCR_FontXHeight( struct qfontface_s *font ) {
			return UI_IMPORT.SCR_FontXHeight( font );
		}

		inline size_t SCR_strWidth( const char *str, struct qfontface_s *font, size_t maxlen ) {
			return UI_IMPORT.SCR_strWidth( str, font, maxlen, UI_IMPORT_TEXTDRAWFLAGS );
		}

		inline size_t SCR_StrlenForWidth( const char *str, struct qfontface_s *font, size_t maxwidth ) {
			return UI_IMPORT.SCR_StrlenForWidth( str, font, maxwidth, UI_IMPORT_TEXTDRAWFLAGS );
		}

		inline size_t SCR_FontAdvance( struct qfontface_s *font ) {
			return UI_IMPORT.SCR_FontAdvance( font );
		}

		inline fdrawchar_t SCR_SetDrawCharIntercept( fdrawchar_t intercept ) {
			return UI_IMPORT.SCR_SetDrawCharIntercept( intercept );
		}

		inline void CL_Quit( void ) {
			UI_IMPORT.CL_Quit ();
		}

		inline void CL_SetKeyDest( int key_dest ) {
			UI_IMPORT.CL_SetKeyDest( key_dest );
		}

		inline void CL_ResetServerCount( void ) {
			UI_IMPORT.CL_ResetServerCount ();
		}

		inline char *CL_GetClipboardData( bool primary ) {
			return UI_IMPORT.CL_GetClipboardData( primary );
		}

		inline void CL_FreeClipboardData( char *data ) {
			UI_IMPORT.CL_FreeClipboardData( data );
		}

		inline bool CL_IsBrowserAvailable( void ) {
			return UI_IMPORT.CL_IsBrowserAvailable();
		}

		inline void CL_OpenURLInBrowser( const char *url ) {
			UI_IMPORT.CL_OpenURLInBrowser( url );
		}
			
		inline size_t CL_ReadDemoMetaData( const char *demopath, char *meta_data, size_t meta_data_size ) {
			return UI_IMPORT.CL_ReadDemoMetaData( demopath, meta_data, meta_data_size );
		}

		inline int CL_PlayerNum( void ) {
			return UI_IMPORT.CL_PlayerNum();
		}

		inline const char *Key_GetBindingBuf( int binding ) {
			return UI_IMPORT.Key_GetBindingBuf( binding );
		}

		inline const char *Key_KeynumToString( int keynum ) {
			return UI_IMPORT.Key_KeynumToString( keynum );
		}

		inline int Key_StringToKeynum( const char *s ) {
			return UI_IMPORT.Key_StringToKeynum( s );
		}

		inline void Key_SetBinding( int keynum, const char *binding ) {
			UI_IMPORT.Key_SetBinding( keynum, binding );
		}

		inline bool Key_IsDown( int keynum ) {
			return UI_IMPORT.Key_IsDown( keynum );
		}

		inline void IN_GetThumbsticks( vec4_t sticks ) {
			UI_IMPORT.IN_GetThumbsticks( sticks );
		}

		inline void IN_ShowSoftKeyboard( bool show ) {
			UI_IMPORT.IN_ShowSoftKeyboard( show );
		}

		inline unsigned int IN_SupportedDevices( void ) {
			return UI_IMPORT.IN_SupportedDevices();
		}

		inline bool VID_GetModeInfo( int *width, int *height, int mode ) {
			return UI_IMPORT.VID_GetModeInfo( width, height, mode );
		}

		inline void VID_FlashWindow( int count ) {
			UI_IMPORT.VID_FlashWindow( count );
		}

		inline void GetConfigString( int i, char *str, int size ) {
			UI_IMPORT.GetConfigString( i, str, size );
		}

		inline unsigned int Milliseconds( void ) {
			return UI_IMPORT.Milliseconds ();
		}

		inline unsigned int Microseconds( void ) {
			return UI_IMPORT.Microseconds ();
		}

		inline cvar_t *Cvar_Get( const char *name, const char *value, int flags ) {
			return UI_IMPORT.Cvar_Get( name, value, flags );
		}

		inline cvar_t *Cvar_Set( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_Set( name, value );
		}

		inline void Cvar_SetValue( const char *name, float value ) {
			UI_IMPORT.Cvar_SetValue( name, value );
		}

		inline cvar_t *Cvar_ForceSet( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_ForceSet( name, value );
		}

		inline float Cvar_Value( const char *name ) {
			return UI_IMPORT.Cvar_Value( name );
		}

		inline int Cvar_Int( const char *name ) {
			return (int) UI_IMPORT.Cvar_Value( name );
		}

		inline const char *Cvar_String( const char *name ) {
			return UI_IMPORT.Cvar_String( name );
		}

		// dynvars
		inline dynvar_t *Dynvar_Create( const char *name, bool console, dynvar_getter_f getter, dynvar_setter_f setter ) {
			return UI_IMPORT.Dynvar_Create( name, console, getter, setter );
		}

		inline void Dynvar_Destroy( dynvar_t *dynvar ) {
			UI_IMPORT.Dynvar_Destroy( dynvar );
		}

		inline dynvar_t *Dynvar_Lookup( const char *name ) {
			return UI_IMPORT.Dynvar_Lookup( name );
		}

		inline const char *Dynvar_GetName( dynvar_t *dynvar ) {
			return UI_IMPORT.Dynvar_GetName( dynvar );
		}

		inline dynvar_get_status_t Dynvar_GetValue( dynvar_t *dynvar, void **value ) {
			return UI_IMPORT.Dynvar_GetValue( dynvar, value );
		}

		inline dynvar_set_status_t Dynvar_SetValue( dynvar_t *dynvar, void *value ) {
			return UI_IMPORT.Dynvar_SetValue( dynvar, value );
		}

		inline void Dynvar_AddListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_AddListener( dynvar, listener );
		}

		inline void Dynvar_RemoveListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_RemoveListener( dynvar, listener );
		}

		//console args
		inline int Cmd_Argc( void ) {
			return UI_IMPORT.Cmd_Argc ();
		}

		inline char *Cmd_Argv( int arg ) {
			return UI_IMPORT.Cmd_Argv( arg );
		}

		inline char *Cmd_Args( void ) {
			return UI_IMPORT.Cmd_Args ();
		}

		inline void *Mem_Alloc( size_t size, const char *filename, int fileline ) {
			return UI_IMPORT.Mem_Alloc( size, filename, fileline );
		}

		inline void Mem_Free( void *data, const char *filename, int fileline ) {
			UI_IMPORT.Mem_Free( data, filename, fileline );
		}

		inline struct angelwrap_api_s *asGetAngelExport( void ) {
			return UI_IMPORT.asGetAngelExport();
		}

		inline void AsyncStream_UrlEncode( const char *src, char *dst, size_t size ) {
			UI_IMPORT.AsyncStream_UrlEncode( src, dst, size );
		}

		inline size_t AsyncStream_UrlDecode( const char *src, char *dst, size_t size ) {
			return UI_IMPORT.AsyncStream_UrlDecode( src, dst, size );
		}

		inline int AsyncStream_PerformRequest( const char *url, const char *method, const char *data, int timeout,
			ui_async_stream_read_cb_t read_cb, ui_async_stream_done_cb_t done_cb, void *privatep ) {
				return UI_IMPORT.AsyncStream_PerformRequest( url, method, data, timeout, read_cb, done_cb, privatep );
		}
		
		inline size_t GetBaseServerURL( char *buffer, size_t buffer_size ) {
			return UI_IMPORT.GetBaseServerURL( buffer, buffer_size );
		}

		inline bool MM_Login( const char *user, const char *password ) {
			return UI_IMPORT.MM_Login( user, password );
		}

		inline bool MM_Logout( bool force ) {
			return UI_IMPORT.MM_Logout( force );
		}

		inline int MM_GetLoginState( void ) {
			return UI_IMPORT.MM_GetLoginState();
		}

		inline size_t MM_GetLastErrorMessage( char *buffer, size_t buffer_size ) {
			return UI_IMPORT.MM_GetLastErrorMessage( buffer, buffer_size );
		}

		inline size_t MM_GetProfileURL( char *buffer, size_t buffer_size, bool rml ) {
			return UI_IMPORT.MM_GetProfileURL( buffer, buffer_size, rml );
		}

		inline size_t MM_GetBaseWebURL( char *buffer, size_t buffer_size ) {
			return UI_IMPORT.MM_GetBaseWebURL( buffer, buffer_size );
		}

		inline const char *L10n_TranslateString( const char *string ) {
			return UI_IMPORT.L10n_TranslateString( string );
		}

		inline void L10n_ClearDomain( void ) {
			UI_IMPORT.L10n_ClearDomain();
		}

		inline void L10n_LoadLangPOFile( const char *filepath ) {
			UI_IMPORT.L10n_LoadLangPOFile( filepath );
		}

		inline const char *L10n_GetUserLanguage( void ) {
			return UI_IMPORT.L10n_GetUserLanguage();
		}

		inline bool GetBlocklistItem( size_t index, uint64_t* steamid_out, char* name, size_t* name_len_in_out ) {
			return UI_IMPORT.GetBlocklistItem( index, steamid_out, name, name_len_in_out );
		}
}

#endif
